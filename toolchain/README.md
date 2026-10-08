# Toolchain de LiteOS

La compilación normal exige una cross-toolchain **x86_64-elf** ya instalada.
LiteOS no descarga artefactos, no compila GCC/Binutils y no cambia de
compilador silenciosamente. No hay una cross-toolchain vendorizada en este
repositorio.

```sh
# Si los ejecutables x86_64-elf-* están en PATH:
./scripts/setup-toolchain.sh
make

# O si están bajo un prefijo (con bin/x86_64-elf-gcc, bin/x86_64-elf-ld, etc.):
make TOOLCHAIN_ROOT=/opt/x86_64-elf toolchain-check
make TOOLCHAIN_ROOT=/opt/x86_64-elf test
```

`make toolchain-check` comprueba CC, AS, LD, AR, OBJCOPY y STRIP, y verifica
que `CC -dumpmachine` coincide con `TARGET`. GCC usa el ensamblador target
para compilar C y `.S`; `LD` enlaza el kernel, `AR` crea la libc, `OBJCOPY`
embebe el initramfs y `STRIP` procesa los binarios de usuario. `OBJDUMP` y
`READELF` son opcionales para inspección manual, no requisitos del build.
Un prefijo incorrecto o una herramienta requerida ausente detienen la
compilación con un error explícito. `./scripts/setup-toolchain.sh` también
comprueba las utilidades de ISO/QEMU.

| Variable | Valor por defecto | Propósito |
| --- | --- | --- |
| `TARGET` | `x86_64-elf` | Triple de destino |
| `TOOLCHAIN_MODE` | `cross` | `cross` o `host` (opt-in) |
| `TOOLCHAIN_ROOT` | vacío | Prefijo opcional; si está vacío se busca en `PATH` |
| `CC AS LD AR OBJCOPY STRIP` | según modo y target | Herramientas requeridas para el build |

**Solo para pruebas locales** sin cross-toolchain puede usarse:

```sh
TOOLCHAIN_MODE=host ./scripts/setup-toolchain.sh
make TOOLCHAIN_MODE=host test
```

Esto usa GCC/binutils nativos en modo freestanding; **no** certifica una
build cross. Los objetos target se compilan con `-nostdinc` y headers propios,
sin reutilizar headers del sistema anfitrión. El sello
`build/.toolchain-config` fuerza recompilar al cambiar modo, flags o
herramientas; no mezcla objetos de toolchains diferentes.

Los submódulos están fijados por sus gitlinks (SHA), pero todavía no se ha
fijado ni distribuido un binario cross con versión y checksum verificados.
Por ello, las builds entre distintas máquinas **no** garantizan resultados
binarios idénticos. Hay que elegir/verificar un artefacto cross concreto
antes de declarar esa propiedad; no se inventan hashes ni se descarga
`latest` de forma implícita.
