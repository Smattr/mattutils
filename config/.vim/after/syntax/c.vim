" Extension to C syntax for systems code
" Language: C
" Maintainer: Matthew Fernandez <matthew.fernandez@gmail.com>
" License: Public domain

" Assert has always been morally an operator to me, though I know it usually expands to a function
syn keyword cOperator assert

" GCC builtins
syn keyword cOperator __builtin_alloca __builtin_alloca_with_align __builtin_apply
    \ __builtin_apply_args __builtin_assume_aligned __builtin_bswap16 __builtin_bswap32
    \ __builtin_bswap64 __builtin_call_with_static_chain __builtin_choose_expr
    \ __builtin___clear_cache __builtin_clrsb __builtin_clrsbl __builtin_clrsbll __builtin_clz
    \ __builtin_clzl __builtin_clzll __builtin_complex __builtin_constant_p __builtin_ctz
    \ __builtin_ctzl __builtin_ctzll __builtin_expect __builtin_extract_return_addr __builtin_FILE
    \ __builtin_fpclassify __builtin_ffs __builtin_ffsl __builtin_ffsll __builtin_frame_address
    \ __builtin_frob_return_address __builtin_FUNCTION __builtin_huge_val __builtin_huge_valf
    \ __builtin_huge_vall __builtin_inf __builtin_infd32 __builtin_infd64 __builtin_infd128
    \ __builtin_inff __builtin_infl __builtin_isinf_sign __builtin_LINE __builtin_nan
    \ __builtin_nand32 __builtin_nand64 __builtin_nand128 __builtin_nanf __builtin_nanl
    \ __builtin_nans __builtin_nansf __builtin_nansl __builtin_object_size __builtin_offsetof
    \ __builtin_parity __builtin_parityl __builtin_parityll __builtin_popcount __builtin_popcountl
    \ __builtin_popcountll __builtin_powi __builtin_powif __builtin_powil __builtin_prefetch
    \ __builtin_return __builtin_return_address __builtin_trap __builtin_types_compatible_p
    \ __builtin_unreachable __builtin_va_arg_pack __builtin_va_arg_pack_len __clear_cache
    \ __builtin_add_overflow __builtin_sadd_overflow __builtin_saddl_overflow
    \ __builtin_saddll_overflow __builtin_uadd_overflow __builtin_uaddl_overflow
    \ __builtin_uaddll_overflow __builtin_sub_overflow __builtin_ssub_overflow
    \ __builtin_ssubl_overflow __builtin_ssubll_overflow __builtin_usub_overflow
    \ __builtin_usubl_overflow __builtin_usubll_overflow __builtin_mul_overflow
    \ __builtin_smul_overflow __builtin_smull_overflow __builtin_smulll_overflow
    \ __builtin_umul_overflow __builtin_umull_overflow __builtin_umulll_overflow
    \ __builtin_add_overflow_p __builtin_sub_overflow_p __builtin_mul_overflow_p

" GCC x86 CPUID builtins
syn keyword cOperator __builtin_cpu_init __builtin_cpu_is __builtin_cpu_supports

" GCC extensions
syn keyword cType __label__ __auto_type __int128 __thread
syn keyword cOperator __alignof__ __asm__ typeof __typeof __typeof__
syn keyword cConstant __FUNCTION__ __PRETTY_FUNCTION__

" Clang builtins
syn keyword cOperator __builtin_assume __builtin_bitreverse __builtin_convertvector
    \ __builtin_readcyclecounter __builtin_shufflevector __builtin_unpredictable

" Clang ARM builtins
syn keyword cOperator __builtin_arm_clrex __builtin_arm_ldaex __builtin_arm_ldrex
    \ __builtin_arm_stlex __builtin_arm_strex __dmb __dsb __isb 

" Linux extras
syn keyword cType __force __iomem __kernel __kernel_size_t __must_check __nocast __percpu __pmem
    \ __private __rcu __safe __user notrace 
syn keyword cType __compat_gid_t __compat_gid16_t __compat_gid32_t __compat_uid_t __compat_uid16_t
    \ __compat_uid32_t __s8 __s16 __s32 __s64 __u8 __u16 __u32 __u64 compat_caddr_t compat_clock_t
    \ compat_daddr_t compat_dev_t compat_fsid_t compat_ino_t compat_int_t compat_ipc_pid_t
    \ compat_key_t compat_loff_t compat_long_t compat_mode_t compat_nlink_t compat_off_t
    \ compat_old_sigset_t compat_pid_t compat_s64 compat_short_t compat_sigset_word compat_size_t
    \ compat_ssize_t compat_time_t compat_timer_t compat_u64 compat_uint_t compat_ulong_t
    \ compat_uptr_t compat_ushort_t dev_t gfp_t irqreturn_t loff_t s8 s16 s32 s64 u8 u16 u32 u64
