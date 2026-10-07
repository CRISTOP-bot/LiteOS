#include <sys/liteos.h>

static inline long syscall3(long number, long first, long second, long third)
{
    long result;
    __asm__ volatile ("int $0x80"
                      : "=a"(result)
                      : "a"(number), "D"(first), "S"(second), "d"(third)
                      : "rcx", "r11", "memory");
    return result;
}

void __stack_chk_fail(void)
{
    sys_exit(139);  /* SIGSEGV-like exit code */
    for (;;)
        sys_yield();
}

long sys_read(int fd, void *buf, unsigned long n)
{
    return syscall3(SYS_read, fd, (long)buf, (long)n);
}

long sys_write(int fd, const void *buf, unsigned long n)
{
    return syscall3(SYS_write, fd, (long)buf, (long)n);
}

long sys_open(const char *path, int flags, int mode)
{
    return syscall3(SYS_open, (long)path, flags, mode);
}

long sys_close(int fd)
{
    return syscall3(SYS_close, fd, 0, 0);
}

long sys_pipe(int fds[2])
{
    return syscall3(SYS_pipe, (long)fds, 0, 0);
}

long sys_dup2(int oldfd, int newfd)
{
    return syscall3(SYS_dup2, oldfd, newfd, 0);
}

long sys_getpid(void)
{
    return syscall3(SYS_getpid, 0, 0, 0);
}

long sys_fork(void)
{
    return syscall3(SYS_fork, 0, 0, 0);
}

long sys_execve(const char *path, char *const argv[], char *const envp[])
{
    return syscall3(SYS_execve, (long)path, (long)argv, (long)envp);
}

long sys_wait4(int pid, int *status)
{
    return syscall3(SYS_wait4, pid, (long)status, 0);
}

long sys_getcwd(char *buf, unsigned long size)
{
    return syscall3(SYS_getcwd, (long)buf, (long)size, 0);
}

long sys_chdir(const char *path)
{
    return syscall3(SYS_chdir, (long)path, 0, 0);
}

long sys_mkdir(const char *path, int mode)
{
    return syscall3(SYS_mkdir, (long)path, mode, 0);
}

long sys_unlink(const char *path)
{
    return syscall3(SYS_unlink, (long)path, 0, 0);
}

long sys_getdents64(int fd, void *buf, unsigned long n)
{
    return syscall3(SYS_getdents64, fd, (long)buf, (long)n);
}

long sys_yield(void)
{
    return syscall3(SYS_yield, 0, 0, 0);
}

void sys_exit(int code)
{
    (void)syscall3(SYS_exit, code, 0, 0);
    for (;;)
        (void)syscall3(SYS_yield, 0, 0, 0);
}
