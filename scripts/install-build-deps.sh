#!/bin/sh
# Install host prerequisites needed for the LiteOS build, toolchain and QEMU test.
set -eu

AUTO_INSTALL_DEPS=${AUTO_INSTALL_DEPS:-1}
required='gcc g++ bison flex gawk makeinfo python3 tar curl m4'
missing=''

for command in $required; do
    command -v "$command" >/dev/null 2>&1 || missing="$missing $command"
done
if command -v gmake >/dev/null 2>&1; then
    :
elif command -v make >/dev/null 2>&1 && make --version 2>/dev/null | grep -q 'GNU Make'; then
    :
else
    missing="$missing GNU_Make"
fi
for command in grub-mkrescue xorriso mformat qemu-system-x86_64; do
    command -v "$command" >/dev/null 2>&1 || missing="$missing $command"
done
if ! command -v sha256sum >/dev/null 2>&1 && ! command -v shasum >/dev/null 2>&1; then
    missing="$missing sha256sum_or_shasum"
fi

if [ -z "$missing" ]; then
    echo 'Build dependencies already available.'
    exit 0
fi

if [ "$AUTO_INSTALL_DEPS" = 0 ]; then
    echo "ERROR: missing build dependencies:$missing" >&2
    echo 'AUTO_INSTALL_DEPS=0: install the listed tools and retry.' >&2
    exit 1
fi

run_privileged()
{
    if [ "$(id -u)" -eq 0 ]; then
        "$@"
    elif command -v sudo >/dev/null 2>&1; then
        sudo "$@"
    else
        echo 'ERROR: administrator privileges are required; install dependencies manually or configure sudo.' >&2
        exit 1
    fi
}

if command -v apt-get >/dev/null 2>&1; then
    echo '[deps] Installing packages for Debian/Ubuntu.'
    run_privileged apt-get update
    run_privileged apt-get install -y \
        build-essential bison flex gawk texinfo python3 curl ca-certificates m4 \
        xorriso mtools grub-pc-bin qemu-system-x86 xz-utils \
        libgmp-dev libmpfr-dev libmpc-dev libisl-dev zlib1g-dev libzstd-dev
elif command -v dnf >/dev/null 2>&1; then
    echo '[deps] Installing packages for Fedora/RHEL.'
    run_privileged dnf install -y \
        gcc gcc-c++ make bison flex gawk texinfo python3 curl ca-certificates m4 \
        xorriso mtools grub2-tools-extra qemu-system-x86-core xz \
        gmp-devel mpfr-devel libmpc-devel isl-devel zlib-devel libzstd-devel
elif command -v pacman >/dev/null 2>&1; then
    echo '[deps] Installing packages for Arch Linux.'
    run_privileged pacman -S --needed --noconfirm \
        base-devel bison flex gawk texinfo python curl ca-certificates m4 \
        xorriso mtools grub qemu-system-x86 xz gmp mpfr libmpc isl zlib zstd
elif command -v brew >/dev/null 2>&1; then
    echo '[deps] Installing packages for Homebrew.'
    brew install gcc make bison flex gawk texinfo python xorriso mtools grub qemu xz curl m4 gmp mpfr libmpc isl zstd
    if ! command -v gmake >/dev/null 2>&1; then
        echo 'ERROR: Homebrew GNU Make (gmake) is not on PATH.' >&2
        echo 'GNU Make must be available before invoking this Makefile; add the Homebrew bin directory to PATH.' >&2
        exit 1
    fi
else
    echo "ERROR: missing build dependencies:$missing" >&2
    echo 'No supported package manager found (apt-get, dnf, pacman, or brew); no packages were installed.' >&2
    exit 1
fi

for command in $required grub-mkrescue xorriso mformat qemu-system-x86_64; do
    if [ "$command" = make ] && command -v gmake >/dev/null 2>&1; then
        continue
    fi
    if ! command -v "$command" >/dev/null 2>&1; then
        echo "ERROR: '$command' is still missing after package installation." >&2
        exit 1
    fi
done
if ! command -v gmake >/dev/null 2>&1 &&
   { ! command -v make >/dev/null 2>&1 || ! make --version 2>/dev/null | grep -q 'GNU Make'; }; then
    echo 'ERROR: GNU Make is missing; the BSD make shipped with macOS is not sufficient.' >&2
    exit 1
fi
if ! command -v sha256sum >/dev/null 2>&1 && ! command -v shasum >/dev/null 2>&1; then
    echo 'ERROR: sha256sum or shasum is missing.' >&2
    exit 1
fi

echo 'Build dependencies installed and verified.'
