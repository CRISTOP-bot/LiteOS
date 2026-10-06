# LiteOS

LiteOS es un sistema operativo Unix-like de 64 bits, monolítico pero
modular, desarrollado desde cero para x86_64. El repositorio es compacto:
el código propio vive en `src/` (kernel) y `libc/` (libc propia), y el
software de terceros se integra como submódulos en `ports/`.

## Estado actual

Etapa 1-4 completadas y verificables:

- Kernel mínimo que arranca vía GRUB (multiboot2) en QEMU, cambia a
  long mode x86_64, instala GDT/IDT básica y reporta por el puerto serie.
- Tests automatizados: `make test` arranca QEMU y exige `BOOT OK` por el
  puerto serie con código de salida del dispositivo `isa-debug-exit`.
- libc propia inicial (`string.h`) construida como `libliteosc.a` e
  instalada en `sysroot/`.
- Submódulos de ports listos: busybox, tcc, lua, nano.

Todavía **no** implementado (se irá añadiendo por etapas): scheduler,
procesos de usuario, ELF loader, syscalls, VFS, /bin/sh.

## Estructura

```
LiteOS/
├── src/           # kernel
│   └── arch/x86_64/
├── libc/          # libc propia
├── ports/         # software externo (submódulos)
├── toolchain/     # configuración/manifest de la toolchain externa
├── sysroot/       # filesystem de desarrollo
├── scripts/       # automatización (bootstrap, QEMU, toolchain)
├── tests/         # pruebas
├── docs/          # documentación
├── linker.ld      # script de enlace del kernel
└── grub.cfg       # configuración del bootloader
```

## Dependencias

- GNU Make, GCC (o `x86_64-elf-gcc`), binutils, xorriso, GRUB (`grub-mkrescue`), mtools, QEMU.
- Arch Linux (pacman): `make gcc binutils xorriso grub mtools qemu-full`
- Debian/Ubuntu (apt): `make gcc binutils xorriso grub-pc-bin grub-common mtools qemu-system-x86`

## Toolchain

Por defecto se usa el GCC del host en modo freestanding. Si existe una
toolchain cruzada instalada, se detecta automáticamente en
`toolchain/toolchain.mk` o vía:

```sh
make TOOLCHAIN_ROOT=/opt/x86_64-elf
```

Ver `toolchain/README.md` para más detalle. No se construye GCC desde cero.

## Compilar y ejecutar

```sh
git submodule update --init --recursive
make            # kernel + libc + sysroot + iso
make run        # arranca QEMU con la ISO
make test       # verificación automatizada de boot
make debug      # QEMU esperando gdb (puerto 1234)
make clean
```

`make` produce `build/kernel.elf` y `build/liteos.iso`.

## Crear un port

1. Añadir el upstream como submódulo: `git submodule add <url> ports/<nombre>`
2. Crear `ports/<nombre>/` con README (versión, estado), `patches/` y reglas.
3. El port compila con la toolchain de `toolchain/toolchain.mk` y se
   instala en `sysroot/`.

## Contribuir

Cada etapa debe dejar el árbol compilable y arrancable. Los cambios de
arquitectura deben mantener el repositorio compacto: sin carpetas
genéricas innecesarias, sin código de terceros copiado (usar submódulos +
patches). Ver `docs/`.