syn keyword cConstant __ATTR_NULL __GFP_HIGHMEM __GFP_HIGH COMPAT_LOFF_T_MAX COMPAT_OFF_T_MAX
    \ EADDRINUSE EADDRNOTAVAIL EADV EAFNOSUPPORT EALREADY EBADE EBADFD EBADR EBADRQC EBADSLT EBFRONT
    \ ECHRNG ECOMM ECONNABORTED ECONNREFUSED ECONNRESET EDEADLOCK EDESTADDRREQ EDOTDOT EDQUOT
    \ EHOSTDOWN EHOSTUNREACH EHWPOISON EIDRM EISCONN EISNAM EKEYEXPIRED EKEYREJECTED EKEYREVOKED
    \ EL2HLT EL2NSYNC EL3HLT EL3RST ELNRNG ELOOP EMULTIHOP ENETDOWN ENETRESET ENETUNREACH ENOANO
    \ ENODATA ENOTCONN ELIBACC ELIBBAD ELIBEXEC ELIBMAX ELIBSCN EMEDIUMTYPE ENAVAIL ENOBUFS ENOKEY
    \ ENOLINK ENOMEDIUM ENOMSG ENONET ENOPKG ENOPROTOOPT ENOSCI ENOSR ENOSTR ENOTNAM ENOTRECOVERABLE
    \ ENOTSOCK ENOTUNIQ EOPNOTSUPP EOVERFLOW EOWNERDEAD EPFNOSUPPORT EPROTO EPROTONOSUPPORT
    \ EPROTOTYPE EREMCHG EREMOTE EREMOTEIO ERESTART ERFKILL ESHUTDOWN ESOCKTNOSUPPORT ESRMNT ESTALE
    \ ESTRPIPE ETIME ETOOMANYREFS EUCLEAN EUNATCH EUSERS EWOULDBLOCK EXFULL GFP_ATOMIC GFP_KERNEL
    \ IRQ_NONE IRQ_HANDLED IRQ_WAKE_THREAD PAGE_SIZE
    \ THIS_MODULE UIO_FASTIOV UIO_MAXIOV PAGE_SHIFT GFP_HIGHUSER
syn keyword cOperator BUILD_BUG_ON BUG_ON EXPORT_SYMBOL EXPORT_SYMBOL_GPL MODULE_AUTHOR
    \ MODULE_DESCRIPTION MODULE_DEVICE_TABLE MODULE_LICENSE WARN_ON BUG MODULE_INFO MODULE_ALIAS
    \ MODULE_VERSION

" `poll` constants
syn keyword cConstant POLLERR POLLHUP POLLIN POLLNVAL POLLOUT POLLPRI POLLRDBAND
  \ POLLRDHUP POLLRDNORM POLLWRBAND POLLWRNORM

" Linux atomics. These are not scalars, but the way we treat them more or less is.
syn keyword cType atomic_t atomic64_t

" C11 atomics
syn keyword cType atomic_bool atomic_char atomic_schar atomic_uchar atomic_short atomic_ushort
    \ atomic_int atomic_uint atomic_long atomic_ulong atomic_llong atomic_ullong atomic_char16_t
    \ atomic_char32_t atomic_wchar_t atomic_int_least8_t atomic_uint_least8_t atomic_int_least16_t
    \ atomic_uint_least16_t atomic_int_least32_t atomic_uint_least32_t atomic_int_least64_t
    \ atomic_uint_least64_t atomic_int_fast8_t atomic_uint_fast8_t atomic_int_fast16_t
    \ atomic_uint_fast16_t atomic_int_fast32_t atomic_uint_fast32_t atomic_int_fast64_t
    \ atomic_uint_fast64_t atomic_intptr_t atomic_uintptr_t atomic_size_t atomic_ptrdiff_t
    \ atomic_intmax_t atomic_uintmax_t atomic_flag
syn keyword cConstant memory_order_relaxed memory_order_consume memory_order_acquire
    \ memory_order_release memory_order_acq_rel memory_order_seq_cst ATOMIC_FLAG_INIT

