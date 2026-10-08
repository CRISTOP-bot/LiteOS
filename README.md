# LiteOS

LiteOS es un sistema operativo Unix-like de 64 bits, monolítico pero
modular, desarrollado desde cero para x86_64. El repositorio es compacto:
el código propio vive en `src/` (kernel y programas propios) y `libc/`;
el software de terceros se integra como submódulos en `ports/`.

## Estado actual

LiteOS arranca con GRUB/Multiboot2 en QEMU y carga una imagen CPIO
embebida que contiene `/bin/sh` y `/bin/hello`. `/bin/sh` es un programa
ELF estático en **espacio de usuario**, no una consola de comandos del kernel.
Usa la TTY, ramfs y syscalls `int 0x80` para ejecutar procesos.

La shell permite `cd`, `pwd`, `ls`, `cat`, `echo`, `mkdir`, `rm`, `exit` y
`help`, programas externos en `/bin`, argumentos con comillas, pipes,
redirecciones `<`, `>` y `>>`. Es una shell mínima propia, **no** BusyBox ni
una implementación POSIX completa: sin expansión de variables, globbing,
control de trabajos ni persistencia del ramfs tras reiniciar.

Los ports BusyBox, tcc, lua y nano siguen siendo submódulos upstream sin
integración con la libc de LiteOS.

## Estructura

```
LiteOS/
├── src/           # kernel y programas propios
│   ├── arch/x86_64/
│   └── user/       # shell y programas ELF estáticos
├── libc/          # libc propia
├── ports/         # software externo (submódulos)
├── toolchain/     # configuración/manifest de la toolchain externa
├── sysroot/       # filesystem de desarrollo
├── scripts/       # build, comprobaciones de toolchain y QEMU
├── tests/         # pruebas
├── docs/          # documentación
├── linker.ld      # script de enlace del kernel
└── grub.cfg       # configuración del bootloader
```

## Dependencias

- GNU Make, cross-toolchain `x86_64-elf` (GCC y binutils), Python 3, xorriso, GRUB (`grub-mkrescue`), QEMU. GCC/binutils host solo para `TOOLCHAIN_MODE=host`.
- Arch Linux (pacman), herramientas host: `make gcc binutils python xorriso grub mtools qemu-full`.
- Debian/Ubuntu (apt), herramientas host: `make gcc binutils python3 xorriso grub-pc-bin grub-common mtools qemu-system-x86`.
- Además, para la build normal se necesita una cross-toolchain `x86_64-elf` externa; esos paquetes host no la sustituyen.

## Toolchain

Por defecto se requiere una **cross-toolchain `x86_64-elf`** ya instalada
en `PATH` o mediante `TOOLCHAIN_ROOT`. No se descarga ni compila GCC
implícitamente; una herramienta ausente produce un error claro:

```sh
./scripts/setup-toolchain.sh
make TOOLCHAIN_ROOT=/opt/x86_64-elf  # si no está en PATH
```

Solo para pruebas locales, puede usarse GCC nativo de forma **explícita**;
esto no equivale a una build cross ni sustituye `x86_64-elf-gcc`:

```sh
make TOOLCHAIN_MODE=host test
```

Ver `toolchain/README.md` para variables, limitaciones y reproducibilidad.

## Compilar y ejecutar

```sh
git submodule update --init --recursive
make            # kernel + libc + /bin/sh + initramfs + ISO
make run        # QEMU: teclado PS/2 en ventana gráfica; salida también por serie
make test       # toolchain + libc/parser + procesos, VFS y shell en QEMU
make test-toolchain # comprueba errores y modo host sin requerir cross-toolchain
make test-libc  # prueba snprintf sin arrancar QEMU
make debug      # QEMU esperando gdb (puerto 1234)
make clean
```

Los submodules se fijan a commits concretos en los gitlinks del repositorio
(ver `git submodule status`), no a la punta de una rama.

`make` produce `build/kernel.elf`, `build/liteos.iso`, `build/user/bin/sh`
y `build/initramfs.cpio`. Al ejecutar `make run`, escribe `help`, `hello`,
`echo hola | cat` o `ls /bin` dentro de la ventana de QEMU. El puerto serie
es para salida de depuración; el teclado PS/2 alimenta la TTY.

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
