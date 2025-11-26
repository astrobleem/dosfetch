# DOSfetch 🎨🎵

**Neofetch for DOS** with special support for Tandy 1000 machines!

A system information tool that celebrates the glory days of 16-bit computing, now with **16-color palette display** and **musical Tandy PSG sound**!

## Why Tandy 1000?

The Tandy 1000 (1984-1987) was the unsung hero of home computing - bridging business and gaming with features that put the IBM PC to shame:

- **16 colors** vs IBM PC's 4-color CGA
- **3 tone channels + noise** (Texas Instruments SN76496) vs PC Speaker beeps
- **320×200 in 16 colors** - Tandy Mode 9 (exclusive!)
- **PCjr compatibility** - but actually successful!

Hundreds of games supported "Tandy graphics and sound" including King's Quest, Space Quest, Ultima series, and SimCity. This was the machine that made DOS gaming musical! 🎮

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
- **Smooth Fade Out** - Professional sound design

*For SepTandy - celebrating all things Tandy!*

## Build Instructions

Built with **Open Watcom 1.9**:

```bash
wmake
```

That's it! Produces `neofetch.exe` ready for DOS.

## Usage

Just run it:
```
neofetch.exe
```

On Tandy systems with PSG, you'll hear the C major chord! 🎵

## Technical Details

### CPU Detection
Uses FLAGS register tests to identify:
- 8086/8088 (bits 12-15 stuck high)
- 80286 (bits 12-15 stuck low)
- 80386+ (bits 12-15 changeable)

NEC V20/V30 detection uses the SALC instruction (opcode D6h) behavior difference - a documented 1980s technique!

### PSG Sound Synthesis
- **Clock:** 3.579545 MHz (NTSC)
- **Formula:** `fout = CLK / (32 * period)`
- **Timing:** BIOS tick counter (~18.2 Hz)

Based on [psgtest.c](https://github.com/astrobleem/oemsound-tandy) reference implementation.

## Credits

- Original dosfetch by [Leah Neukirchen](mailto:leah@vuxu.org)
- Shell version by [@doekman](https://github.com/doekman/dosfetch)
- GRAFIX.ASM code by Joseph A. Albrecht
- Tandy PSG code inspired by [oemsound-tandy](https://github.com/astrobleem/oemsound-tandy)

## Pull Requests Welcome! 🤝

Help make this the ultimate DOS system info tool!

---

**Sixteen colors and three sound channels!** The Tandy 1000 way. 🎨🎵

## Screenshots

![image](https://github.com/user-attachments/assets/bf9bc351-fbba-48cd-8400-fd4231798745)

![image](https://github.com/user-attachments/assets/9cfceb71-4f2b-4a09-adf6-01eed93f4e96)
