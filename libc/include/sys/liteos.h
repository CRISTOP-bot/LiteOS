#ifndef LITEOS_SYS_H
#define LITEOS_SYS_H

/* LiteOS int 0x80 ABI: rax=number, rdi/rsi/rdx=arguments;
 * negative return values are -errno. Numbers match x86_64 Linux where possible. */
#define SYS_read       0
#define SYS_write      1
#define SYS_open       2
#define SYS_close      3
#define SYS_pipe      22
#define SYS_dup2      33
#define SYS_getpid    39
#define SYS_fork      57
#define SYS_execve    59
#define SYS_exit      60
#define SYS_wait4     61
#define SYS_getcwd    79
#define SYS_chdir     80
#define SYS_mkdir     83
#define SYS_unlink    87
#define SYS_getdents64 217
#define SYS_yield     24

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
