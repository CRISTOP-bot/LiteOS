#ifndef LITEOS_SYS_NUMBERS_H
#define LITEOS_SYS_NUMBERS_H

/*
 * Stable LiteOS x86-64 syscall numbers. These are LiteOS ABI assignments;
 * their historical numeric overlap with another OS is not a compatibility
 * guarantee. Keep this header as the single source shared by the kernel,
 * libc and user entry assembly.
 */
#define SYS_read        0
#define SYS_write       1
#define SYS_open        2
#define SYS_close       3
#define SYS_pipe       22
#define SYS_yield      24
#define SYS_dup2       33
#define SYS_getpid     39
#define SYS_fork       57
#define SYS_execve     59
#define SYS_exit       60
#define SYS_wait4      61
#define SYS_getcwd     79
#define SYS_chdir      80
#define SYS_mkdir      83
#define SYS_unlink     87
#define SYS_getdents64 217

#endif