" GCC built-in atomics
syn keyword cOperator __atomic_add_fetch __atomic_and_fetch __atomic_always_lock_free __atomic_clear
    \ __atomic_compare_exchange __atomic_compare_exchange_n __atomic_exchange __atomic_exchange_n
    \ __atomic_fetch_add __atomic_fetch_and __atomic_fetch_nand __atomic_fetch_or __atomic_fetch_sub
    \ __atomic_fetch_xor __atomic_is_lock_free __atomic_load __atomic_load_n __atomic_nand_fetch
    \ __atomic_or_fetch __atomic_signal_fence __atomic_store __atomic_store_n __atomic_sub_fetch
    \ __atomic_test_and_set __atomic_thread_fence __atomic_xor_fetch
syn keyword cConstant __ATOMIC_ACQ_REL __ATOMIC_ACQUIRE __ATOMIC_CONSUME __ATOMIC_RELAXED
    \ __ATOMIC_RELEASE __ATOMIC_SEQ_CST

" GCC hacking
syn keyword cOperator gcc_assert

" LLVM hacking
syn keyword cOperator llvm_unreachable

" Signal hacking
syn keyword cConstant SG_SUCCESS SG_ERR_NOMEM SG_ERR_INVAL SG_ERR_UNKNOWN SG_ERR_DUPLICATE_MESSAGE
    \ SG_ERR_INVALID_KEY SG_ERR_INVALID_KEY_ID SG_ERR_INVALID_MAC SG_ERR_INVALID_MESSAGE
    \ SG_ERR_INVALID_VERSION SG_ERR_LEGACY_MESSAGE SG_ERR_NO_SESSION SG_ERR_STALE_KEY_EXCHANGE
    \ SG_ERR_UNTRUSTED_IDENTITY SG_ERR_VRF_SIG_VERIF_FAILED SG_ERR_INVALID_PROTO_BUF
    \ SG_ERR_FP_VERSION_MISMATCH SG_ERR_FP_IDENT_MISMATCH

" SQLite
syn keyword cConstant SQLITE_OK SQLITE_ERROR SQLITE_INTERNAL SQLITE_PERM SQLITE_ABORT SQLITE_BUSY
    \ SQLITE_LOCKED SQLITE_NOMEM SQLITE_READONLY SQLITE_INTERRUPT SQLITE_IOERR SQLITE_CORRUPT
    \ SQLITE_NOTFOUND SQLITE_FULL SQLITE_CANTOPEN SQLITE_PROTOCOL SQLITE_EMPTY SQLITE_SCHEMA
    \ SQLITE_TOOBIG SQLITE_CONSTRAINT SQLITE_MISMATCH SQLITE_MISUSE SQLITE_NOLFS SQLITE_AUTH
    \ SQLITE_FORMAT SQLITE_RANGE SQLITE_NOTADB SQLITE_NOTICE SQLITE_WARNING SQLITE_ROW SQLITE_DONE
    \ SQLITE_STATIC SQLITE_TRANSIENT
syn keyword cType sqlite3_int64 sqlite3_uint64 sqlite3_value

" getopt
syn keyword cConstant no_argument optional_argument required_argument

" Misc glibc
syn keyword cConstant F_OK R_OK W_OK X_OK DT_BLK DT_CHR DT_DIR DT_FIFO DT_LNK DT_REG DT_SOCK DT_UNKNOWN
    \ O_RDONLY O_WRONLY O_RDWR O_CLOEXEC O_APPEND O_ASYNC O_CREAT O_DIRECT O_DIRECTORY O_DSYNC O_EXCL
    \ O_LARGEFILE O_NOATIME O_NOCTTY O_NOFOLLOW O_NONBLOCK O_NDELAY O_PATH O_SYNC O_TRUNC PROT_READ
    \ PROT_EXEC PROT_WRITE PROT_NONE MAP_SHARED MAP_PRIVATE MAP_FAILED REG_EXTENDED REG_ICASE
    \ REG_NOSUB REG_NEWLINE REG_NOTEOL REG_NOTBOL REG_NOMATCH REG_BADBR REG_BADPAT REG_BADRPT
    \ REG_EBRACE REG_EBRACK REG_ECOLLATE REG_ECTYPE REG_EEND REG_EESCAPE REG_EPAREN REG_ERANGE
    \ REG_ESIZE REG_ESPACE REG_ESUBREG MAP_32BIT MAP_ANON MAP_ANONYMOUS MAP_DENYWRITE MAP_EXECUTABLE
    \ MAP_FILE MAP_FIXED MAP_GROWSDOWN MAP_HUGETLB MAP_LOCKED MAP_NONBLOCK MAP_NORESERVE
    \ MAP_POPULATE MAP_STACK MAP_UNINITIALIZED MAP_AUTOGROW MAP_AUTORESRV MAP_COPY MAP_LOCAL

