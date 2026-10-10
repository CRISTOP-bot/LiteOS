# BusyBox en LiteOS: auditoría de preparación

## Alcance y base revisada

Esta auditoría describe el estado observado en LiteOS `master` en el commit `1543131feda53c703bdb5add9263253c814775b1` (10 de octubre de 2026). El árbol registra `ports/busybox` como submódulo Git fijado en `b829bba95095b87834628adb38ea14b5885c0fbc`, URL `https://git.busybox.net/busybox`. El `Makefile` del commit fijado declara BusyBox **1.39.0.git**. No se cambió ni se actualizó esa referencia.

El repositorio no contiene actualmente reglas de build o configuración BusyBox: `docs/ports.md` dice que ningún port se ha compilado aún. El submódulo no está materializado en la copia local de auditoría; la versión y el commit se comprobaron usando el puntero gitlink y el `Makefile` en el commit upstream fijado.

## Estado por dependencia

| Área | Estado | Evidencia y límite actual |
|---|---|---|
| Compilador / enlazador | Parcial | `toolchain/toolchain.mk` fija por defecto `x86_64-elf` y el build del kernel usa `-ffreestanding -nostdinc`; no hay objetivo BusyBox ni sysroot de libc POSIX. El build BusyBox no se ejecutó. |
| ELF / ABI de usuario | Parcial | `src/elf.c` valida ELF64 little-endian x86-64 `ET_EXEC`, segmentos dentro de rangos acotados; no hay cargador dinámico, PIE/`ET_DYN`, relocaciones ni intérprete. El arranque de usuario usa `int 0x80`; la libc expone retornos negativos `-errno`. |
| libc / cabeceras | Parcial | `libc/include` contiene solo un subconjunto de cabeceras; `libc/src` implementa wrappers LiteOS, `snprintf` reducido y funciones de cadena básicas. No es una libc POSIX compatible con BusyBox; no hay `errno` estándar que traduzca el ABI a `-1` y `errno`. |
| Syscalls | Parcial | `src/syscall.c` despacha `read`, `write`, `open`, `close`, `pipe`, `dup2`, `getpid`, `fork`, `execve`, `exit`, `wait4`, `getcwd`, `chdir`, `mkdir`, `unlink`, `getdents64` y `yield`. No están implementadas muchas interfaces usadas por applets, entre ellas `stat/fstat`, `lseek`, `rename`, `rmdir`, `ioctl`, `fcntl`, `mmap`, `uname` y las interfaces de señales/tiempo. |
| Errores | Parcial | El kernel devuelve códigos negativos numéricos de error. Los wrappers en `libc/src/liteos.c` devuelven el valor crudo; no se observó la semántica libc POSIX de `-1` más `errno`. |
| Procesos, argumentos y entorno | Parcial | Hay `fork/execve/wait4` y pila inicial con `argv`/`envp`; `src/syscall.c` limita cada vector a 16 cadenas de hasta 255 caracteres. No hay soporte general de sesiones, grupos de procesos o señales POSIX. |
| Descriptores y pipes | Parcial | Tabla acotada de 64 descriptores por proceso, `dup2` y pipes de kernel con buffer circular de 2048 bytes. No hay `fcntl`, `dup`, `poll/select` ni semántica completa de bloqueo/interrupción POSIX. |
| Archivos y directorios | Parcial | VFS ramfs y CPIO `newc`; hay lectura/escritura, creación de archivo con `O_CREAT`, `O_TRUNC`, `O_APPEND`, `mkdir`, `unlink` (solo archivos) y `getdents64`. `mode` no configura permisos; no hay `stat`, `rename`, `rmdir` ni almacenamiento persistente. |
| Terminal | Parcial | TTY básico con entrada canónica y eco, más `/dev/console`, `/dev/tty` y dispositivos simples. No hay `ioctl`/`termios`, control de trabajos ni modos POSIX de terminal. |
| Señales y tiempo | Ausente en ABI de usuario | Hay temporizador y algunos campos internos de proceso, pero no se encontraron syscalls para `sigaction`, `kill`, alarmas, `clock_gettime`, `nanosleep` u otras interfaces de tiempo requeridas por applets. `/dev/random` y `/dev/urandom` devuelven `ENOSYS` en lectura. |
| Kconfig de BusyBox | Ausente | No hay configuración BusyBox específica de LiteOS ni `.config` comprometido que seleccione applets compatibles. |
| Integración al rootfs | Parcial | `scripts/mkinitramfs.py` y el arranque incorporan `/bin/sh` y `/bin/hello`; no existe instalación BusyBox ni ejecución de `busybox` desde el shell actual. |
| Compilación / QEMU | No verificado | Este entorno no tiene `git`, `make`, `gcc`, `qemu-system-x86_64` ni `x86_64-elf-gcc` disponibles. No se afirmó build ni ejecución. |

## Consecuencia

No es seguro activar applets de BusyBox ni declarar un port funcional con el ABI/libc actuales. En particular, compilar contra cabeceras del host o añadir stubs para las funciones ausentes produciría un binario engañoso o incompatible, y no se hizo.

## Secuencia recomendada para continuar

1. Materializar y verificar el submódulo en un checkout con `git`; conservar su SHA fijado.
2. Definir el contrato libc/syscall soportado por LiteOS y añadir pruebas de kernel para cada syscall nueva. Como base, implementar y probar `lseek`, `stat/fstat`, `rename`, `rmdir`, `fcntl`, `ioctl`/TTY y la interfaz de errores de libc antes de seleccionar applets que las necesitan.
3. Añadir compatibilidad de tiempo/señales/proceso solo con implementaciones reales y pruebas, no con stubs.
4. Añadir una configuración BusyBox mínima revisada contra ese contrato y reglas reproducibles que usen únicamente la toolchain/sysroot reales.
5. Instalar al initramfs y probar ELF y applets bajo QEMU. Reportar build y ejecución como hitos distintos.

La lista de syscalls es una auditoría estática del dispatcher y las cabeceras, no una declaración de conformidad POSIX. Cada futura capacidad debe verificarse con pruebas de ejecución antes de habilitar applets dependientes.
