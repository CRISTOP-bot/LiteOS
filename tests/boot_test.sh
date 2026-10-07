#!/bin/sh
# Compatibilidad con la entrada anterior del test de arranque.
set -eu
exec python3 "$(dirname "$0")/boot_test.py" "$@"
