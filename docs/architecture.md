# Arquitectura de LiteOS

## Kernel (`src/`)

- `src/kernel.c`: punto de entrada del kernel tras el bootstrap.
- `src/arch/x86_64/boot.S`: entrada multiboot2, tablas de páginas
  (identidad de las primeras 16 MiB con páginas de 2 MiB), transición a
  long mode, salto a 64 bits.
- `src/arch/x86_64/idt.c` + `isrs.S`: IDT básica con excepciones CPU que
  reportan el vector por el puerto serie y abortan (fase temprana).
- `src/arch/x86_64/serial.c`: consola por COM1 (115200 8N1).

El código dependiente de arquitectura vive bajo `src/arch/<arch>/` para
que el código genérico no dependa de x86_64. Inicialmente solo se soporta
x86_64; aarch64/riscv64 se podrán añadir como `src/arch/aarch64/` y
`src/arch/riscv64/` sin reescribir el resto.

## Flujo de arranque

GRUB (multiboot2) -> `_start` (32 bits, protegido) -> paging + EFER.LME
-> `kernel_main` (64 bits) -> IDT/GDT, memoria y VFS -> initramfs ->
proceso inicial `/bin/sh` en ring 3 -> prompt interactivo.

## ABI / syscalls

El ejecutable estático `/bin/sh` se incluye en un initramfs CPIO `newc`
enlazado al kernel. Se enlaza en `0x10000001000`, fuera del mapa identidad
supervisor del kernel; el loader carga segmentos ELF64 en un espacio de
usuario independiente. Los procesos invocan `int 0x80`, con número de
syscall en `rax`, argumentos en `rdi`, `rsi`, `rdx` y valor de retorno en
`rax` (errores negativos `-errno`). Los números públicos están en
`libc/include/sys/liteos.h`.

No se ejecuta la shell en ring 0: la TTY, el VFS ramfs y los pipes son
servicios del kernel; `/bin/sh` y `/bin/hello` son ELF de usuario. La
shell mínima no pretende implementar POSIX ni ejecutar BusyBox todavía.
