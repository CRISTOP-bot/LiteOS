#!/bin/bash
set -e

ROOTFS_DIR="${1:-rootfs}"
APP_ROOT="${2:-.}"

mkdir -p "$ROOTFS_DIR"/bin "$ROOTFS_DIR"/sbin "$ROOTFS_DIR"/usr/bin "$ROOTFS_DIR"/usr/sbin "$ROOTFS_DIR"/lib "$ROOTFS_DIR"/etc "$ROOTFS_DIR"/tmp "$ROOTFS_DIR"/dev "$ROOTFS_DIR"/proc "$ROOTFS_DIR"/sys "$ROOTFS_DIR"/home "$ROOTFS_DIR"/root "$ROOTFS_DIR"/var/log

# Copy BusyBox install tree if present
if [ -d "$APP_ROOT/rootfs/busybox-install" ]; then
    cp -a "$APP_ROOT/rootfs/busybox-install/." "$ROOTFS_DIR"/
fi

# Copy Lua runtime if present
if [ -d "$APP_ROOT/rootfs/lua-install" ]; then
    mkdir -p "$ROOTFS_DIR"/usr/share/lua
    cp -a "$APP_ROOT/rootfs/lua-install/bin/." "$ROOTFS_DIR"/usr/bin/ 2>/dev/null || true
    cp -a "$APP_ROOT/rootfs/lua-install/lib/." "$ROOTFS_DIR"/usr/lib/ 2>/dev/null || true
    cp -a "$APP_ROOT/rootfs/lua-install/share/." "$ROOTFS_DIR"/usr/share/ 2>/dev/null || true
fi

# Copy TCC if present
if [ -d "$APP_ROOT/rootfs/tcc-install" ]; then
    cp -a "$APP_ROOT/rootfs/tcc-install/bin/." "$ROOTFS_DIR"/usr/bin/ 2>/dev/null || true
    cp -a "$APP_ROOT/rootfs/tcc-install/lib/." "$ROOTFS_DIR"/usr/lib/ 2>/dev/null || true
    cp -a "$APP_ROOT/rootfs/tcc-install/include/." "$ROOTFS_DIR"/usr/include/ 2>/dev/null || true
fi

# Copy Nano if present
if [ -d "$APP_ROOT/rootfs/nano-install" ]; then
    cp -a "$APP_ROOT/rootfs/nano-install/bin/." "$ROOTFS_DIR"/usr/bin/ 2>/dev/null || true
    cp -a "$APP_ROOT/rootfs/nano-install/share/." "$ROOTFS_DIR"/usr/share/ 2>/dev/null || true
fi

cat > "$ROOTFS_DIR"/init <<'EOF'
#!/bin/sh
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mkdir -p /dev/pts
mount -t devpts devpts /dev/pts

/bin/sh
EOF

chmod +x "$ROOTFS_DIR"/init

echo "[+] Rootfs created in $ROOTFS_DIR"