" sysconf
syn keyword cConstant _SC_ARG_MAX _SC_CHILD_MAX _SC_HOST_NAME_MAX _SC_LOGIN_NAME_MAX _SC_CLK_TCK
    \ _SC_OPEN_MAX _SC_PAGESIZE _SC_RE_DUP_MAX _SC_STREAM_MAX _SC_SYMLOOP_MAX _SC_TTY_NAME_MAX
    \ _SC_TZNAME_MAX _SC_VERSION _SC_BC_BASE_MAX _SC_BC_DIM_MAX _SC_BC_SCALE_MAX _SC_BC_STRING_MAX
    \ _SC_COLL_WEIGHTS_MAX _SC_EXPR_NEST_MAX _SC_LINE_MAX _SC_2_VERSION _SC_2_C_DEV _SC_2_FORT_DEV
    \ _SC_2_FORT_RUN _SC_2_LOCALEDEF _SC_2_SW_DEV _SC_PHYS_PAGES _SC_AVPHYS_PAGES
    \ _SC_NPROCESSORS_CONF _SC_NPROCESSORS_ONLN

" Apple semaphore interface
syn keyword cConstant DISPATCH_TIME_FOREVER DISPATCH_TIME_NOW

" Linux Seccomp constants
syn keyword cConstant PR_GET_NO_NEW_PRIVS PR_GET_SECCOMP PR_SET_NO_NEW_PRIVS PR_SET_SECCOMP

" Linux Seccomp modes
syn keyword cConstant SECCOMP_MODE_STRICT SECCOMP_MODE_FILTER
  \ SECCOMP_SET_MODE_STRICT SECCOMP_SET_MODE_FILTER SECCOMP_GET_ACTION_AVAIL

" Linux Seccomp filter flags
syn keyword cConstant SECCOMP_FILTER_FLAG_TSYNC SECCOMP_FILTER_FLAG_LOG
  \ SECCOMP_FILTER_FLAG_SPEC_ALLOW

" Clang Block syntax
syn keyword cType __block

" file descriptors
syn keyword cConstant STDIN_FILENO STDOUT_FILENO STDERR_FILENO

" x86 HLE
syn keyword cConstant __ATOMIC_HLE_ACQUIRE __ATOMIC_HLE_RELEASE

" x86 RTM
syn keyword cConstant _XABORT_EXPLICIT _XABORT_RETRY _XABORT_CONFLICT
  \ _XABORT_CAPACITY _XABORT_DEBUG _XABORT_NESTED _XBEGIN_STARTED

" macOS sandbox_init() constants
syn keyword cConstant kSBXProfileNoInternet kSBXProfileNoNetwork
  \ kSBXProfileNoWrite kSBXProfileNoWriteExceptTemporary
  \ kSBXProfilePureComputation

" flock()
syn keyword cConstant LOCK_EX LOCK_NB LOCK_SH LOCK_UN

" openat()
syn keyword cConstant AT_FDCWD

" ioctl_tty
syn keyword cConstant TCGETS TCSETS TCSETSW TCSETSF TIOCGLCKTRMIOS
  \ TIOCSLCKTRMIOS TIOCGWINSZ TIOCSWINSZ TCSBRK TCSBRKP TIOCSBRK TIOCCBRK TCXONC
  \ FIONREAD TIOCINQ TIOCOUTQ TCFLSH TIOCSTI TIOCCONS TIOCSCTTY TIOCNOTTY
  \ TIOCGPGRP TIOCSPGRP TIOCGSID TIOCEXCL TIOCGEXCL TIOCNXCL TIOCGETD TIOCSETD
  \ TIOCPKT TIOCGPKT TIOCSPTLCK TIOCGPTLCK TIOCGPTPEER TIOCMGET TIOCMSET
  \ TIOCMBIC TIOCMBIS TIOCMIWAIT TIOCGICOUNT TIOCGSOFTCAR TIOCSSOFTCAR
  \ TIOCTTYGSTRUCT

" tcsetattr()
syn keyword cConstant TCSANOW TCSADRAIN TCSAFLUSH

