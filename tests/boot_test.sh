#!/bin/sh
# Verifica que LiteOS arranque en QEMU y reporte BOOT OK via serial,
# saliendo con codigo esperado del dispositivo isa-debug-exit.
set -u
ISO="${1:?uso: boot_test.sh <liteos.iso>}"
OUT=$(mktemp)
qemu-system-x86_64 -cdrom "$ISO" -serial file:"$OUT" -no-reboot \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04
RC=$?
cat "$OUT"
if grep -q "BOOT OK" "$OUT" && [ "$RC" -eq 33 ]; then
    echo "PASS: boot correcto (qemu rc=$RC)"
    rm -f "$OUT"
    exit 0
fi
echo "FAIL: qemu rc=$RC"
rm -f "$OUT"
exit 1
