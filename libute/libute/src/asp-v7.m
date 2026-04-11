/** @file
 * @brief Murphi model of asp.c
 *
 * To run a Murphi model, you can use a tool like Rumur. This uses >57GB and
 * runs in >1h15m on my laptop.
 * https://github.com/smattr/rumur
 */

-- number of threads to model
const N_THREAD: 3;

-- number of shared pointers a single thread can have outstanding
const N_SP: 2;

const LOAD_SCALE: 8;

const REFS_MASK: LOAD_SCALE - 1;

-- type of a counter value
const COUNT_MAX: LOAD_SCALE * LOAD_SCALE - 1;
type count_t: 0..COUNT_MAX;

-- thread identifier
type tid_t: scalarset(N_THREAD);

-- pointers
type ptr_t: scalarset(3);

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
end;

-- sp_rel stack frame
type sp_rel_t: record
  old_count: count_t;
end;

-- sp_store stack frame
type sp_store_t: record
  old: asp_t;
  old_count: count_t;
end;

-- sp_cas stack frame
type sp_cas_t: record
  old: asp_t;
  old_count: count_t;
end;

-- program counter location
type pc_t: enum {
  SP_ACQ_L1,   -- sp_acq, asp.c:259
  SP_ACQ_L2,   -- sp_acq, asp.c:277
  SP_ACQ_L3,   -- sp_acq, asp.c:278
  SP_ACQ_L4,   -- sp_acq, asp.c:282
  SP_ACQ_L5,   -- sp_acq, asp.c:285
  SP_REL_L1,   -- dec_ref, asp.c:154
  SP_STORE_L1, -- sp_store, asp.c:345
  SP_STORE_L2, -- sp_store, asp.c:154/asp.c:195
  SP_CAS_L1,   -- sp_cas, asp.c:371
  SP_CAS_L2,   -- sp_cas, asp.c:383
  SP_CAS_L3,   -- sp_cas, asp.c:154/asp.c:195
};

-- thread-local storage
type tls_t: record
  pc: pc_t;
  sp: array[0..N_SP - 1] of sp_t; -- currently live sps this thread has
  sp_acq: sp_acq_t;
  sp_rel: sp_rel_t;
  sp_store: sp_store_t;
  sp_cas: sp_cas_t;
end;

/******************************************************************************/

var sp_ctrls: array[sp_ctrl_ptr_t] of sp_ctrl_t;

-- assume we have a single global asp
var asp: asp_t;

var tls: array[tid_t] of tls_t;

/******************************************************************************/

--- how many values in the type count_t?
function countof_count(): 0..COUNT_MAX + 1;
  var r: 0..COUNT_MAX + 1;
begin
  r := 0;
  for c: count_t do
    r := r + 1;
  end;
  return r;
end;

-- how many 1 bits in x?
function popcount(x: 0..COUNT_MAX + 1): count_t;
  var r: count_t;
  var t: 0..COUNT_MAX + 1;
begin
  t := x;
  r := 0;
  while t != 0 do
    if (t & 1) = 1 then
      r := r + 1;
    end;
    t := t >> 1;
  end;
  return r;
end;

function calloc_sp_ctrl(): nullable_sp_ctrl_ptr_t;
begin
  for i: sp_ctrl_ptr_t do
    if isundefined(sp_ctrls[i].value) then
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
  assert !isundefined(sp_ctrls[p].value) "double free";
  undefine sp_ctrls[p];
end;

-- add 2 counts with wrap semantics
function add(lhs: count_t; rhs: count_t): count_t;
begin
  return (lhs + rhs) % (COUNT_MAX + 1);
end;

-- subtract 2 counts with wrap semantics
function sub(lhs: count_t; rhs: count_t): count_t;
begin
  if lhs >= rhs then
    return lhs - rhs;
  end;
  return COUNT_MAX + 1 - (rhs - lhs);
end;

-- multiply 2 counts with wrap semantics
function mul(lhs: count_t; rhs: count_t): count_t;
  var r: count_t;
begin
  -- this is an extremely inefficient way of achieving this, but I did not come
  -- up with anything better on the spot
  r := 0;
  for i: count_t do
    if i < rhs then
      r := (r + lhs) % (COUNT_MAX + 1);
    end;
  end;
  return r;
