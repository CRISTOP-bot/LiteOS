/* LiteOS: tipos base del kernel (sin dependencias de libc del host). */
#ifndef LITEOS_TYPES_H
#define LITEOS_TYPES_H

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef signed char        s8;
typedef signed short       s16;
typedef signed int         s32;
typedef signed long long   s64;
typedef unsigned long      usize;
typedef signed long        ssize_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#define LITEOS_VERSION "0.1.0"

/* Direcciones del espacio de usuario (ver docs/abi.md).
 * El kernel vive en identidad baja (PML4[0]); el espacio de usuario
 * ocupa PML4[2..255] para no compartir tablas de paginas con el
 * kernel: base de texto 1 TiB, pila hasta 128 TiB. */
#define USER_TEXT_BASE   0x10000000000ULL
#define USER_STACK_TOP   0x7FFFFFFFF000ULL
#define USER_BRK_BASE    (USER_TEXT_BASE + 0x40000000ULL)
#define USER_BRK_LIMIT   0x40000000000ULL
#define USER_MMAP_BASE   (USER_STACK_TOP - 0x4000000ULL)

/* Flags de pagina x86_64 */
#define PAGE_P     0x001
#define PAGE_RW    0x002
#define PAGE_U     0x004
#define PAGE_PS    0x080
#define PAGE_KERNEL (PAGE_P | PAGE_RW)
#define PAGE_USER   (PAGE_P | PAGE_RW | PAGE_U)

/* Selectores GDT (ver gdt.c) */
#define GDT_KCODE 0x08
#define GDT_KDATA 0x10
#define GDT_UCODE 0x1B
#define GDT_UDATA 0x23
#define GDT_TSS   0x28

#endif
