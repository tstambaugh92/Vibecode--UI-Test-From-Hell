# Notice

Everything exept this text, including the headers below this, is vibecode slop.

The purpose of this repo is not a product, but as example code to reference when making SDL games.

I have made a ton of SDL games in the past, but one thing I struggled with was menus.

I'm not a lawyer, I don't claim any of this vibecode is mine and intend for it only to be a fun helpful way to learn menus as windows. The program does things like place multiple virtual windows and runs the windows as separate processes with the option to pause other processes on demand. It also scales these UI elements appropriately. For those reasons, I believe this information is helpful if you're learning SDL, so I'm sharing it.

I don't know what the actual rules are as far as an SDL3 package under release goes. SDL3 is under the zlib license to my knowledge. If I'm violating any rules, make a PR with a correction or contact me at stambtx@proton.me.

As of 5/20/26, I added Stalin sort and confirmed another arch linux machine could build and run the app image. Maybe I'll see about windows too, but otherwise I'm about done messing with this.

The way that it runs the snake and sorting logic in easter egg mode without them stepping on eachother is something I would have struggled to figure out on my own, but also, any modern game engine just does that. Educational goodness.


# Vibecode: UI Test From Hell

Interactive SDL3 playground featuring:
- Sorting visualizer with multiple algorithms
- Audio feedback
- Animated background mode
- Snake easter egg (fullscreen + PiP)

![Demo](demo.gif)

## Build

### Linux
```bash
make
```

The output is in the bin folder. You can run it either from the repo root or the bin folder and the audio should still work.

### Windows cross-build (from Linux)
```bash
make win
```

Create a demo package folder (exe + optional SDL3.dll):
```bash
make win-package SDL3_DLL=/path/to/SDL3.dll
```

This is untested, by the way.

### Linux AppImage

make appimage 

## Controls

- `1..7` select sorting algorithm
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

### Other Credits
I love this font, Daniel Bold, https://www.dafont.com/daniel.font
