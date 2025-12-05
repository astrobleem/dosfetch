# DOSfetch 🎨🎵

**Neofetch for DOS** with special support for Tandy 1000 machines!

A system information tool that celebrates the glory days of 16-bit computing, now with **16-color palette display**, **musical Tandy PSG sound**, and a suite of **Graphics Demos**!

## Why Tandy 1000?

The Tandy 1000 (1984-1987) was the unsung hero of home computing - bridging business and gaming with features that put the IBM PC to shame:

- **16 colors** vs IBM PC's 4-color CGA
- **3 tone channels + noise** (Texas Instruments SN76496) vs PC Speaker beeps
- **320×200 in 16 colors** - Tandy Mode 9 (exclusive!)
- **PCjr compatibility** - but actually successful!

## Features ✨

### System Detection
- **CPU Type** - Intel 8088/8086/286/386+ with **NEC V20/V30 detection**!
- **Tandy Hardware** - Specific detection for Tandy 1000 series
- **Video Mode** - CGA, Tandy Mode 9, Mode A detection
- **Memory** - Base and extended memory reporting
- **Sound Chips** - SN76496 PSG, 8253 Timer detection
- **FPU Detection** - Math coprocessor check

### Visual Goodness
- **16-Color Palette Bars** - Classic neofetch style
- **Compact DOS Logo** - Clean ASCII art
- **Tandy ASCII Art** - Special logo for Tandy machines
- **Color-Coded Output** - Uses full 16-color DOS palette

### Audio Magic
- **C Major Triad** - Musical startup sound on Tandy PSG
- **3-Channel Polyphony** - Full chord (C4 + E4 + G4)
- **Noise Percussion** - Satisfying "click" using channel 4

### Graphics Demos 🎮
Run `dosfetch [mode]` to launch specific demos:

| Mode | Command | Description | Controls |
|------|---------|-------------|----------|
| **7** | `dosfetch 7` | **Mode 7 Scaling** (SNES Style) | Arrows to move/rotate |
| **B** | `dosfetch B` | **Wireframe Cube** | Arrows to rotate, `*`/`/` to scale |
| **C** | `dosfetch C` | **Parallax Scrolling** | Arrows to scroll |
| **D** | `dosfetch D` | **Palette Cycling** | Auto-cycling rainbow text |
| **E** | `dosfetch E` | **Sprite Scaling** | `+`/`-` to scale logo |
| **F** | `dosfetch F` | **Sprite Collision** | Arrows to move player (Green) |
| **G** | `dosfetch G` | **Mouse Drawing** | Left Click (Draw), Right Click (Erase) |

*Also supports standard CGA modes (4, 5, 6) and Tandy Low/High Res (8, 10).*

## Build Instructions

Built with **Open Watcom 1.9**.

**Compile:**
```bash
wcc -ml main.c
wcc -ml tandy_graph.c
```

**Link:**
```bash
wlink system dos file main.obj file tandy_graph.obj name dosfetch.exe
```

## Usage

Just run it:
```
dosfetch.exe
```

On Tandy systems with PSG, you'll hear the C major chord! 🎵

## Technical Details

### CPU Detection
Uses FLAGS register tests to identify:
- 8086/8088 (bits 12-15 stuck high)
- 80286 (bits 12-15 stuck low)
- 80386+ (bits 12-15 changeable)

NEC V20/V30 detection uses the SALC instruction (opcode D6h) behavior difference.

### Graphics Engine
- **Direct Framebuffer Access**: 64KB double-buffered rendering.
- **Custom Blitters**: Optimized assembly-like C routines for Mode 9 (4-way interleave), Mode 8 (2-way), and Mode 0Ah (2-bit packing).
- **Fixed-Point Math**: Used for 3D rotations and scaling on integer-only CPUs.

## Credits

- Original dosfetch by [Leah Neukirchen](mailto:leah@vuxu.org)
- Shell version by [@doekman](https://github.com/doekman/dosfetch)
- GRAFIX.ASM code by Joseph A. Albrecht
- Tandy PSG code inspired by [oemsound-tandy](https://github.com/astrobleem/oemsound-tandy)

## Screenshots

![Mode G Proof](/C:/Users/chad/.gemini/antigravity/brain/82635400-fb66-4c8e-b67f-5e1d459a371d/mode_g_proof.png)

![image](https://github.com/user-attachments/assets/bf9bc351-fbba-48cd-8400-fd4231798745)
