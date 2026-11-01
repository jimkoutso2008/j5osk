#!/bin/bash
# Add j5osk.exe to a WinPE boot.wim and start it from startnet.cmd
# (WSL Debian / Linux, needs wimtools):
#   bash install-bootwim.sh /path/to/sources/boot.wim [out/j5osk.exe]
set -e
WIM="$(realpath "$1")"
EXE="$(realpath "${2:-$(dirname "$0")/out/j5osk.exe}")"
[ -f "$WIM" ] && [ -f "$EXE" ] || { echo "usage: $0 boot.wim [j5osk.exe]"; exit 1; }

W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT
cd "$W"
wimextract "$WIM" 1 /Windows/System32/startnet.cmd --dest-dir=. --no-acls >/dev/null 2>&1 || printf 'wpeinit\r\n' > startnet.cmd
grep -qi 'j5osk' startnet.cmd || printf 'start "" X:\\Windows\\System32\\j5osk.exe\r\n' >> startnet.cmd
cp "$EXE" j5osk.exe
printf '%s\n' "add j5osk.exe /Windows/System32/j5osk.exe" \
              "add startnet.cmd /Windows/System32/startnet.cmd" | wimupdate "$WIM" 1
echo "startnet.cmd is now:"; cat startnet.cmd
