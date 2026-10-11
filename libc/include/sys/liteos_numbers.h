#ifndef LITEOS_SYS_NUMBERS_H
#define LITEOS_SYS_NUMBERS_H

#define LITEOS_ABI_VERSION 1

/*
 * Stable LiteOS x86-64 syscall numbers. The 0x4c00 namespace is deliberately
 * separate from Linux x86-64 syscall numbering. Keep this as the single
 * source shared by the kernel dispatcher and user ABI headers/assembly.
 */
#define SYS_read        0x4c01
#define SYS_write       0x4c02
#define SYS_open        0x4c03
#define SYS_close       0x4c04
#define SYS_pipe        0x4c05
#define SYS_yield       0x4c06
#define SYS_dup2        0x4c07
#define SYS_getpid      0x4c08
#define SYS_fork        0x4c09
#define SYS_execve      0x4c0a
#define SYS_exit        0x4c0b
#define SYS_wait4       0x4c0c
#define SYS_getcwd      0x4c0d
#define SYS_chdir       0x4c0e
#define SYS_mkdir       0x4c0f
#define SYS_unlink      0x4c10
#define SYS_getdents64  0x4c11

#endif
