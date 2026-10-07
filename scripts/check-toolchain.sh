#!/bin/sh
# Validate every tool before building; never install or download anything.
set -eu

if [ "$#" -ne 10 ]; then
    echo 'uso: check-toolchain.sh <target> <cross|host> <cc> <as> <ld> <ar> <objcopy> <objdump> <strip> <root>' >&2
    exit 2
fi

target=$1 mode=$2 cc=$3 assembler=$4 linker=$5 archiver=$6
objcopy=$7 objdump=$8 strip=$9
shift 9
root=$1

if [ "$mode" != cross ] && [ "$mode" != host ]; then
    echo "ERROR: TOOLCHAIN_MODE inválido: $mode" >&2
    exit 2
fi
if [ "$mode" = cross ] && [ -n "$root" ] && [ ! -d "$root/bin" ]; then
    echo "ERROR: TOOLCHAIN_ROOT no contiene bin/: $root" >&2
    exit 1
fi

for tool in "$cc" "$assembler" "$linker" "$archiver" "$objcopy" "$objdump" "$strip"; do
    if [ -z "$tool" ]; then
        echo 'ERROR: nombre de herramienta vacío' >&2
        exit 1
    fi
    case "$tool" in
        */*) if [ ! -x "$tool" ]; then
                 echo "ERROR: herramienta no ejecutable: $tool" >&2
                 exit 1
             fi ;;
        *) if ! command -v "$tool" >/dev/null 2>&1; then
               echo "ERROR: herramienta ausente: $tool (TARGET=$target, TOOLCHAIN_MODE=$mode)" >&2
               echo 'Configura TOOLCHAIN_ROOT=<prefijo> o instala una cross-toolchain; para pruebas explícitas: TOOLCHAIN_MODE=host.' >&2
               exit 1
           fi ;;
    esac
done

machine=$("$cc" -dumpmachine) || {
    echo "ERROR: $cc no es un compilador GCC utilizable" >&2
    exit 1
}
if [ "$mode" = cross ] && [ "$machine" != "$target" ]; then
    echo "ERROR: $cc produce $machine; se esperaba $target" >&2
    exit 1
fi
if [ "$mode" = host ]; then
    arch=${target%%-*}
    case "$machine" in
        "$arch"-*) ;;
        *) echo "ERROR: $cc produce $machine; se esperaba un compilador nativo $arch" >&2
           exit 1 ;;
    esac
    if [ "$machine" = "$target" ]; then
        echo "ERROR: TOOLCHAIN_MODE=host requiere un compilador nativo; usa cross para $target" >&2
        exit 1
    fi
fi

version=$("$cc" -dumpfullversion 2>/dev/null) || version=$("$cc" -dumpversion)
printf 'Toolchain OK: %s %s (%s, %s)\n' "$cc" "$version" "$machine" "$mode"
