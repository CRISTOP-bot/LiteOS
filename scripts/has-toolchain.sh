#!/bin/sh
# Quietly report whether PATH contains a usable target toolchain.
set -eu
target=${1:-x86_64-elf}
for tool in gcc as ld ar objcopy strip; do
    command -v "$target-$tool" >/dev/null 2>&1 || exit 1
done
cc=$(command -v "$target-gcc")
[ "$($cc -dumpmachine 2>/dev/null)" = "$target" ] || exit 1
"$target-as" --version >/dev/null 2>&1 || exit 1
"$target-ld" --version >/dev/null 2>&1 || exit 1
"$target-ar" --version >/dev/null 2>&1 || exit 1
"$target-objcopy" --version >/dev/null 2>&1 || exit 1
"$target-strip" --version >/dev/null 2>&1 || exit 1
check_dir=$(mktemp -d "${TMPDIR:-/tmp}/liteos-cross-check.XXXXXX") || exit 1
cleanup()
{
    result=$?
    trap - EXIT
    rm -rf "$check_dir"
    exit "$result"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
cat > "$check_dir/check.c" <<'EOF_C'
void _start(void) { for (;;) { } }
EOF_C
"$cc" -ffreestanding -fno-pie -c "$check_dir/check.c" -o "$check_dir/check.o" >/dev/null 2>&1 || exit 1
"$target-ld" -nostdlib -static -e _start "$check_dir/check.o" -o "$check_dir/check.elf" >/dev/null 2>&1 || exit 1
