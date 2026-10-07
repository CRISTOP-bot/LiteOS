# Ports en LiteOS

Ningún port está aún compilado contra LiteOS. La infraestructura está
lista: upstream como submódulo, y reglas futuras en el Makefile raíz que:

1. Compilan con la toolchain de `toolchain/toolchain.mk`.
2. Aplican parches de `ports/<nombre>/patches/`.
3. Instalan binarios/headers en `sysroot/`.

Orden previsto, condicionado a syscalls/libc reales: **BusyBox**, TCC,
Lua y Nano. Ninguno se declara portado hasta compilar y ejecutarse en LiteOS.
GCC como port avanzado: explícitamente fuera del alcance inicial.
