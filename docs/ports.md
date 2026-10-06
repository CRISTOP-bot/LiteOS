# Ports en LiteOS

Ningún port está aún compilado contra LiteOS. La infraestructura está
lista: upstream como submódulo, y reglas futuras en el Makefile raíz que:

1. Compilan con la toolchain de `toolchain/toolchain.mk`.
2. Aplican parches de `ports/<nombre>/patches/`.
3. Instalan binarios/headers en `sysroot/`.

Port prioritario: **TCC** (compilador C dentro del propio LiteOS).
Luego: Lua (scripting), BusyBox (utilidades Unix), Nano (editor).
GCC como port avanzado: explícitamente fuera del alcance inicial.
