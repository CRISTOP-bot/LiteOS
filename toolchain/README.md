# Toolchain de LiteOS

LiteOS no construye GCC desde cero. La infraestructura abstrae la
toolchain a través de `toolchain.mk`:

| Variable        | Descripción                              |
|-----------------|------------------------------------------|
| `TARGET`        | Triple objetivo (por defecto `x86_64-elf`) |
| `TOOLCHAIN_ROOT`| Prefijo de instalación de la toolchain   |
| `CC/AS/LD/AR/OBJCOPY/OBJDUMP` | Binarios derivados |

## Comportamiento por defecto

Si no existe `$(TOOLCHAIN_ROOT)/bin/$(TARGET)-gcc`, se usa el GCC del
host (`gcc`, `as`, `ld`, ...) con flags freestanding:
`-ffreestanding -nostdlib -mno-red-zone -mgeneral-regs-only -mcmodel=large`.
Esto es suficiente para el kernel y la libc inicial.

## Usar una toolchain cruzada real

Instalar una toolchain `x86_64-elf-gcc` (por ejemplo, construyéndola una
sola vez con crosstool-NG/musl-cross, o descargando un binario) y:

```sh
make TOOLCHAIN_ROOT=/opt/x86_64-elf
```

La ISO y todo el flujo (`run`, `test`, `debug`) funcionan igual.
