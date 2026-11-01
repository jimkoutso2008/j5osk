# j5osk

A minimal on-screen keyboard for **Windows PE on ARM64**. It was made for the
Galaxy J5 2016 port ([edk2-msm8916-j5xlte](https://github.com/jimkoutso2008/edk2-msm8916-j5xlte),
with touch from [j5touch](https://github.com/jimkoutso2008/j5touch)), but it only
uses plain Win32, so it works with any pointer device.

WinPE ships no `osk.exe`, and TabTip needs `TabletInputService`, which WinPE
doesn't have. `j5osk.exe` is a single ~10 KB executable with no C runtime:
- docks at the bottom 30% of the screen, always on top;
- never takes focus (`WS_EX_NOACTIVATE`), so keys go to the window you last tapped;
- sends real key presses with `SendInput` (letters, digits, symbols, Esc, Tab,
  arrows, Backspace, Enter);
- Shift and Ctrl are sticky: tap once, and they apply to the next key.

## Building (WSL Debian)

```bash
sudo apt install clang lld llvm mingw-w64-common
```

```bash
bash build.sh
```

The result is `out/j5osk.exe`.

## Installing into WinPE

```bash
bash install-bootwim.sh "/mnt/e/path/to/sd card/sources/boot.wim"
```

This copies it to `X:\Windows\System32` and appends this line to `startnet.cmd`:

```
start "" X:\Windows\System32\j5osk.exe
```

If your `winpeshl.ini` launches `cmd.exe` directly, `startnet.cmd` never runs.
Make it run `cmd /k startnet.cmd` instead, or start `j5osk.exe` by hand.

## Using it
Tap the cmd window first to give it focus, then type.
