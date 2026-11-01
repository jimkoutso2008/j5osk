#!/bin/bash
# Build j5osk.exe for Windows ARM64 with Debian's clang/lld, no C runtime.
# Run in WSL Debian:  bash build.sh   (needs clang lld llvm mingw-w64-common).
# Output goes to ./out.
set -e
SRC="$(cd "$(dirname "$0")" && pwd)"
B=~/.cache/j5osk-build
mkdir -p "$B" "$SRC/out" && cd "$B"

# --- import libraries
printf 'LIBRARY user32.dll\nEXPORTS\nBeginPaint\nCreateWindowExW\nDefWindowProcW\nDispatchMessageW\nDrawTextW\nEndPaint\nFillRect\nGetClientRect\nGetMessageW\nGetSystemMetrics\nInvalidateRect\nLoadCursorW\nPostQuitMessage\nPtInRect\nRegisterClassW\nSendInput\nTranslateMessage\n' > user32.def
printf 'LIBRARY gdi32.dll\nEXPORTS\nCreateFontW\nCreateSolidBrush\nDeleteObject\nSelectObject\nSetBkMode\nSetTextColor\n' > gdi32.def
printf 'LIBRARY kernel32.dll\nEXPORTS\nExitProcess\nGetModuleHandleW\n' > kernel32.def
printf 'LIBRARY ntdll.dll\nEXPORTS\nmemset\n' > ntdll.def
for l in user32 gdi32 kernel32 ntdll; do llvm-dlltool -m arm64 -d $l.def -l $l.lib; done

RES=$(clang -print-resource-dir)/include
clang --target=aarch64-w64-mingw32 -O2 -ffreestanding -fno-builtin -fno-stack-protector -fno-asynchronous-unwind-tables -nostdinc \
  -isystem $RES -isystem /usr/share/mingw-w64/include -D_WIN32_WINNT=0x0A00 -DUNICODE -Wall -c "$SRC/j5osk.c" -o j5osk.o
lld-link /nologo /subsystem:windows,10.0 /osversion:10.0 /entry:WinMainCRTStartup /machine:arm64 /nodefaultlib \
  /out:"$SRC/out/j5osk.exe" j5osk.o user32.lib gdi32.lib kernel32.lib ntdll.lib
ls -la "$SRC/out/j5osk.exe"
