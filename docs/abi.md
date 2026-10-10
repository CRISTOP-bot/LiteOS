# LiteOS x86-64 userspace ABI (current subset)

This document describes the ABI implemented by the current kernel and libc. It
is an implementation contract for this LiteOS revision, not a claim of POSIX,
Linux, or general Unix compatibility. The syscall numbers are LiteOS ABI
assignments; their numeric overlap with Linux x86-64 is historical and does not
imply behavioral or binary compatibility. Values are centralized in
`libc/include/sys/liteos_numbers.h`; do not renumber or reuse them without an
explicit ABI break and migration plan.

## System-call entry and return

- Entry: software interrupt `int $0x80` from 64-bit user mode.
- `RAX`: syscall number on entry; result on return.
- Arguments: `RDI`, `RSI`, and `RDX` in that order (up to three arguments).
- Successful calls return a nonnegative result unless a particular operation
  uses another documented value (for example `fork` returns zero in the child
  and the child's PID in the parent).
- Failures return the negative kernel errno value directly in `RAX`; libc's
  `sys_*` functions currently preserve that raw convention. This is not the
  conventional POSIX `-1` plus `errno` interface.
- The current syscall implementation does not promise a broader register
  preservation or signal/restart convention beyond the behavior of the
  interrupt entry code. See `src/arch/x86_64/isrs.S` and `src/syscall.c`.

## Assigned calls

| Number | Name | Arguments (`RDI`, `RSI`, `RDX`) | Current implementation |
|---:|---|---|---|
| 0 | `read` | fd, buffer, byte count | VFS/device/pipe read |
| 1 | `write` | fd, buffer, byte count | VFS/device/pipe write |
| 2 | `open` | path, flags, mode | VFS open; mode is currently ignored |
| 3 | `close` | fd | Close descriptor |
| 22 | `pipe` | pointer to two fds | Create pipe and copy descriptor pair to user memory |
| 24 | `yield` | — | Yield to scheduler |
| 33 | `dup2` | old fd, new fd | Duplicate descriptor |
| 39 | `getpid` | — | Current process ID |
| 57 | `fork` | — | Clone process/address space |
| 59 | `execve` | path, argv, envp | Load the supported static ELF64 executable subset |
| 60 | `exit` | status | Exit process |
| 61 | `wait4` | pid, status pointer, — | Wait for child; `pid` semantics are currently a subset |
| 79 | `getcwd` | buffer, size, — | Copy current working directory |
| 80 | `chdir` | path | Change working directory |
| 83 | `mkdir` | path, mode, — | Create directory; mode is currently ignored |
| 87 | `unlink` | path | Remove file/directory subject to VFS rules |
| 217 | `getdents64` | fd, buffer, byte count | Return current VFS directory entries |

The numeric assignments are also compile-time inputs to user startup assembly
(e.g. the exit call), the syscall dispatcher, and libc declarations. The
host-side `test-syscall-abi` test guards the assigned values. That test validates
ABI-number consistency only; it does not validate kernel behavior or establish
compatibility with another operating system.

## Deliberate limitations

The current implementation is a small, evolving OS interface. It does not
provide a complete POSIX syscall set, dynamic linking, PIE loading, `mmap`,
`stat`/`fstat`, `lseek`, a POSIX `errno` variable, or a general-purpose libc
allocator. ELF loading currently targets static ELF64 little-endian x86-64
`ET_EXEC` files only. These limitations must be checked against the code and
integration tests before adding libc declarations or describing an API as
available.
