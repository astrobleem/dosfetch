// ============================================================================
// TANDY GRAPHICS LIBRARY (HEADER)
// ============================================================================
/* This library provides a set of high-performance graphics routines for
   Tandy 1000 and PCjr systems, as well as standard CGA/MDA support.

   Key Features:
   - Support for Tandy 16-color modes (Low, Medium, High)
   - Double-buffered rendering (Framebuffer -> Video Memory)
   - Optimized blitting routines for interleaved memory layouts
   - Sprite rendering with scaling and collision detection
   - Advanced effects: Copper bars, Parallax scrolling, Mode 7, 3D Wireframes
*/
// ============================================================================

#ifndef GRAPHICS_H
#define GRAPHICS_H

/* ----------------------------------------------------------------------------
   GRAPHICS MODES
   ---------------------------------------------------------------------------- */
#define CGA_4 4     /* CGA 320x200 4-color (2 bits per pixel) */
#define CGA_5 5     /* CGA 320x200 4-color (BW Burst) */
#define CGA_6 6     /* CGA 640x200 2-color (1 bit per pixel) */
#define MDA_7 7     /* MDA 80x25 Monochrome Text */
#define LOW_16 8    /* Tandy Mode 8 (160x200, 16 colors) */
#define MEDIUM_16 9 /* Tandy Mode 9 (320x200, 16 colors) - PRIMARY MODE */
#define HIGH_4 0x0A /* Tandy Mode 0Ah (640x200, 4 colors) */

/* ----------------------------------------------------------------------------
   COLOR PALETTE (Standard IRGB)
   ---------------------------------------------------------------------------- */
#define BLACK 0
#define BLUE 1
#define GREEN 2
#define CYAN 3
#define RED 4
#define MAGENTA 5
#define BROWN 6
#define LIGHTGRAY 7
#define DARKGRAY 8
#define LIGHTBLUE 9
#define LIGHTGREEN 10
#define LIGHTCYAN 11
#define LIGHTRED 12
#define LIGHTMAGENTA 13
#define YELLOW 14
#define WHITE 15

/* ----------------------------------------------------------------------------
   DATA STRUCTURES
   ---------------------------------------------------------------------------- */
typedef struct {
    int x, y;       /* Position */
    int dx, dy;     /* Velocity */
    int color;      /* Solid color (for simple sprites) */
    int active;     /* 1 = Active, 0 = Inactive */
} Sprite;

/* ----------------------------------------------------------------------------
   GLOBAL VARIABLES
   ---------------------------------------------------------------------------- */
/* The off-screen framebuffer. All drawing operations should target this buffer
   first, then call a blit() function to update the screen.
   Size: 128,000 bytes (Max 640x200 bytes) */
extern unsigned char huge framebuffer[128000];

/* ----------------------------------------------------------------------------
   CORE GRAPHICS FUNCTIONS
   ---------------------------------------------------------------------------- */

/**
 * Sets the video mode using BIOS Interrupt 10h.
 * @param m The mode number (e.g., MEDIUM_16, CGA_4).
 */
void mode(int m);

/**
 * Clears the screen (fills with BLACK) using the 'box' primitive.
 */
void cls(void);

/**
 * Moves the text cursor to the specified coordinates.
 * @param x Column (0-39 or 0-79)
 * @param y Row (0-24)
 */
void gotoxy(int x, int y);

/**
 * Fills the entire framebuffer with a specific color.
 * @param color The color index (0-15).
 */
void clear_framebuffer(int color);

/**
 * Sets a single pixel in the framebuffer.
 * @param x X Coordinate (0-319)
 * @param y Y Coordinate (0-199)
 * @param color Color index (0-15)
 */
void set_pixel(int x, int y, int color);

/* ----------------------------------------------------------------------------
   DRAWING PRIMITIVES
   ---------------------------------------------------------------------------- */

/**
 * Draws a pixel directly to VIDEO MEMORY using BIOS (Slow).
 * Use set_pixel() for framebuffer drawing instead.
 * @param x X Coordinate
 * @param y Y Coordinate
 * @param color Color index
 */
void point(int x, int y, int color);

/**
 * Draws a filled rectangle using set_pixel().
 * @param x1 Top-left X
 * @param y1 Top-left Y
 * @param x2 Bottom-right X
 * @param y2 Bottom-right Y
 * @param color Color index
 */
void box(int x1, int y1, int x2, int y2, int color);

/**
 * Draws a line using Bresenham's algorithm (BIOS version).
 * @param x1 Start X
 * @param y1 Start Y
 * @param x2 End X
 * @param y2 End Y
 * @param color Color index
 */
void line(int x1, int y1, int x2, int y2, int color);

/**
 * Draws a line using Bresenham's algorithm (Framebuffer version).
 * Much faster than line().
 * @param x1 Start X
 * @param y1 Start Y
 * @param x2 End X
 * @param y2 End Y
 * @param color Color index
 */
void line_fast(int x1, int y1, int x2, int y2, int color);

/**
 * Draws a scaled 8x8 character from font data.
 * @param x Top-left X
 * @param y Top-left Y
 * @param color Color index
 * @param scale Scaling factor (e.g., 2 for 16x16)
 * @param data Pointer to 8 bytes of font data
 */
void draw_char_scaled(int x, int y, int color, int scale, unsigned char *data);

