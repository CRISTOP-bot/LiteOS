#ifndef LITEOS_FCNTL_H
#define LITEOS_FCNTL_H

/* Flags implemented by LiteOS open(); values are part of its syscall ABI. */
#define O_RDONLY   0
#define O_WRONLY   1
#define O_RDWR     2
#define O_ACCMODE  3
#define O_CREAT    0100
#define O_TRUNC    01000
#define O_APPEND   02000

#endif
