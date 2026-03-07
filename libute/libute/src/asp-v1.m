/** @file
 * @brief Murphi model of asp.c
 *
 * This model documents the code as it was at commit
 * 880c32ba7081df03c878765881f9112e4ac9d817. This has a known bug wherein racing
 * threads can cause a use-after-free.
 *
 * To run a Murphi model, you can use a tool like Rumur.
 * https://github.com/smattr/rumur
 */

-- number of threads to model
const N_THREAD: 3;

-- type of a counter value
type count_t: 0..N_THREAD * 2;

-- thread identifier
type tid_t: 0..N_THREAD-1;

-- pointers
type ptr_t: 0..3;

/*******************************************************************************
 * shared pointer control blocks                                               *
 ******************************************************************************/

-- a control block itself
type sp_ctrl_t: record
  value: ptr_t;
  -- ignore dtor
  ref_count: count_t;
end;

-- pointer to a sp_ctrl_t
type sp_ctrl_ptr_t: 1..N_THREAD;

type nullable_sp_ctrl_ptr_t: 0..N_THREAD;

/*******************************************************************************
 * shared pointers                                                             *
 ******************************************************************************/

type sp_t: record
  ptr: ptr_t;
  impl: sp_ctrl_ptr_t;
end;

/*******************************************************************************
 * atomic shared pointers                                                      *
 ******************************************************************************/

type asp_t: record
  ctrl: sp_ctrl_ptr_t;
  load_count: count_t;
end;

/*******************************************************************************
 * thread-local storage                                                        *
 ******************************************************************************/

-- sp_acq stack frame
type sp_acq_t: record
  old: asp_t;
  new: asp_t;
  sp_ptr: ptr_t;
  old_count: count_t;
end;

-- sp_rel stack frame
type sp_rel_t: record
  old_count: count_t;
end;

-- sp_store stack frame
type sp_store_t: record
  new: asp_t;
  old: asp_t;
  old_count: count_t;
end;

-- program counter location
type pc_t: enum {
  SP_ACQ_L1,   -- sp_acq, asp.c:146
  SP_ACQ_L2,   -- sp_acq, asp.c:161
  SP_ACQ_L3,   -- sp_acq, asp.c:166
  SP_ACQ_L4,   -- sp_acq, asp.c:170
  SP_ACQ_L5,   -- sp_acq, asp.c:176
  SP_ACQ_L6,   -- dec_ref, asp.c:81
  SP_REL_L1,   -- dec_ref, asp.c:81
  SP_STORE_L1, -- sp_store, asp.c:213
  SP_STORE_L2, -- sp_store, asp.c:219
  SP_STORE_L3  -- dec_ref, asp.c:81
};

-- thread-local storage
type tls_t: record
  pc: pc_t;
  sp: sp_t; -- currently live sp this thread has
  sp_acq: sp_acq_t;
  sp_rel: sp_rel_t;
  sp_store: sp_store_t;
end;

/******************************************************************************/

var sp_ctrls_allocated: array[sp_ctrl_ptr_t] of boolean;
var sp_ctrls: array[sp_ctrl_ptr_t] of sp_ctrl_t;

-- assume we have a single global asp
var asp: asp_t;

var tls: array[tid_t] of tls_t;

/******************************************************************************/

function calloc_sp_ctrl(): nullable_sp_ctrl_ptr_t;
begin
  for i: sp_ctrl_ptr_t do
    if !sp_ctrls_allocated[i] then
      sp_ctrls_allocated[i] := true;
      undefine sp_ctrls[i].value;
      sp_ctrls[i].ref_count := 0;
      return i;
    end;
  end;
  return 0;
end;

function free_sp_ctrl(p: sp_ctrl_ptr_t);
begin
  if isundefined(p) then
    return;
  end;
  assert sp_ctrls_allocated[p] "double free";
  sp_ctrls_allocated[p] := false;
  undefine sp_ctrls[p];
end;

function inc_ref(ctrl: sp_ctrl_ptr_t; by_: count_t);
begin
  assert !isundefined(ctrl);
  sp_ctrls[ctrl].ref_count := sp_ctrls[ctrl].ref_count + by_;
end;

function dec_ref_1(ctrl: sp_ctrl_ptr_t): count_t;
  var old: count_t;
begin
  assert !isundefined(ctrl);
  old := sp_ctrls[ctrl].ref_count;
  sp_ctrls[ctrl].ref_count := sp_ctrls[ctrl].ref_count - 1;
  assert old > 0 "dropping a reference that was not held";
  return old;
end;