" madvise()
syn keyword cConstant POSIX_MADV_NORMAL POSIX_MADV_SEQUENTIAL POSIX_MADV_RANDOM
  \ POSIX_MADV_WILLNEED POSIX_MADV_DONTNEED MADV_NORMAL MADV_SEQUENTIAL
  \ MADV_RANDOM MADV_WILLNEED MADV_DONTNEED MADV_REMOVE MADV_DONTFORK
  \ MADV_DOFORK MADV_HWPOISON MADV_MERGEABLE MADV_UNMERGEABLE MADV_SOFT_OFFLINE
  \ MADV_HUGEPAGE MADV_NOHUGEPAGE MADV_DONTDUMP MADV_DODUMP MADV_FREE
  \ MADV_WIPEONFORK MADV_KEEPONFORK MADV_COLD MADV_PAGEOUT MADV_NOSYNC
  \ MADV_AUTOSYNC MADV_NOCORE MADV_CORE MADV_PROTECT

" getrlimit()/setrlimit()
syn keyword cConstant RLIMIT_AS RLIMIT_CORE RLIMIT_CPU RLIMIT_DATA RLIMIT_FSIZE
  \ RLIMIT_LOCKS RLIMIT_MEMLOCK RLIMIT_MSGQUEUE RLIMIT_NICE RLIMIT_NOFILE
  \ RLIMIT_NPROC RLIMIT_RSS RLIMIT_RTPRIO RLIMIT_RTTIME RLIMIT_SIGPENDING
  \ RLIMIT_STACK

" __fsetlocking()
syn keyword cConstant FSETLOCKING_INTERNAL FSETLOCKING_BYCALLER
  \ FSETLOCKING_QUERY

" ptrace
syn keyword cConstant PTRACE_EVENT_CLONE PTRACE_EVENT_EXEC PTRACE_EVENT_EXIT
  \ PTRACE_EVENT_FORK PTRACE_EVENT_VFORK PTRACE_EVENT_VFORK_DONE
  \ PTRACE_EVENT_SECCOMP
syn keyword cConstant PTRACE_ATTACH PTRACE_CONT PTRACE_DETACH
  \ PTRACE_GET_SYSCALL_INFO PTRACE_GET_THREAD_AREA PTRACE_GETEVENTMSG
  \ PTRACE_GETFPREGS PTRACE_GETREGS PTRACE_GETREGSET PTRACE_GETSIGINFO
  \ PTRACE_GETSIGMASK PTRACE_INTERRUPT PTRACE_KILL PTRACE_LISTEN PTRACE_PEEKDATA
  \ PTRACE_PEEKTEXT PTRACE_PEEKUSER PTRACE_POKEDATA PTRACE_POKETEXT
  \ PTRACE_POKEUSER PTRACE_SECCOMP_GET_FILTER PTRACE_SEIZE PTRACE_SETFPREGS
  \ PTRACE_SETOPTIONS PTRACE_SETREGS PTRACE_SETREGSET PTRACE_SETSIGINFO
  \ PTRACE_SETSIGMASK PTRACE_SET_SYSCALL PTRACE_SET_THREAD_AREA
  \ PTRACE_SINGLESTEP PTRACE_SYSCALL PTRACE_SYSEMU PTRACE_SYSEMU_SINGLESTEP
  \ PTRACE_TRACEME
syn keyword cConstant PTRACE_O_EXITKILL PTRACE_O_TRACECLONE PTRACE_O_TRACECLONE
  \ PTRACE_O_TRACEEXEC PTRACE_O_TRACEEXIT PTRACE_O_TRACEFORK
  \ PTRACE_O_TRACESYSGOOD PTRACE_O_TRACEVFORK PTRACE_O_TRACEVFORKDONE
  \ PTRACE_O_TRACESECCOMP PTRACE_O_SYSPEND_SECCOMP

" C11 threads
syn keyword cConstant thrd_success thrd_nomem thrd_timeout thrd_busy thrd_error

" memfd_create
syn keyword cConstant MFD_CLOEXEC MFD_ALLOW_SEALING MFD_HUGETLB

" clock_gettime
syn keyword cConstant CLOCK_REALTIME CLOCK_REALTIME_ALARM CLOCK_REALTIME_COARSE
  \ CLOCK_TAI CLOCK_MONOTONIC CLOCK_MONOTONIC_COARSE CLOCK_MONOTONIC_RAW
  \ CLOCK_BOOTTIME CLOCK_BOOTTIME_ALARM CLOCK_PROCESS_CPUTIME_ID
  \ CLOCK_THREAD_CPUTIME_ID

" wait
syn keyword cConstant WNOHANG WUNTRACED WCONTINUED WEXITED WSTOPPED WNOWAIT
  \ __WCLONE __WALL __WNOTHREAD
