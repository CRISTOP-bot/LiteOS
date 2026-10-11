#!/bin/sh
# Build a pinned, real x86_64-elf cross compiler from GNU source releases.
set -eu

TARGET=${1:-x86_64-elf}
if [ "$#" -ge 2 ]; then
    PREFIX=$2
else
    PREFIX=build/toolchain
fi
JOBS=${JOBS:-}

if [ "$TARGET" != x86_64-elf ]; then
    echo "ERROR: bootstrap supports only x86_64-elf (TARGET=$TARGET)." >&2
    exit 2
fi
case "$PREFIX" in
    '') ;;
    /*) ;;
    *) PREFIX="$(pwd)/$PREFIX" ;;
esac

if [ -z "$JOBS" ]; then
    JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)
fi
case "$JOBS" in
    ''|*[!0-9]*|0) echo 'ERROR: JOBS must be a positive integer.' >&2; exit 2 ;;
esac

BINUTILS_VERSION=2.46.1
BINUTILS_FILE="binutils-$BINUTILS_VERSION.tar.xz"
BINUTILS_SHA256=e127a709cba24c76de8936cb7083dd768f28cd37eb010492e2f19b71eb1294e4
GCC_VERSION=14.2.0
GCC_FILE="gcc-$GCC_VERSION.tar.xz"
GCC_SHA256=a7b39bc69cbf9e25826c5a60ab26477001f7c08d85cec04bc0e29cabed6f3cc9

resolve_tool()
{
    name=$1
    if [ -n "$PREFIX" ]; then
        [ -x "$PREFIX/bin/$TARGET-$name" ] || return 1
        printf '%s\n' "$PREFIX/bin/$TARGET-$name"
    else
        command -v "$TARGET-$name" 2>/dev/null || return 1
    fi
}

validate_existing()
{
    cc=$(resolve_tool gcc) || return 1
    assembler=$(resolve_tool as) || return 1
    linker=$(resolve_tool ld) || return 1
    archiver=$(resolve_tool ar) || return 1
    objcopy=$(resolve_tool objcopy) || return 1
    strip=$(resolve_tool strip) || return 1
    [ "$($cc -dumpmachine 2>/dev/null)" = "$TARGET" ] || return 1
    "$assembler" --version >/dev/null 2>&1 || return 1
    "$linker" --version >/dev/null 2>&1 || return 1
    "$archiver" --version >/dev/null 2>&1 || return 1
    "$objcopy" --version >/dev/null 2>&1 || return 1
    "$strip" --version >/dev/null 2>&1 || return 1

    check_dir=$(mktemp -d "${TMPDIR:-/tmp}/liteos-cross-check.XXXXXX") || return 1
    cat > "$check_dir/check.c" <<'EOF'
void _start(void) { for (;;) { } }
EOF
    if ! "$cc" -ffreestanding -fno-pie -c "$check_dir/check.c" -o "$check_dir/check.o" >/dev/null 2>&1 ||
       ! "$linker" -nostdlib -static -e _start "$check_dir/check.o" -o "$check_dir/check.elf" >/dev/null 2>&1; then
        rm -rf "$check_dir"
        return 1
    fi
    rm -rf "$check_dir"
    return 0
}

if validate_existing; then
    if [ -n "$PREFIX" ]; then
        echo "Valid x86_64-elf cross-toolchain already available; reusing $PREFIX."
    else
        echo 'Valid x86_64-elf cross-toolchain already available on PATH; reusing it.'
    fi
    exit 0
fi

if [ -z "$PREFIX" ]; then
    PREFIX="$(pwd)/build/toolchain"
fi

if ! command -v gcc >/dev/null 2>&1 || ! command -v make >/dev/null 2>&1; then
    echo 'ERROR: host GCC and GNU Make are required to build the cross-toolchain.' >&2
    echo 'Run make toolchain to install supported host dependencies, or install them manually.' >&2
    exit 1
fi
if ! make --version 2>/dev/null | grep -q 'GNU Make'; then
    if command -v gmake >/dev/null 2>&1; then
        make_cmd=gmake
    else
        echo 'ERROR: GNU Make is required (use gmake on macOS); BSD make is unsupported.' >&2
        exit 1
    fi
else
    make_cmd=make
fi
if ! command -v curl >/dev/null 2>&1 && ! command -v wget >/dev/null 2>&1; then
    echo 'ERROR: curl or wget is required to download the GNU sources.' >&2
    exit 1
fi
if ! command -v sha256sum >/dev/null 2>&1 && ! command -v shasum >/dev/null 2>&1; then
    echo 'ERROR: sha256sum or shasum is required to verify GNU sources.' >&2
    exit 1
fi

WORK=$(mktemp -d "${TMPDIR:-/tmp}/liteos-toolchain.XXXXXX")
cleanup()
{
    result=$?
    trap - EXIT
    rm -rf "$WORK"
    exit "$result"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
mkdir -p "$WORK/src" "$WORK/build-binutils" "$WORK/build-gcc" "$PREFIX/sysroot"

fetch()
{
    url=$1
    destination=$2
    if command -v curl >/dev/null 2>&1; then
        curl --fail --location --retry 3 --proto '=https' --tlsv1.2 \
            --output "$destination" "$url"
    else
        wget --https-only --tries=3 --output-document="$destination" "$url"
    fi
}

verify_sha256()
{
    expected=$1
    file=$2
    if command -v sha256sum >/dev/null 2>&1; then
        printf '%s  %s\n' "$expected" "$file" | sha256sum --check --status -
    else
        printf '%s  %s\n' "$expected" "$file" | shasum -a 256 --check --status -
    fi
}

fetch "https://ftp.gnu.org/gnu/binutils/$BINUTILS_FILE" "$WORK/src/$BINUTILS_FILE"
if ! verify_sha256 "$BINUTILS_SHA256" "$WORK/src/$BINUTILS_FILE"; then
    echo "ERROR: SHA-256 mismatch for $BINUTILS_FILE; refusing to extract it." >&2
    exit 1
fi
fetch "https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/$GCC_FILE" "$WORK/src/$GCC_FILE"
if ! verify_sha256 "$GCC_SHA256" "$WORK/src/$GCC_FILE"; then
    echo "ERROR: SHA-256 mismatch for $GCC_FILE; refusing to extract it." >&2
    exit 1
fi

printf '[toolchain] Building Binutils %s for %s\n' "$BINUTILS_VERSION" "$TARGET"
tar -xf "$WORK/src/$BINUTILS_FILE" -C "$WORK/src"
(
    cd "$WORK/build-binutils"
    "$WORK/src/binutils-$BINUTILS_VERSION/configure" \
        --target="$TARGET" --prefix="$PREFIX" --with-sysroot="$PREFIX/sysroot" \
        --disable-nls --disable-werror --disable-gprofng --disable-gdb
    "$make_cmd" -j"$JOBS"
    "$make_cmd" install
)

printf '[toolchain] Building GCC %s (C only; no target libc)\n' "$GCC_VERSION"
tar -xf "$WORK/src/$GCC_FILE" -C "$WORK/src"
(
    cd "$WORK/build-gcc"
    "$WORK/src/gcc-$GCC_VERSION/configure" \
        --target="$TARGET" --prefix="$PREFIX" --with-sysroot="$PREFIX/sysroot" \
        --without-headers --with-newlib --enable-languages=c --disable-nls \
        --disable-shared --disable-threads --disable-multilib --disable-libssp \
        --disable-libquadmath --disable-libgomp --disable-libatomic
    "$make_cmd" -j"$JOBS" all-gcc all-target-libgcc
    "$make_cmd" install-gcc install-target-libgcc
)

if ! validate_existing; then
    echo 'ERROR: installed cross-toolchain failed target compile/link validation.' >&2
    exit 1
fi
printf '%s\n' \
    "target=$TARGET" \
    "binutils=$BINUTILS_VERSION sha256=$BINUTILS_SHA256" \
    "gcc=$GCC_VERSION sha256=$GCC_SHA256" > "$PREFIX/.liteos-toolchain"
echo "Real cross-toolchain built and validated: $PREFIX"