function dec_ref_2(ctrl: sp_ctrl_ptr_t; old: count_t);
begin
  assert !isundefined(ctrl);
  -- “if we just dropped the last reference, clean up”
  if old = 1 then
    free_sp_ctrl(ctrl);
  end;
end;

function asp_cas(var expected: asp_t; desired: asp_t): boolean;
  var rc: boolean;
begin
  rc := true;
  if isundefined(asp.ctrl) then
    if !isundefined(expected.ctrl) then
      rc := false;
    end;
  elsif isundefined(expected.ctrl) then
    rc := false;
  elsif asp.ctrl != expected.ctrl then
    rc := false;
  end;
  if isundefined(asp.load_count) then
    if !isundefined(expected.load_count) then
      rc := false;
    end;
  elsif isundefined(expected.load_count) then
    rc := false;
  elsif asp.load_count != expected.load_count then
    rc := false;
  end;
  expected := asp;
  if rc then
    asp := desired;
  end;
  return rc;
end;

function asp_xchg(src: asp_t): asp_t;
  var old: asp_t;
begin
  old := asp;
  asp := src;
  return old;
end;

startstate begin
  for ctrl: sp_ctrl_ptr_t do
    sp_ctrls_allocated[ctrl] := false;
  end;
end;

ruleset tid: tid_t do
  ruleset value: ptr_t do
    rule "sp_new"
      isundefined(tls[tid].pc) & isundefined(tls[tid].sp.ptr) ==>
      var ctrl: nullable_sp_ctrl_ptr_t;
    begin
      -- “a null pointer needs no bookkeeping”
      if value = 0 then
        tls[tid].sp.ptr := 0;
        undefine tls[tid].sp.impl;
        return;
      end;

      ctrl := calloc_sp_ctrl();
      if ctrl = 0 then
        return;
      end;

      sp_ctrls[ctrl].value := value;
      inc_ref(ctrl, 1);

      tls[tid].sp.ptr := value;
      tls[tid].sp.impl := ctrl;
    end;
  end;

  rule "sp_acq (1 / 7)"
    isundefined(tls[tid].pc) & isundefined(tls[tid].sp.ptr) ==>
  begin
    -- “load the implementation…”
    tls[tid].sp_acq.old := asp;
    tls[tid].pc := SP_ACQ_L1;
  end;

  rule "sp_acq (2 / 7)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_ACQ_L1 ==>
  begin
    if isundefined(tls[tid].sp_acq.old.ctrl) then
      -- “the target pointer is null; no need to ref count”
      undefine tls[tid].sp;
      undefine tls[tid].sp_acq;
      undefine tls[tid].pc;
      return;
    end;
    tls[tid].sp_acq.new := tls[tid].sp_acq.old;
    tls[tid].sp_acq.new.load_count := tls[tid].sp_acq.new.load_count + 1;
    if asp_cas(tls[tid].sp_acq.old, tls[tid].sp_acq.new) then
      tls[tid].sp_acq.old := tls[tid].sp_acq.new;
      assert !isundefined(tls[tid].sp_acq.old.ctrl)
        "non-null pointer has no control block";
      tls[tid].pc := SP_ACQ_L2;
    end;
  end;

  rule "sp_acq (3 / 7)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_ACQ_L2 ==>
  begin
    -- “…incrementing the reference count”
    sp_ctrls[tls[tid].sp_acq.old.ctrl].ref_count :=
      sp_ctrls[tls[tid].sp_acq.old.ctrl].ref_count + 1;
    tls[tid].pc := SP_ACQ_L3;
  end;

  rule "sp_acq (4 / 7)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_ACQ_L3 ==>
  begin
    -- “load the target pointer…”
    -- Do a funny increment loop here to simulate something like torn reads
    if isundefined(tls[tid].sp_acq.sp_ptr) then
      tls[tid].sp_acq.sp_ptr := 1;
    else
      tls[tid].sp_acq.sp_ptr := tls[tid].sp_acq.sp_ptr + 1;
    end;
    if tls[tid].sp_acq.sp_ptr < sp_ctrls[tls[tid].sp_acq.old.ctrl].value then
      return;
    end;
    tls[tid].sp.ptr := tls[tid].sp_acq.sp_ptr;
    tls[tid].sp.impl := tls[tid].sp_acq.old.ctrl;
    tls[tid].pc := SP_ACQ_L4;
  end;

  rule "sp_acq (5 / 7)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_ACQ_L4 ==>
  begin
    -- “undo our increment of the load count”
    assert tls[tid].sp_acq.old.load_count > 0
      "sp_acq decremented a load for a control block it was not using";
    tls[tid].sp_acq.new := tls[tid].sp_acq.old;
    tls[tid].sp_acq.new.load_count := tls[tid].sp_acq.new.load_count - 1;
    if asp_cas(tls[tid].sp_acq.old, tls[tid].sp_acq.new) then
      undefine tls[tid].sp_acq;
      undefine tls[tid].pc;
      return;
    end;
    tls[tid].pc := SP_ACQ_L5;
  end;

  rule "sp_acq (6 / 7)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_ACQ_L5 ==>
    var updated: asp_t;
  begin
    if isundefined(tls[tid].sp_acq.old.ctrl) |
       tls[tid].sp_acq.new.ctrl != tls[tid].sp_acq.old.ctrl then
      tls[tid].sp_acq.old_count := dec_ref_1(tls[tid].sp_acq.new.ctrl);
      tls[tid].pc := SP_ACQ_L6;
      return;
    end;
    tls[tid].pc := SP_ACQ_L4;
  end;

  rule "sp_acq (7 / 7)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_ACQ_L6 ==>
  begin
    dec_ref_2(tls[tid].sp_acq.new.ctrl, tls[tid].sp_acq.old_count);
    undefine tls[tid].sp_acq;
    undefine tls[tid].pc;
  end;

  rule "sp_rel (1 / 2)"
    isundefined(tls[tid].pc) & !isundefined(tls[tid].sp.ptr) ==>
  begin
    -- “if the target pointer was null, it was not reference counted”
    if tls[tid].sp.ptr = 0 then
      assert isundefined(tls[tid].sp.impl)
        "null pointer with non-null control block";
      undefine tls[tid].sp;
      return;
    end;

    assert !isundefined(tls[tid].sp.impl)
      "non-null pointer with no control block";
    tls[tid].sp_rel.old_count := dec_ref_1(tls[tid].sp.impl);
    tls[tid].pc := SP_REL_L1;
  end;

  rule "sp_rel (2 / 2)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_REL_L1 ==>
  begin
    dec_ref_2(tls[tid].sp.impl, tls[tid].sp_rel.old_count);
    undefine tls[tid].sp;
    undefine tls[tid].sp_rel;
    undefine tls[tid].pc;
  end;

  rule "sp_store (1 / 4)"
    isundefined(tls[tid].pc) & !isundefined(tls[tid].sp.ptr) ==>
  begin
    if isundefined(tls[tid].sp.impl) then
      undefine tls[tid].sp_store.new.ctrl;
    else
      tls[tid].sp_store.new.ctrl := tls[tid].sp.impl;
    end;
    tls[tid].sp_store.new.load_count := 0;
    tls[tid].sp_store.old := asp_xchg(tls[tid].sp_store.new);
    tls[tid].pc := SP_STORE_L1;
  end;

  rule "sp_store (2 / 4)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_STORE_L1 ==>
  begin
    if !isundefined(tls[tid].sp_store.old.ctrl) then
      if tls[tid].sp_store.old.load_count > 0 then
        inc_ref(tls[tid].sp_store.old.ctrl, tls[tid].sp_store.old.load_count);
      end;
      tls[tid].pc := SP_STORE_L2;
    else
      undefine tls[tid].sp;
      undefine tls[tid].sp_store;
      undefine tls[tid].pc;
    end;
  end;

  rule "sp_store (3 / 4)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_STORE_L2 ==>
  begin
    tls[tid].sp_store.old_count := dec_ref_1(tls[tid].sp_store.old.ctrl);
    tls[tid].pc := SP_STORE_L3;
  end;

  rule "sp_store (4 / 4)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_STORE_L3 ==>
  begin
    dec_ref_2(tls[tid].sp_store.old.ctrl, tls[tid].sp_store.old_count);
    undefine tls[tid].sp;
    undefine tls[tid].sp_store;
    undefine tls[tid].pc;
  end;
end;

invariant
  "null pointers have no control block"
  forall tid: tid_t do
    !isundefined(tls[tid].sp.ptr) & tls[tid].sp.ptr = 0 ->
      isundefined(tls[tid].sp.impl)
  end;

invariant
  "non-null pointers have a control block"
  forall tid: tid_t do
    !isundefined(tls[tid].sp.ptr) & tls[tid].sp.ptr != 0 ->
      !isundefined(tls[tid].sp.impl)
  end;

invariant
  "live shared pointers do not use freed control blocks"
  forall tid: tid_t do
    !isundefined(tls[tid].sp.impl) -> sp_ctrls_allocated[tls[tid].sp.impl]
  end;

invariant
  "no memory leaks of control blocks"
  forall ctrl: sp_ctrl_ptr_t do
    sp_ctrls_allocated[ctrl] -> exists tid: tid_t do
      !isundefined(tls[tid].sp.impl) & ctrl = tls[tid].sp.impl
    end
  end;
