# LiteOS x86-64 userspace ABI (current subset)

This document describes LiteOS userspace ABI version 1 as implemented by the
current kernel and libc. It is an implementation contract for this revision,
not a claim of POSIX, Linux, or general Unix compatibility. The syscall numbers
occupy the LiteOS-specific `0x4c00` namespace, separate from Linux x86-64
numbering. Values are centralized in
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
| `0x4c01` | `read` | fd, buffer, byte count | VFS/device/pipe read |
| `0x4c02` | `write` | fd, buffer, byte count | VFS/device/pipe write |
| `0x4c03` | `open` | path, flags, mode | VFS open; mode is currently ignored |
| `0x4c04` | `close` | fd | Close descriptor |
| `0x4c05` | `pipe` | pointer to two fds | Create pipe and copy descriptor pair to user memory |
| `0x4c06` | `yield` | — | Yield to scheduler |
| `0x4c07` | `dup2` | old fd, new fd | Duplicate descriptor |
| `0x4c08` | `getpid` | — | Current process ID |
| `0x4c09` | `fork` | — | Clone process/address space |
| `0x4c0a` | `execve` | path, argv, envp | Load the supported static ELF64 executable subset |
| `0x4c0b` | `exit` | status | Exit process |
| `0x4c0c` | `wait4` | pid, status pointer, — | Wait for child; `pid` semantics are currently a subset |
| `0x4c0d` | `getcwd` | buffer, size, — | Copy current working directory |
| `0x4c0e` | `chdir` | path | Change working directory |
| `0x4c0f` | `mkdir` | path, mode, — | Create directory; mode is currently ignored |
| `0x4c10` | `unlink` | path | Remove file/directory subject to VFS rules |
| `0x4c11` | `getdents64` | fd, buffer, byte count | Return current VFS directory entries |

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
