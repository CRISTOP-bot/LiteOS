#!/bin/sh
# Comprueba dependencias locales. No descarga ni instala software.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
if ! command -v make >/dev/null 2>&1; then
    echo 'ERROR: GNU Make no está instalado' >&2
    exit 1
fi
make --no-print-directory -s -C "$root" toolchain-check \
    TARGET="${TARGET:-x86_64-elf}" \
    TOOLCHAIN_MODE="${TOOLCHAIN_MODE:-cross}" \
    TOOLCHAIN_ROOT="${TOOLCHAIN_ROOT:-}"

for cmd in make python3 grub-mkrescue xorriso mformat qemu-system-x86_64; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "ERROR: dependencia de build/QEMU ausente: $cmd" >&2
        exit 1
    fi
done
printf '%s\n' 'Entorno LiteOS OK (sin descargas ni modificaciones).'
