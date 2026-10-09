#!/bin/bash
set -e

TARGET="${1:-x86_64-linux-gnu}"
TOOLCHAIN_URL="${2:-}"
TOOLCHAIN_DIR="${3:-toolchain}"

if [ -z "$TOOLCHAIN_URL" ]; then
    echo "Usage: $0 <target> <toolchain_url> [toolchain_dir]"
    echo ""
    echo "Example:"
    echo "  $0 x86_64-linux-gnu https://example.com/x86_64-toolchain.tar.gz"
    echo ""
    echo "Common prebuilt toolchains:"
    echo "  - Bootlin toolchains: https://toolchains.bootlin.com/"
    echo "  - Linaro releases: https://releases.linaro.org/components/toolchain/binaries/"
    exit 1
fi

mkdir -p "$TOOLCHAIN_DIR"

if command -v wget >/dev/null 2>&1; then
    wget -q -O /tmp/liteos-toolchain.tar.gz "$TOOLCHAIN_URL"
else
    curl -L --fail -o /tmp/liteos-toolchain.tar.gz "$TOOLCHAIN_URL"
fi

tar -xzf /tmp/liteos-toolchain.tar.gz -C "$TOOLCHAIN_DIR" --strip-components=1
rm -f /tmp/liteos-toolchain.tar.gz

echo "[+] Toolchain extracted to $TOOLCHAIN_DIR"

if [ -f "$TOOLCHAIN_DIR/bin/$TARGET-gcc" ]; then
    "$TOOLCHAIN_DIR/bin/$TARGET-gcc" --version | head -n 1
else
    echo "[!] Expected compiler not found: $TOOLCHAIN_DIR/bin/$TARGET-gcc"
    echo "    If your toolchain is already extracted, set TOOLCHAIN_DIR manually."
fi

