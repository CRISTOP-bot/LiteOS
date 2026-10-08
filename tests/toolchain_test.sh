#!/bin/sh
# Negative and positive checks without installing or downloading a compiler.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
temp_parent=${TMPDIR:-/tmp/opencode}
[ -d "$temp_parent" ] || temp_parent=/tmp
tmp=$(mktemp -d "$temp_parent/liteos-toolchain.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

if make --no-print-directory -s -C "$root" TOOLCHAIN_MODE=cross TOOLCHAIN_ROOT="$tmp/missing" toolchain-check >"$tmp/output" 2>&1; then
    echo 'FAIL: una toolchain ausente fue aceptada' >&2
    exit 1
fi
grep -q 'TOOLCHAIN_ROOT no contiene bin/' "$tmp/output"

make --no-print-directory -s -C "$root" TOOLCHAIN_MODE=host toolchain-check >"$tmp/output"
grep -q 'Toolchain OK:' "$tmp/output"

if make --no-print-directory -s -C "$root" TOOLCHAIN_MODE=host OBJCOPY="$tmp/inexistente" toolchain-check >"$tmp/output" 2>&1; then
    echo 'FAIL: una herramienta ausente fue aceptada' >&2
    exit 1
fi
grep -q 'herramienta no ejecutable' "$tmp/output"

if make --no-print-directory -s -C "$root" TARGET=invalid-elf TOOLCHAIN_MODE=cross \
    CC=gcc AS=as LD=ld AR=ar OBJCOPY=objcopy STRIP=strip \
    toolchain-check >"$tmp/output" 2>&1; then
    echo 'FAIL: GCC con triple incorrecto fue aceptado' >&2
    exit 1
fi
grep -q 'se esperaba invalid-elf' "$tmp/output"

if make --no-print-directory -s -C "$root" TOOLCHAIN_MODE=host TOOLCHAIN_ROOT="$tmp" toolchain-check >"$tmp/output" 2>&1; then
    echo 'FAIL: un prefijo cross fue aceptado en modo host' >&2
    exit 1
fi
grep -q 'TOOLCHAIN_ROOT solo se usa' "$tmp/output"

echo 'PASS: verificación de toolchain'