end;

function inc_ref(ctrl: sp_ctrl_ptr_t; by_: count_t);
begin
  assert !isundefined(ctrl);
  assert by_ > 0 "redundant inc_ref";
  assert by_ < LOAD_SCALE "overflow";
  sp_ctrls[ctrl].ref_count := add(sp_ctrls[ctrl].ref_count, by_);
end;

function inc_load_ref(ctrl: sp_ctrl_ptr_t; by_: count_t);
begin
  assert !isundefined(ctrl);
  assert by_ > 0 "redundant inc_load_ref";
  assert (sp_ctrls[ctrl].ref_count & REFS_MASK) > 0
    "changing load count while not holding a reference";
  sp_ctrls[ctrl].ref_count := add(sp_ctrls[ctrl].ref_count, mul(by_, LOAD_SCALE));
end;

function dec_ref_1(ctrl: sp_ctrl_ptr_t): count_t;
  var old: count_t;
begin
  assert !isundefined(ctrl);
  old := sp_ctrls[ctrl].ref_count;
  sp_ctrls[ctrl].ref_count := sp_ctrls[ctrl].ref_count - 1;
  assert (old & REFS_MASK) > 0 "dropping a reference that was not held";
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

function dec_load_ref(ctrl: sp_ctrl_ptr_t);
begin
  assert !isundefined(ctrl);
  assert (sp_ctrls[ctrl].ref_count & REFS_MASK) > 0
    "changing load count while not holding a reference";
  sp_ctrls[ctrl].ref_count := sub(sp_ctrls[ctrl].ref_count, LOAD_SCALE);
end;

function inc_and_dec_1(ctrl: sp_ctrl_ptr_t; loads: count_t): count_t;
  var old: count_t;
begin
  assert !isundefined(ctrl);
  assert loads > 0 "redundant inc_and_dec";
  old := sp_ctrls[ctrl].ref_count;
  sp_ctrls[ctrl].ref_count :=
    add(sp_ctrls[ctrl].ref_count, mul(loads, LOAD_SCALE) - 1);
  assert (old & REFS_MASK) > 0 "dropping a reference that was not held";
  return add(old, mul(loads, LOAD_SCALE));
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
  -- nothing required
end;

