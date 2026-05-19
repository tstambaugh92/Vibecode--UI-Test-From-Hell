# Windows Build Notes

This project supports Linux and Windows builds through `make`.

## 1) Linux (current machine)

```bash
make
make run
```

## 2) Windows build on a Windows machine (recommended)

Use MSYS2 UCRT64 shell:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-SDL3 make
```

Then from the project root:

```bash
make win WIN_CC=x86_64-w64-mingw32-gcc WIN_PKG_CONFIG=x86_64-w64-mingw32-pkg-config
```

Result:

- `bin/codex_test.exe`

## 3) Package `exe + SDL3.dll` for demo

If `SDL3.dll` is available in your UCRT64 environment, package with:

```bash
make win-package \
  WIN_CC=x86_64-w64-mingw32-gcc \
  WIN_PKG_CONFIG=x86_64-w64-mingw32-pkg-config \
  SDL3_DLL_DIR=/ucrt64/bin
```

Result:

- `bin/windows-package/codex_test.exe`
- `bin/windows-package/SDL3.dll`

If your `SDL3.dll` is elsewhere, pass `SDL3_DLL=/full/path/to/SDL3.dll`.
