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
-> `kernel_main` (64 bits) -> GDT/IDT -> BOOT OK -> `isa-debug-exit`.

## ABI / syscalls

Pendiente de definir con la capa de syscalls (etapa 8). En la ABI
inicial, cuando se implemente, se utilizará la convención `syscall` de
x86_64 (rax = número, rdi/rsi/rdx/r10/r8/r9 = argumentos) propia de LiteOS.
