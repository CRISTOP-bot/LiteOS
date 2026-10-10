# Ports en LiteOS

La infraestructura permite construir el kernel y su espacio de usuario mínimo, pero **ningún port de terceros está aún compilado y ejecutado contra LiteOS**. Existe un andamiaje auxiliar en `apps/Makefile.apps` que menciona BusyBox, Lua, TCC y Nano, pero no define sus recetas de compilación, no se incluye desde el `Makefile` principal y no integra esos programas en el initramfs. Su selección de compilador host y objetivo `x86_64-linux-gnu` tampoco constituye el uso de la toolchain real de LiteOS. BusyBox no tiene configuración ni pruebas de ejecución.

La auditoría del ABI y los bloqueos verificables de BusyBox están en [busybox-port-audit.md](busybox-port-audit.md). En resumen, la libc es un subconjunto propio, el cargador solo acepta ELF64 estáticos `ET_EXEC`, y faltan interfaces POSIX importantes. Por eso no se habilitaron applets ni se presentaron declaraciones o stubs como implementación.

El siguiente trabajo debe ampliar y probar primero las interfaces reales de kernel/libc requeridas; después podrá incorporarse una configuración mínima de BusyBox y verificarse bajo QEMU. TCC, Lua y Nano siguen igualmente pendientes y condicionados a sus dependencias reales.
