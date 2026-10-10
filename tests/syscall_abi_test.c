#include <sys/liteos_numbers.h>

_Static_assert(SYS_read == 0, "read syscall ABI changed");
_Static_assert(SYS_write == 1, "write syscall ABI changed");
_Static_assert(SYS_open == 2, "open syscall ABI changed");
_Static_assert(SYS_close == 3, "close syscall ABI changed");
_Static_assert(SYS_pipe == 22, "pipe syscall ABI changed");
_Static_assert(SYS_yield == 24, "yield syscall ABI changed");
_Static_assert(SYS_dup2 == 33, "dup2 syscall ABI changed");
_Static_assert(SYS_getpid == 39, "getpid syscall ABI changed");
_Static_assert(SYS_fork == 57, "fork syscall ABI changed");
_Static_assert(SYS_execve == 59, "execve syscall ABI changed");
_Static_assert(SYS_exit == 60, "exit syscall ABI changed");
_Static_assert(SYS_wait4 == 61, "wait4 syscall ABI changed");
_Static_assert(SYS_getcwd == 79, "getcwd syscall ABI changed");
_Static_assert(SYS_chdir == 80, "chdir syscall ABI changed");
_Static_assert(SYS_mkdir == 83, "mkdir syscall ABI changed");
_Static_assert(SYS_unlink == 87, "unlink syscall ABI changed");
_Static_assert(SYS_getdents64 == 217, "getdents64 syscall ABI changed");

int main(void)
{
    return 0;
}
