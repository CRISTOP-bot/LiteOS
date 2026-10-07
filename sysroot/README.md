# Sysroot de desarrollo

`make sysroot` instala la libc y los programas de LiteOS en este árbol.
Los binarios y headers generados no se versionan; se regeneran desde sus
fuentes. Este sysroot es para la **compilación**, no un filesystem persistente
para QEMU: el arranque usa `build/initramfs.cpio`.

No copies aquí headers del sistema anfitrión para ocultar dependencias
faltantes de `libc/`. Esas APIs deben implementarse y probarse por fases.
