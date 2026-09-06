#!/usr/bin/env bash
# Wrapper Unix pour dist/gen_qr.py — utile si "python" n'est pas python3
# sur la machine et que le shebang du .py n'est pas exécutable (Windows -> WSL,
# archive dézippée sans le +x, etc.).
#
# Usage identique au script Python :
#     dist/gen_qr.sh https://mon.url/installer.cia --out dist/qr.html
#     dist/gen_qr.sh --selftest
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec python3 "$SCRIPT_DIR/gen_qr.py" "$@"