/* ----------------------------------------------------------------------------
   BLITTING (Framebuffer -> Video Memory)
   ---------------------------------------------------------------------------- */

/**
 * Copies the framebuffer to video memory for Tandy Mode 9 (320x200, 16 colors).
 * Handles the 4-way interleaved memory layout.
 */
void blit(void);

/**
 * Copies the framebuffer to video memory for Tandy Mode 0Ah (640x200, 4 colors).
 * Handles 2 bits per pixel packing.
 */
void blit_high(void);

/**
 * Copies the framebuffer to video memory for Tandy Mode 8 (160x200, 16 colors).
 */
void blit_low(void);

/**
 * Copies the framebuffer to video memory for CGA Mode 4 (320x200, 4 colors).
 */
void blit_cga_4(void);

/**
 * Copies the framebuffer to video memory for CGA Mode 6 (640x200, 2 colors).
 */
void blit_cga_6(void);

/**
 * Blits the framebuffer to a specific video page (Mode 9).
 * @param page The target video page (0 or 1).
 */
void blit_page(int page);

/* ----------------------------------------------------------------------------
   SPRITE FUNCTIONS
   ---------------------------------------------------------------------------- */

/**
 * Draws a 16x16 sprite to the framebuffer (No scaling).
 * @param x Top-left X
 * @param y Top-left Y
 * @param color Color index
 * @param data Pointer to 16 unsigned shorts (bitmap data)
 */
void draw_sprite_fast(int x, int y, int color, const unsigned short *data);

/**
 * Draws a sprite for High Resolution mode (640x200).
 */
void draw_sprite_high(int x, int y, int color, const unsigned short *data);

/**
 * Draws a sprite for Low Resolution mode (160x200).
 */
void draw_sprite_low(int x, int y, int color, const unsigned short *data);

/**
 * Draws a scaled 16x16 sprite.
 * @param x Top-left X
 * @param y Top-left Y
 * @param color Color index
 * @param scale Fixed-point scale (256 = 1.0)
 * @param data Pointer to sprite data
 */
void draw_sprite_scaled(int x, int y, int color, int scale, const unsigned short *data);

/**
 * Checks for collision between two sprites (AABB).
 * @param s1 Pointer to first sprite
 * @param s2 Pointer to second sprite
 * @return 1 if colliding, 0 otherwise
 */
int check_sprite_collision(Sprite *s1, Sprite *s2);

/* ----------------------------------------------------------------------------
   BACKGROUNDS & DEMO EFFECTS
   ---------------------------------------------------------------------------- */

/**
 * Draws a scrolling XOR pattern background (Standard).
 */
void draw_background(int scroll_x, int scroll_y);

/**
 * Draws a scrolling XOR pattern background (High Res).
 */
void draw_background_high(int scroll_x, int scroll_y);

/**
 * Draws a scrolling XOR pattern background (Low Res).
 */
void draw_background_low(int scroll_x, int scroll_y);

/**
 * Draws a scrolling XOR pattern background (CGA).
 */
void draw_background_cga(int scroll_x, int scroll_y);

/**
 * Draws a Mode 7 style perspective plane.
 * @param angle Rotation angle (0-255)
 * @param cx Camera X
 * @param cy Camera Y
 */
void draw_mode7_background(int angle, int cx, int cy);

/**
 * Draws a rotating 3D wireframe cube.
 * @param angle_x X Rotation (0-255)
 * @param angle_y Y Rotation (0-255)
 * @param angle_z Z Rotation (0-255)
 * @param scale Scale factor (e.g., 64)
 */
void draw_wireframe_cube(int angle_x, int angle_y, int angle_z, int scale);

/**
 * Draws a 3-layer parallax scrolling background.
 * @param scroll_x Horizontal scroll offset
 */
void draw_parallax_background(int scroll_x);

/**
 * Draws a palette cycling demo (TANDY logo).
 * @param frame Frame counter for cycling
 */
void draw_palette_cycling_demo(int frame);

/**
 * Draws a pattern for hardware scrolling demonstration.
 */
void draw_hardware_scrolling_demo(void);

/**
 * Draws "Copper Bars" using raster effects (Palette changes per scanline).
 * @param offset Animation offset
 */
void draw_raster_bars(int offset);

/**
 * Cycles the entire palette for a full-screen color cycling effect.
 * @param offset Animation offset
 */
void cycle_palette_bars(int offset);

/**
 * Draws a rectangle directly to a specific video page in VRAM.
 * @param x Top-left X
 * @param y Top-left Y
 * @param w Width
 * @param h Height
 * @param color Color index
 * @param page Target page (0 or 1)
 */
void draw_rect_page(int x, int y, int w, int h, int color, int page);

/* ----------------------------------------------------------------------------
   HARDWARE CONTROL
   ---------------------------------------------------------------------------- */

/**
 * Sets the CRTC Start Address for hardware scrolling.
 * @param offset The memory offset (in words) to start displaying from.
 */
void set_crtc_start(unsigned int offset);

/**
 * Sets the active video page for hardware page flipping.
 * @param page The page number (0 or 1).
 */
void set_video_page(int page);

/**
 * Waits for the Vertical Sync (VSync) period.
 * Essential for tear-free animation and raster effects.
 */
void wait_vsync(void);

#endif