ruleset tid: tid_t do
  ruleset value: ptr_t do
    rule "sp_new"
      -- we are idle and do not have a shared pointer
      isundefined(tls[tid].pc) & isundefined(tls[tid].sp[0].ptr) &
      -- the shared pointer we are constructing does not alias the asp global
      (isundefined(asp.ctrl) | sp_ctrls[asp.ctrl].value != value) &
      -- the shared pointer we are constructing does not alias anyone else’s
      forall peer: tid_t do
        forall i: 0..N_SP - 1 do
          isundefined(tls[peer].sp[i].ptr) | tls[peer].sp[i].ptr != value
        end
      end ==>
      var ctrl: nullable_sp_ctrl_ptr_t;
    begin
      -- omit null pointer construction, as all threads effectively start with
      -- them

      ctrl := calloc_sp_ctrl();
      if ctrl = 0 then
        return;
      end;

      sp_ctrls[ctrl].value := value;
      inc_ref(ctrl, 1);

      tls[tid].sp[0].ptr := value;
      tls[tid].sp[0].impl := ctrl;
    end;
  end;

  rule "sp_acq (1 / 6)"
    isundefined(tls[tid].pc) & isundefined(tls[tid].sp[0].ptr) ==>
  begin
    -- “load the implementation…”
    tls[tid].sp_acq.old := asp;
    tls[tid].pc := SP_ACQ_L1;
  end;

  rule "sp_acq (2 / 6)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_ACQ_L1 ==>
  begin
    if isundefined(tls[tid].sp_acq.old.ctrl) then
      -- “the target pointer is null; no need to ref count”
      undefine tls[tid].sp[0];
      undefine tls[tid].sp_acq;
      undefine tls[tid].pc;
      return;
    end;
    tls[tid].sp_acq.new := tls[tid].sp_acq.old;
    tls[tid].sp_acq.new.load_count := add(tls[tid].sp_acq.new.load_count, 1);
    if asp_cas(tls[tid].sp_acq.old, tls[tid].sp_acq.new) then
      tls[tid].sp_acq.old := tls[tid].sp_acq.new;
      assert !isundefined(tls[tid].sp_acq.old.ctrl)
        "non-null pointer has no control block";
      tls[tid].pc := SP_ACQ_L2;
    end;
  end;

  rule "sp_acq (3 / 6)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_ACQ_L2 ==>
  begin
    -- “…incrementing the reference count”
    inc_ref(tls[tid].sp_acq.old.ctrl, 1);
    tls[tid].pc := SP_ACQ_L3;
  end;

  rule "sp_acq (4 / 6)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_ACQ_L3 ==>
  begin
    -- “load the target pointer…”
    tls[tid].sp[0].ptr := sp_ctrls[tls[tid].sp_acq.old.ctrl].value;
    tls[tid].sp[0].impl := tls[tid].sp_acq.old.ctrl;
    tls[tid].pc := SP_ACQ_L4;
  end;

  rule "sp_acq (5 / 6)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_ACQ_L4 ==>
  begin
    -- “undo our increment of the load count”
    tls[tid].sp_acq.new := tls[tid].sp_acq.old;
    tls[tid].sp_acq.new.load_count := sub(tls[tid].sp_acq.new.load_count, 1);
    if asp_cas(tls[tid].sp_acq.old, tls[tid].sp_acq.new) then
      undefine tls[tid].sp_acq;
      undefine tls[tid].pc;
      return;
    end;
    tls[tid].pc := SP_ACQ_L5;
  end;

  rule "sp_acq (6 / 6)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_ACQ_L5 ==>
    var updated: asp_t;
  begin
    if isundefined(tls[tid].sp_acq.old.ctrl) |
       tls[tid].sp_acq.new.ctrl != tls[tid].sp_acq.old.ctrl then
      dec_load_ref(tls[tid].sp_acq.new.ctrl);
      undefine tls[tid].sp_acq;
      undefine tls[tid].pc;
      return;
    end;
    tls[tid].pc := SP_ACQ_L4;
  end;

  rule "sp_rel (1 / 2)"
    isundefined(tls[tid].pc) & !isundefined(tls[tid].sp[0].ptr) ==>
  begin
    -- omit null pointer release

    assert !isundefined(tls[tid].sp[0].impl)
      "non-null pointer with no control block";
    tls[tid].sp_rel.old_count := dec_ref_1(tls[tid].sp[0].impl);
    tls[tid].pc := SP_REL_L1;
  end;

  rule "sp_rel (2 / 2)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_REL_L1 ==>
  begin
    dec_ref_2(tls[tid].sp[0].impl, tls[tid].sp_rel.old_count);
    undefine tls[tid].sp[0];
    undefine tls[tid].sp_rel;
    undefine tls[tid].pc;
  end;

  rule "sp_store (1 / 3)"
    isundefined(tls[tid].pc) & isundefined(tls[tid].sp[1].ptr) ==>
    var new: asp_t;
  begin
    if isundefined(tls[tid].sp[0].impl) then
      undefine new.ctrl;
    else
      new.ctrl := tls[tid].sp[0].impl;
    end;
    new.load_count := 0;
    tls[tid].sp_store.old := asp_xchg(new);
    tls[tid].pc := SP_STORE_L1;
  end;

  rule "sp_store (2 / 3)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_STORE_L1 ==>
  begin
    if !isundefined(tls[tid].sp_store.old.ctrl) then
      if mul(tls[tid].sp_store.old.load_count, LOAD_SCALE) = 0 then
        tls[tid].sp_store.old_count := dec_ref_1(tls[tid].sp_store.old.ctrl);
      else
        tls[tid].sp_store.old_count :=
          inc_and_dec_1(tls[tid].sp_store.old.ctrl, tls[tid].sp_store.old.load_count);
      end;
      tls[tid].pc := SP_STORE_L2;
    else
      undefine tls[tid].sp[0];
      undefine tls[tid].sp_store;
      undefine tls[tid].pc;
    end;
  end;

  rule "sp_store (3 / 3)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_STORE_L2 ==>
  begin
    dec_ref_2(tls[tid].sp_store.old.ctrl, tls[tid].sp_store.old_count);
    undefine tls[tid].sp[0];
    undefine tls[tid].sp_store;
    undefine tls[tid].pc;
  end;

  rule "sp_cas (1 / 4)"
    isundefined(tls[tid].pc) ==>
  begin
    if isundefined(tls[tid].sp[0].impl) then
      undefine tls[tid].sp_cas.old.ctrl;
    else
      tls[tid].sp_cas.old.ctrl := tls[tid].sp[0].impl;
    end;
    tls[tid].sp_cas.old.load_count := 0;
    tls[tid].pc := SP_CAS_L1;
  end;

  rule "sp_cas (2 / 4)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_CAS_L1 ==>
    var new: asp_t;
  begin
    if isundefined(tls[tid].sp[1].impl) then
      undefine new.ctrl;
    else
      new.ctrl := tls[tid].sp[1].impl;
    end;
    new.load_count := 0;
    if asp_cas(tls[tid].sp_cas.old, new) then
      undefine tls[tid].sp[1];
      tls[tid].pc := SP_CAS_L2;
      return;
    end;
    if (isundefined(tls[tid].sp_cas.old.ctrl) != isundefined(tls[tid].sp[0].impl)) |
       (!isundefined(tls[tid].sp_cas.old.ctrl) & !isundefined(tls[tid].sp[0].impl) & tls[tid].sp_cas.old.ctrl != tls[tid].sp[0].impl) then
      undefine tls[tid].sp_cas;
      undefine tls[tid].pc;
    end;
  end;

  rule "sp_cas (3 / 4)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_CAS_L2 ==>
  begin
    if !isundefined(tls[tid].sp_cas.old.ctrl) then
      if mul(tls[tid].sp_cas.old.load_count, LOAD_SCALE) = 0 then
        tls[tid].sp_cas.old_count := dec_ref_1(tls[tid].sp_cas.old.ctrl);
      else
        tls[tid].sp_cas.old_count :=
          inc_and_dec_1(tls[tid].sp_cas.old.ctrl, tls[tid].sp_cas.old.load_count);
      end;
      tls[tid].pc := SP_CAS_L3;
      return;
    end;
    undefine tls[tid].sp_cas;
    undefine tls[tid].pc;
  end;

  rule "sp_cas (4 / 4)"
    !isundefined(tls[tid].pc) & tls[tid].pc = SP_CAS_L3 ==>
  begin
    dec_ref_2(tls[tid].sp_cas.old.ctrl, tls[tid].sp_cas.old_count);
    undefine tls[tid].sp_cas;
    undefine tls[tid].pc;
  end;

  rule "swap pointers"
    isundefined(tls[tid].pc) ==>
    var tmp: sp_t;
  begin
    tmp := tls[tid].sp[0];
    tls[tid].sp[0] := tls[tid].sp[1];
    tls[tid].sp[1] := tmp;
  end;
