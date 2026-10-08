# Arquitectura de LiteOS

## Kernel (`src/`)

- `src/kernel.c`: punto de entrada del kernel tras el bootstrap.
- `src/arch/x86_64/boot.S`: entrada multiboot2, tablas de páginas
  (identidad de las primeras 16 MiB con páginas de 2 MiB), transición a
  long mode, salto a 64 bits.
- `src/arch/x86_64/idt.c`, `isrs.S`, `trap.c` y `gdt.c`: GDT/TSS,
  IDT, stubs y despacho de traps específicos de x86_64.
- `src/arch/x86_64/paging.c`, `pmm.c` y `heap.c`: memoria ligada al
  mapa de arranque Multiboot2 y al mapeo identidad actual.
- `src/drivers/serial/16550.c`: consola UART 16550A en COM1 (debug).
- `src/drivers/input/keyboard.c`, `src/drivers/video/vga.c` y
`src/drivers/rtc.c`:
  teclado PS/2, consola VGA texto y RTC CMOS.

`src/drivers/` agrupa implementaciones concretas de dispositivos. Sus accesos
por puerto aún dependen de las primitivas x86 de `kernel.h`; no son interfaces
portables entre arquitecturas.

El código dependiente de arquitectura vive bajo `src/arch/<arch>/` para
que el código genérico no dependa de x86_64. Inicialmente solo se soporta
x86_64; aarch64/riscv64 se podrán añadir como `src/arch/aarch64/` y
`src/arch/riscv64/` sin reescribir el resto.

### Criptografía (`src/crypto/`)

Las primitivas SHA-256 y SHA-512 son implementaciones genéricas de software
basadas en FIPS 180-4. Sus interfaces de una sola operación están en
`src/crypto.h`; no requieren memoria dinámica ni dependen de la arquitectura.
No hay todavía proveedor de entropía del kernel ni API RNG. Por seguridad,
las lecturas de `/dev/random` y `/dev/urandom` fallan con `ENOSYS`: el antiguo
xorshift sembrado con el RTC no era criptográficamente seguro y no se debe usar para
claves ni secretos. Las optimizaciones por arquitectura se añadirán solo junto
con una implementación real y pruebas.

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
