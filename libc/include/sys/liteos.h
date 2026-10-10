#ifndef LITEOS_SYS_H
#define LITEOS_SYS_H

/* LiteOS int 0x80 ABI: rax=number, rdi/rsi/rdx=arguments;
 * negative return values are -errno. */
#include <sys/liteos_numbers.h>

/* dirent64 layout: d_off is the index of the next entry. */
struct liteos_dirent64 {
    unsigned long long d_ino;
    long long d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};

long sys_read(int fd, void *buf, unsigned long n);
long sys_write(int fd, const void *buf, unsigned long n);
long sys_open(const char *path, int flags, int mode);
long sys_close(int fd);
long sys_pipe(int fds[2]);
long sys_dup2(int oldfd, int newfd);
long sys_getpid(void);
long sys_fork(void);
long sys_execve(const char *path, char *const argv[], char *const envp[]);
long sys_wait4(int pid, int *status);
long sys_getcwd(char *buf, unsigned long size);
long sys_chdir(const char *path);
long sys_mkdir(const char *path, int mode);
long sys_unlink(const char *path);
long sys_getdents64(int fd, void *buf, unsigned long n);
long sys_yield(void);
void sys_exit(int code) __attribute__((noreturn));

#endif