end;

invariant
  "null pointers have no control block"
  forall tid: tid_t do
    forall i: 0..N_SP - 1 do
      isundefined(tls[tid].sp[i].ptr) -> isundefined(tls[tid].sp[i].impl)
    end
  end;

invariant
  "non-null pointers have a control block"
  forall tid: tid_t do
    forall i: 0..N_SP - 1 do
      !isundefined(tls[tid].sp[i].ptr) -> !isundefined(tls[tid].sp[i].impl)
    end
  end;

invariant
  "no memory leaks of control blocks"
  forall tid: tid_t do isundefined(tls[tid].pc) end -> -- all threads idle
    forall ctrl: sp_ctrl_ptr_t do
      !isundefined(sp_ctrls[ctrl].value) -> exists tid: tid_t do exists i: 0..N_SP - 1 do
        !isundefined(tls[tid].sp[i].impl) & ctrl = tls[tid].sp[i].impl
      end end | (!isundefined(asp.ctrl) & asp.ctrl = ctrl)
    end;

invariant
  "counter is power-of-2 sized"
  popcount(countof_count()) = 1;

invariant
  "live shared pointers do not use freed control blocks"
  forall tid: tid_t do isundefined(tls[tid].pc) ->
    forall i: 0..N_SP - 1 do !isundefined(tls[tid].sp[i].ptr) ->
      !isundefined(sp_ctrls[tls[tid].sp[i].impl].value)
    end
  end;
