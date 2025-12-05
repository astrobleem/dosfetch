# Tandy Graphics Bible
## A Guide to High-Performance Graphics on Tandy 1000 & PCjr

**Version:** 1.0  
**Author:** Antigravity  
**Date:** December 2, 2025

---

## 1. Introduction

The Tandy Graphics Library is a specialized C library designed to unlock the full potential of the Tandy 1000 and PCjr video hardware. Unlike standard CGA, the Tandy Video Graphics Array (TGA) offers 16-color modes, hardware page flipping, and unique memory layouts that require specific handling.

This library abstracts the complexity of:
*   **Video Modes:** Setting up 160x200, 320x200, and 640x200 modes.
*   **Memory Layout:** Handling the 4-way interleaved video memory.
*   **Double Buffering:** Drawing to an off-screen framebuffer and blitting to VRAM.
*   **Hardware Effects:** Copper bars, palette cycling, and page flipping.

---

## 2. Getting Started

### Include the Header
```c
#include "graphics.h"
```

### Initialize a Mode
The library supports several modes. The most versatile is **Mode 9 (Medium Res)**.
```c
void main() {
    mode(MEDIUM_16); // Set 320x200, 16 colors
    
    // Your code here...
    
    mode(3); // Restore text mode before exiting
}
```

### The Framebuffer
All drawing operations (except `point` and `draw_rect_page`) target the global `framebuffer` array. This ensures flicker-free rendering.
```c
// 1. Clear Framebuffer
clear_framebuffer(BLACK);

// 2. Draw Shapes
box(10, 10, 50, 50, RED);
line(0, 0, 319, 199, WHITE);

// 3. Blit to Screen
blit();
```

---

## 3. Core Concepts

### 3.1. Interleaved Memory
Tandy video memory is **interleaved**. In Mode 9 (320x200x16), lines are stored in 4 banks:
*   **Bank 0:** Lines 0, 4, 8...
*   **Bank 1:** Lines 1, 5, 9...
*   **Bank 2:** Lines 2, 6, 10...
*   **Bank 3:** Lines 3, 7, 11...

The `blit()` function handles this automatically, packing 2 pixels (4 bits each) into one byte and writing to the correct offset.

### 3.2. Palette & Colors
The library uses the standard IRGB palette (0-15).
*   **0:** Black
*   **1:** Blue
*   **2:** Green
*   **4:** Red
*   **15:** White

### 3.3. Sprites
Sprites are 16x16 bitmaps. The library supports:
*   **Fast Drawing:** `draw_sprite_fast` (1-bit source, solid color).
*   **Scaling:** `draw_sprite_scaled` (Arbitrary scaling).
*   **Collision:** `check_sprite_collision` (AABB).

---

## 4. Advanced Effects

### 4.1. Copper Bars (Raster Effects)
> [!WARNING]
> **Experimental Feature:** This effect relies on extremely precise CPU timing (counting cycles) to change the palette during the horizontal blanking interval. It may not work correctly on all hardware speeds or emulators, often resulting in flickering or solid colors. Use with caution.

"Copper bars" are horizontal color bars created by changing the palette *during* the screen refresh.
```c
// Call this in your main loop
draw_raster_bars(offset); 
offset++;
```
*Requires `wait_vsync()` to stabilize the effect.*

### 4.2. Mode 7 (Perspective Plane)
Renders a pseudo-3D floor plane similar to SNES Mode 7.
```c
draw_mode7_background(angle, camera_x, camera_y);
```

### 4.3. Hardware Page Flipping
For the smoothest animation, use hardware page flipping (Mode I).
1.  Draw to the *inactive* page in VRAM using `draw_rect_page` or `blit_page`.
2.  Flip the visible page using `set_video_page`.
```c
int active_page = 0;
while(1) {
    int draw_page = !active_page;
    
    // Draw to hidden page
    draw_rect_page(x, y, w, h, color, draw_page);
    
    // Flip
    set_video_page(draw_page);
    active_page = draw_page;
    
    wait_vsync();
}
```

---

## 5. Windows 3.0 Driver Developer Guide

