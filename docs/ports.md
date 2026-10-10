# Ports en LiteOS

La infraestructura permite construir el kernel y su espacio de usuario mínimo, pero **ningún port de terceros está aún compilado y ejecutado contra LiteOS**. BusyBox está fijado como submódulo en `ports/busybox`; todavía no tiene reglas de build, configuración, instalación al initramfs ni pruebas de ejecución.

La auditoría del ABI y los bloqueos verificables de BusyBox están en [busybox-port-audit.md](busybox-port-audit.md). En resumen, la libc es un subconjunto propio, el cargador solo acepta ELF64 estáticos `ET_EXEC`, y faltan interfaces POSIX importantes. Por eso no se habilitaron applets ni se presentaron declaraciones o stubs como implementación.

El siguiente trabajo debe ampliar y probar primero las interfaces reales de kernel/libc requeridas; después podrá incorporarse una configuración mínima de BusyBox y verificarse bajo QEMU. TCC, Lua y Nano siguen igualmente pendientes y condicionados a sus dependencias reales.
