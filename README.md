# Notice

Everything exept this text, including the headers below this, is vibecode slop.

The purpose of this repo is not a product, but as example code to reference when making SDL games.

I have made a ton of SDL games in the past, but one thing I struggled with was menus.

I'm not a lawyer, I don't claim any of this vibecode is mine and intend for it only to be a fun helpful way to learn menus as windows. The program does things like place multiple virtual windows and runs the windows as separate processes with the option to pause other processes on demand. It also scales these UI elements appropriately. For those reasons, I believe this information is helpful if you're learning SDL, so I'm sharing it.


Demo:

![Demo](demo.gif)



# Vibecode: UI Test From Hell

Interactive SDL3 playground featuring:
- Sorting visualizer with multiple algorithms
- Audio feedback
- Animated background mode
- Snake easter egg (fullscreen + PiP)

## Build

### Linux
```bash
make
```

Run:
```bash
make run
```

### Windows cross-build (from Linux)
```bash
make win
```

Create a demo package folder (exe + optional SDL3.dll):
```bash
make win-package SDL3_DLL=/path/to/SDL3.dll
```

## Controls

- `1..6` select sorting algorithm
- `S` start/pause
- `R` reshuffle
- `+/-` speed up/down
- `M` mute/unmute
- `B` animated background on/off
- `H` help overlay
- `Numpad 1-9` palette select
- `F` or `F11` fullscreen
- `Esc` quit

Snake easter egg:
- type `S N A K E`
- `Tab` toggles Snake PiP mode
- arrow keys control snake
- `Space` restarts snake when dead
- `X` closes snake mode

## Project Layout

```text
include/    public headers
src/        implementation files
bin/        built binaries
```

## Notes

- Keep `Makefile` in sync when new build targets are added.
- Windows build instructions: see `README-build-windows.md`.
