#include <sys/liteos_numbers.h>

_Static_assert(LITEOS_ABI_VERSION == 1, "LiteOS ABI version changed");
_Static_assert(SYS_read == 0x4c01, "read syscall ABI changed");
_Static_assert(SYS_write == 0x4c02, "write syscall ABI changed");
_Static_assert(SYS_open == 0x4c03, "open syscall ABI changed");
_Static_assert(SYS_close == 0x4c04, "close syscall ABI changed");
_Static_assert(SYS_pipe == 0x4c05, "pipe syscall ABI changed");
_Static_assert(SYS_yield == 0x4c06, "yield syscall ABI changed");
_Static_assert(SYS_dup2 == 0x4c07, "dup2 syscall ABI changed");
_Static_assert(SYS_getpid == 0x4c08, "getpid syscall ABI changed");
_Static_assert(SYS_fork == 0x4c09, "fork syscall ABI changed");
_Static_assert(SYS_execve == 0x4c0a, "execve syscall ABI changed");
_Static_assert(SYS_exit == 0x4c0b, "exit syscall ABI changed");
_Static_assert(SYS_wait4 == 0x4c0c, "wait4 syscall ABI changed");
_Static_assert(SYS_getcwd == 0x4c0d, "getcwd syscall ABI changed");
_Static_assert(SYS_chdir == 0x4c0e, "chdir syscall ABI changed");
_Static_assert(SYS_mkdir == 0x4c0f, "mkdir syscall ABI changed");
_Static_assert(SYS_unlink == 0x4c10, "unlink syscall ABI changed");
_Static_assert(SYS_getdents64 == 0x4c11, "getdents64 syscall ABI changed");

/* Guard against accidentally reusing the Linux x86-64 syscall range. */
_Static_assert(SYS_read > 548, "LiteOS syscall numbers must not overlap Linux x86-64");

int main(void)
{
    return 0;
}