If you are developing a display driver (`TANDY16.DRV`) for Windows 3.0, this library provides the foundational routines you need.

### 5.1. Essential Driver Functions

A Windows display driver must implement specific DDI (Device Driver Interface) functions. Here is how you can map them to this library's concepts:

#### `Enable()` / `Disable()`
Initializes the hardware.
*   **Implementation:** Use `mode(MEDIUM_16)` to enter graphics mode.
*   **Note:** You must save the original video state and restore it in `Disable()`.

#### `BitBlt()` (Bit Block Transfer)
Moves pixels between memory and screen, or screen to screen.
*   **Implementation:** 
    *   **Memory -> Screen:** Use the logic in `blit()`. You must handle the 4-way interleave!
    *   **Screen -> Screen:** Read from VRAM (0xB800 segment), handle interleave, write to destination.
    *   **ROP Codes:** Windows requires Raster Operations (XOR, AND, OR, COPY). You must apply these logic operations to the pixel data.

#### `Output()` (Line Drawing)
Draws lines and scans.
*   **Implementation:** Adapt `line_fast()`.
*   **Optimization:** For horizontal lines (scans), write consecutive bytes to VRAM. For vertical lines, you must jump 8KB (0x2000) between banks.

#### `Pixel()` (Set/Get Pixel)
*   **SetPixel:**
    ```c
    void SetPixel(int x, int y, int color) {
        unsigned char far *vram = 0xB8000000L;
        int bank = y & 3;
        int offset = (bank * 0x2000) + ((y / 4) * 160) + (x / 2);
        unsigned char val = vram[offset];
        
        if (x & 1) { // Odd pixel (Low nibble)
            val = (val & 0xF0) | (color & 0x0F);
        } else {     // Even pixel (High nibble)
            val = (val & 0x0F) | (color << 4);
        }
        vram[offset] = val;
    }
    ```

### 5.2. Sample Driver Skeleton

```c
// TANDY16.DRV Skeleton

#include "graphics.h" // Use our library for low-level ops

// The PDEVICE structure holds state
typedef struct {
    int mode;
    int width;
    int height;
    // ... Windows specific fields
} PDEVICE;

int FAR PASCAL Enable(PDEVICE *pd, int style, LPSTR type, LPSTR output, LPSTR stuff) {
    // 1. Save current BIOS mode
    // 2. Switch to Tandy Mode 9
    mode(MEDIUM_16);
    
    // 3. Initialize PDEVICE
    pd->mode = MEDIUM_16;
    pd->width = 320;
    pd->height = 200;
    
    return 1; // Success
}

void FAR PASCAL Disable(PDEVICE *pd) {
    // Restore text mode
    mode(3);
}

// The "Big One" - BitBlt
int FAR PASCAL BitBlt(PDEVICE *dest_dev, int dest_x, int dest_y, 
                      PDEVICE *src_dev, int src_x, int src_y,
                      int width, int height, long rop, 
                      void *lpPBrush, void *lpDrawMode) {
                      
    // Check if destination is the screen
    if (dest_dev->type == DEVICE_SCREEN) {
        // We are writing to Tandy VRAM!
        // Use our interleaved addressing logic here.
        
        int row, col;
        for (row = 0; row < height; row++) {
            // Calculate VRAM address for (dest_x, dest_y + row)
            // ...
            
            for (col = 0; col < width; col++) {
                // Get source pixel
                // Apply ROP (e.g., SRCCOPY, PATCOPY, XOR)
                // Write to VRAM
            }
        }
    }
    return 1;
}
```

### 5.3. Critical Tips for Driver Devs
1.  **Bank Switching:** Tandy 1000 doesn't use bank switching like SVGA. It uses **Interleaving**. Always remember the `y & 3` rule.
2.  **Performance:** Direct VRAM access is slow. Use an off-screen bitmap (DIB) for complex composition, then Blit the final result to the screen (like our `framebuffer` -> `blit()` workflow).
3.  **Mouse Cursor:** You must implement a software cursor. This involves saving the background under the cursor, drawing the cursor, and restoring the background when it moves. See `draw_sprite_fast` for drawing logic.

---
**Happy Coding!**
*Antigravity*
