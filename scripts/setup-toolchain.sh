#!/bin/sh
# Verifica la toolchain disponible y guía el setup.
set -eu
TARGET=${TARGET:-x86_64-elf}
echo "Target: $TARGET"
if command -v $TARGET-gcc >/dev/null; then
    echo "OK: $TARGET-gcc encontrado: $(command -v $TARGET-gcc)"
elif command -v gcc >/dev/null; then
    echo "AVISO: no hay $TARGET-gcc; se usará el GCC del host en modo freestanding."
    echo "Para usar una toolchain cruzada: make TOOLCHAIN_ROOT=<prefijo>"
else
    echo "ERROR: no se encontró ningún compilador GCC."
    exit 1
fi
echo "Checking dependencias de build..."
for cmd in make ld objcopy grub-mkrescue xorriso mformat qemu-system-x86_64; do
    if command -v "$cmd" >/dev/null; then echo "  ok: $cmd"; else echo "  FALTA: $cmd"; fi
done
