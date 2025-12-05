#include <i86.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <dos.h>
#include "graphics.h"

/* Constants */
#define BLACK 0
#define DARKGRAY 8

/* Framebuffer (640x200 bytes max)
   This is the off-screen buffer where all drawing happens. */
unsigned char huge framebuffer[128000];

/* ============================================================================
   CORE FUNCTIONS
   ============================================================================ */

void mode(int m) {
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = (unsigned char)m;
    int86(0x10, &regs, &regs);
}

void point(int x, int y, int color) {
    /* BIOS Pixel Plot - Very Slow, use for debugging only */
    union REGS regs;
    regs.h.ah = 0x0C;
    regs.h.al = (unsigned char)color;
    regs.h.bh = 0;
    regs.w.cx = x;
    regs.w.dx = y;
    int86(0x10, &regs, &regs);
}

void box(int x1, int y1, int x2, int y2, int color) {
    int x, y;
    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            set_pixel(x, y, color);
        }
    }
}

void line(int x1, int y1, int x2, int y2, int color) {
    /* Bresenham's Line Algorithm (BIOS Version) */
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;) {
        point(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void cls(void) {
    /* Clear screen by filling framebuffer with BLACK
       Note: This only clears the framebuffer, call blit() to update screen. */
    box(0, 0, 319, 199, BLACK);
}

void gotoxy(int x, int y) {
    union REGS regs;
    regs.h.ah = 0x02;
    regs.h.bh = 0;
    regs.h.dh = (unsigned char)y;
    regs.h.dl = (unsigned char)x;
    int86(0x10, &regs, &regs);
}

void clear_framebuffer(int color) {
    unsigned long i;
    for (i = 0; i < 128000; i++) {
        framebuffer[i] = (unsigned char)color;
    }
}

void set_pixel(int x, int y, int color) {
    /* Safe pixel setting with bounds checking */
    if (x >= 0 && x < 320 && y >= 0 && y < 200) {
        framebuffer[(unsigned long)y * 320 + x] = (unsigned char)color;
    }
}

/* ============================================================================
   BLITTING ROUTINES
   ============================================================================ */

void blit(void) {
    /* Tandy Mode 9 (320x200, 16 colors) Blitter
       Video Memory starts at 0xB800:0000
       Layout: 4-way Interleaved
       Bank 0: Lines 0, 4, 8...
       Bank 1: Lines 1, 5, 9...
       Bank 2: Lines 2, 6, 10...
       Bank 3: Lines 3, 7, 11...
       Pixels are packed: 2 pixels per byte (4 bits each) */
    
    unsigned char far *video_mem = (unsigned char far *)0xB8000000L;
    int x, y;
    int bank;
    unsigned int offset;
    unsigned char packed;
    
    for (y = 0; y < 200; y++) {
        bank = y & 3; /* y % 4 */
        /* Offset calculation:
           (Bank * 8KB) + (Line Index * BytesPerLine)
           8KB = 0x2000
           Line Index = y / 4
           BytesPerLine = 320 pixels / 2 = 160 bytes */
        offset = (bank * 0x2000) + ((y / 4) * 160);
        
        for (x = 0; x < 320; x += 2) {
            /* Pack 2 pixels into 1 byte
               Pixel 1 (Even X): High Nibble
               Pixel 2 (Odd X): Low Nibble */
            packed = (framebuffer[(unsigned long)y * 320 + x] << 4) | (framebuffer[(unsigned long)y * 320 + x + 1] & 0x0F);
            video_mem[offset + (x / 2)] = packed;
        }
    }
}

void draw_sprite_fast(int x, int y, int color, const unsigned short *data) {
    /* Draws a 16x16 sprite (1 bit per pixel source)
       data: Array of 16 shorts, each bit represents a pixel */
    int row, col;
    unsigned short row_data;
    int px, py;
    
    for (row = 0; row < 16; row++) {
        py = y + row;
        if (py < 0 || py >= 200) continue;
        
        row_data = data[row];
        for (col = 0; col < 16; col++) {
            px = x + col;
            if (px < 0 || px >= 320) continue;
            
            /* Check bit (MSB first) */
            if (row_data & (0x8000 >> col)) {
                framebuffer[(unsigned long)py * 320 + px] = (unsigned char)color;
            }
        }
    }
}

void draw_background(int scroll_x, int scroll_y) {
    /* Simple XOR pattern background */
    int x, y;
    int col;
    for (y = 0; y < 200; y++) {
        for (x = 0; x < 320; x++) {
            if (((x + scroll_x) ^ (y + scroll_y)) & 32) {
                col = DARKGRAY;
            } else {
                col = BLACK;
            }
            framebuffer[(unsigned long)y * 320 + x] = (unsigned char)col;
        }
    }
}

void blit_high(void) {
    /* Tandy Mode 0Ah (640x200, 4 colors) Blitter
       Layout: 4-way Interleaved (Same as Mode 9)
       Pixels are packed: 4 pixels per byte (2 bits each) */
    
    unsigned char far *video_mem = (unsigned char far *)0xB8000000L;
    int x, y;
    int bank;
    unsigned int offset;
    unsigned char packed;
    
    for (y = 0; y < 200; y++) {
        bank = y & 3;
        offset = (bank * 0x2000) + ((y / 4) * 160);
        
        for (x = 0; x < 640; x += 4) {
            /* Pack 4 pixels into 1 byte */
            packed = (framebuffer[(unsigned long)y * 640 + x] << 6) |
                     ((framebuffer[(unsigned long)y * 640 + x + 1] & 0x03) << 4) |
                     ((framebuffer[(unsigned long)y * 640 + x + 2] & 0x03) << 2) |
                     (framebuffer[(unsigned long)y * 640 + x + 3] & 0x03);
            video_mem[offset + (x / 4)] = packed;
        }
    }
}

void draw_sprite_high(int x, int y, int color, const unsigned short *data) {
    /* High Res Sprite (640x200) */
    int row, col;
    unsigned short row_data;
    int px, py;
    
    for (row = 0; row < 16; row++) {
        py = y + row;
        if (py < 0 || py >= 200) continue;
        
        row_data = data[row];
        for (col = 0; col < 16; col++) {
            px = x + col;
            if (px < 0 || px >= 640) continue;
            
            if (row_data & (0x8000 >> col)) {
                framebuffer[(unsigned long)py * 640 + px] = (unsigned char)color;
            }
        }
    }
}

void draw_background_high(int scroll_x, int scroll_y) {
    int x, y;
    int col;
    for (y = 0; y < 200; y++) {
        for (x = 0; x < 640; x++) {
            if (((x + scroll_x) ^ (y + scroll_y)) & 64) {
                col = 1; 
            } else {
                col = 0;
            }
            framebuffer[(unsigned long)y * 640 + x] = (unsigned char)col;
        }
    }
}

void blit_low(void) {
    /* Tandy Mode 8 (160x200, 16 colors) Blitter
       Layout: 4-way Interleaved
       Pixels are packed: 2 pixels per byte (4 bits each)
       Wait... Mode 8 is 160x200.
       Actually, Mode 8 uses the same memory layout but pixels are double-wide?
       No, Mode 8 is 160x200x16.
       160 pixels / 2 = 80 bytes per line. */
    
    unsigned char far *video_mem = (unsigned char far *)0xB8000000L;
    int x, y;
    int bank;
    unsigned int offset;
    unsigned char packed;
    
    for (y = 0; y < 200; y++) {
        bank = y & 3;
        offset = (bank * 0x2000) + ((y / 4) * 80);
        
        for (x = 0; x < 160; x += 2) {
            packed = (framebuffer[(unsigned long)y * 160 + x] << 4) | (framebuffer[(unsigned long)y * 160 + x + 1] & 0x0F);
            video_mem[offset + (x / 2)] = packed;
        }
    }
}

void draw_sprite_low(int x, int y, int color, const unsigned short *data) {
    int row, col;
    unsigned short row_data;
    int px, py;
    
    for (row = 0; row < 16; row++) {
        py = y + row;
        if (py < 0 || py >= 200) continue;
        
        row_data = data[row];
        for (col = 0; col < 16; col++) {
            px = x + col;
            if (px < 0 || px >= 160) continue;
            
            if (row_data & (0x8000 >> col)) {
                framebuffer[(unsigned long)py * 160 + px] = (unsigned char)color;
            }
        }
    }
}

void draw_background_low(int scroll_x, int scroll_y) {
    int x, y;
    int col;
    for (y = 0; y < 200; y++) {
        for (x = 0; x < 160; x++) {
            if (((x + scroll_x) ^ (y + scroll_y)) & 16) {
                col = 12; 
            } else {
                col = 0;
            }
            framebuffer[(unsigned long)y * 160 + x] = (unsigned char)col;
        }
    }
}

void blit_cga_4(void) {
    /* CGA Mode 4 (320x200, 4 colors)
       Layout: 2-way Interleaved (Odd/Even lines)
       Bank 0: Even lines (0, 2, 4...) at 0xB800:0000
       Bank 1: Odd lines (1, 3, 5...) at 0xB800:2000
       Pixels: 4 pixels per byte (2 bits each) */
    
    unsigned char far *video_mem = (unsigned char far *)0xB8000000L;
    int x, y;
    int bank;
    unsigned int offset;
    unsigned char packed;
    
    for (y = 0; y < 200; y++) {
        bank = y & 1; 
        offset = (bank * 0x2000) + ((y / 2) * 80); 
        
        for (x = 0; x < 320; x += 4) {
            packed = ((framebuffer[(unsigned long)y * 320 + x] & 3) << 6) |
                     ((framebuffer[(unsigned long)y * 320 + x + 1] & 3) << 4) |
                     ((framebuffer[(unsigned long)y * 320 + x + 2] & 3) << 2) |
                     (framebuffer[(unsigned long)y * 320 + x + 3] & 3);
            video_mem[offset + (x / 4)] = packed;
        }
    }
}

void blit_cga_6(void) {
    /* CGA Mode 6 (640x200, 2 colors)
       Layout: 2-way Interleaved
       Pixels: 8 pixels per byte (1 bit each) */
    
    unsigned char far *video_mem = (unsigned char far *)0xB8000000L;
    int x, y;
    int bank;
    unsigned int offset;
    unsigned char packed;
    
    for (y = 0; y < 200; y++) {
        bank = y & 1;
        offset = (bank * 0x2000) + ((y / 2) * 80);
        
        for (x = 0; x < 640; x += 8) {
            packed = 0;
            if (framebuffer[(unsigned long)y * 640 + x]) packed |= 0x80;
            if (framebuffer[(unsigned long)y * 640 + x + 1]) packed |= 0x40;
            if (framebuffer[(unsigned long)y * 640 + x + 2]) packed |= 0x20;
            if (framebuffer[(unsigned long)y * 640 + x + 3]) packed |= 0x10;
            if (framebuffer[(unsigned long)y * 640 + x + 4]) packed |= 0x08;
            if (framebuffer[(unsigned long)y * 640 + x + 5]) packed |= 0x04;
            if (framebuffer[(unsigned long)y * 640 + x + 6]) packed |= 0x02;
            if (framebuffer[(unsigned long)y * 640 + x + 7]) packed |= 0x01;
            
            video_mem[offset + (x / 8)] = packed;
        }
    }
}

void draw_background_cga(int scroll_x, int scroll_y) {
    int x, y;
    int col;
    for (y = 0; y < 200; y++) {
        for (x = 0; x < 320; x++) {
            if (((x + scroll_x) ^ (y + scroll_y)) & 32) {
                col = 1; 
            } else {
                col = 0;
            }
            framebuffer[(unsigned long)y * 320 + x] = (unsigned char)col;
        }
    }
}

// ============================================================================
// MATH & ADVANCED RENDERING
// ============================================================================

// Fixed Point Math (8.8 format)
// 256 = 1.0
#define FP_SHIFT 8
#define FP_ONE 256
#define PI 3.14159265359

int sin_table[256];
int cos_table[256];
int tables_initialized = 0;

void init_math_tables(void) {
    // Precompute Sine and Cosine tables for performance
    // Maps 0-255 to 0-2PI
    int i;
    for (i = 0; i < 256; i++) {
        double rad = (i * 3.14159 * 2.0) / 256.0;
        sin_table[i] = (int)(sin(rad) * 256);
        cos_table[i] = (int)(cos(rad) * 256);
    }
    tables_initialized = 1;
}

void draw_mode7_background(int angle, int cx, int cy) {
    // SNES Mode 7 Style Perspective Plane
    // Renders a floor plane with rotation and scaling
    
    int x, y;
    int horizon = 60; 
    int cos_a, sin_a;
    int space_z;
    int space_x, space_y;
    int tx, ty;
    int color;
    int dx_start, dx_end, dx_step, curr_dx;
    
    if (!tables_initialized) init_math_tables();
    
    cos_a = cos_table[angle];
    sin_a = sin_table[angle];
    
    // Clear sky
    memset(framebuffer, 1, 320 * horizon); 
    
    // Render floor scanline by scanline
    for (y = horizon; y < 200; y++) {
        if (y == horizon) continue;
        
        // Calculate Z depth based on screen Y (1/Y)
        space_z = (8192) / (y - horizon); 
        
        // Calculate X range in world space for this scanline
        dx_start = -160 * space_z;
        dx_end = 160 * space_z;
        dx_step = (dx_end - dx_start) / 320;
        
        curr_dx = dx_start;
        
        for (x = 0; x < 320; x++) {
            // Rotate world coordinates
            space_x = (curr_dx * cos_a - space_z * sin_a) >> 8;
            space_y = (curr_dx * sin_a + space_z * cos_a) >> 8;
            
            // Map to texture coordinates (checkerboard)
            tx = (cx + space_x) >> 5; 
            ty = (cy + space_y) >> 5;
            
            // XOR pattern for floor texture
            color = ((tx ^ ty) & 1) ? 8 : 7; 
            
            framebuffer[(unsigned long)y * 320 + x] = (unsigned char)color;
            
            curr_dx += dx_step; 
        }
    }
}

void set_pixel_fast(int x, int y, int color) {
    // Unsafe pixel set (no bounds check) for internal use
    if (x >= 0 && x < 320 && y >= 0 && y < 200) {
        framebuffer[(unsigned long)y * 320 + x] = (unsigned char)color;
    }
}

void line_fast(int x1, int y1, int x2, int y2, int color) {
    // Bresenham's Line Algorithm (Optimized for Framebuffer)
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;) {
        set_pixel_fast(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void draw_wireframe_cube(int angle_x, int angle_y, int angle_z, int scale) {
    // 3D Wireframe Cube Renderer
    // Rotates and projects 8 vertices
    
    int vertices[8][3] = {
        {-64, -64, -64}, {64, -64, -64}, {64, 64, -64}, {-64, 64, -64},
        {-64, -64, 64}, {64, -64, 64}, {64, 64, 64}, {-64, 64, 64}
    };
    
    int edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0}, 
        {4,5}, {5,6}, {6,7}, {7,4}, 
        {0,4}, {1,5}, {2,6}, {3,7}  
    };
    
    int p2d[8][2];
    int i;
    int x, y, z;
    int rx, ry, rz;
    int cx, cy;
    int dist = 256; 
    int z_off = 400; 
    
    if (!tables_initialized) init_math_tables();
    
    memset(framebuffer, 0, 64000);
    
    for (i = 0; i < 8; i++) {
        x = vertices[i][0];
        y = vertices[i][1];
        z = vertices[i][2];
        
        // Rotate X
        cx = cos_table[angle_x]; cy = sin_table[angle_x]; 
        ry = (y * cx - z * cy) >> 8;
        rz = (y * cy + z * cx) >> 8;
        y = ry; z = rz;
        
        // Rotate Y
        cx = cos_table[angle_y]; cy = sin_table[angle_y];
        rx = (x * cx + z * cy) >> 8;
        rz = (-x * cy + z * cx) >> 8;
        x = rx; z = rz;
        
        // Rotate Z
        cx = cos_table[angle_z]; cy = sin_table[angle_z];
        rx = (x * cx - y * cy) >> 8;
        ry = (x * cy + y * cx) >> 8;
        x = rx; y = ry;
        
        // Scale
        x = (x * scale) >> 6; 
        y = (y * scale) >> 6;
        z = (z * scale) >> 6;
        
        // Project to 2D
        if (z + z_off != 0) {
            p2d[i][0] = 160 + (x * dist) / (z + z_off);
            p2d[i][1] = 100 + (y * dist) / (z + z_off);
        } else {
            p2d[i][0] = 160 + x;
            p2d[i][1] = 100 + y;
        }
    }
    
    // Draw Edges
    for (i = 0; i < 12; i++) {
        int p1 = edges[i][0];
        int p2 = edges[i][1];
        line_fast(p2d[p1][0], p2d[p1][1], p2d[p2][0], p2d[p2][1], 10);
    }
}

void draw_parallax_background(int scroll_x) {
    // 3-Layer Parallax Scrolling
    // Uses sine waves for hills
    int x, y;
    int layer1_x, layer2_x, layer3_x;
    int h;
    int idx;
    
    if (!tables_initialized) init_math_tables();
    
    memset(framebuffer, 1, 320 * 120); 
    
    // Layer 1: Distant (Slow)
    layer1_x = scroll_x >> 2;
    for (x = 0; x < 320; x++) {
        idx = ((x + layer1_x) * 2) & 0xFF;
        h = 80 + ((sin_table[idx] * 20) >> 8);
        for (y = h; y < 200; y++) {
             framebuffer[(unsigned long)y * 320 + x] = 5; 
        }
    }
    
    // Layer 2: Middle (Medium)
    layer2_x = scroll_x >> 1;
    for (x = 0; x < 320; x++) {
        idx = ((x + layer2_x) * 3) & 0xFF;
        h = 120 + ((sin_table[idx] * 15) >> 8);
        for (y = h; y < 200; y++) {
             framebuffer[(unsigned long)y * 320 + x] = 2; 
        }
    }
    
    // Layer 3: Foreground (Fast)
    layer3_x = scroll_x;
    for (x = 0; x < 320; x++) {
        h = 160;
        for (y = h; y < 200; y++) {
             if (((x + layer3_x) ^ y) & 16) {
                 framebuffer[(unsigned long)y * 320 + x] = 10; 
             } else {
                 framebuffer[(unsigned long)y * 320 + x] = 2; 
             }
        }
    }
    
    // Sun
    for (y = 20; y < 40; y++) {
        for (x = 280; x < 300; x++) {
            if ((x-290)*(x-290) + (y-30)*(y-30) < 80) {
                framebuffer[(unsigned long)y * 320 + x] = 14; 
            }
        }
    }
}

unsigned char font_t[8] = {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18};
unsigned char font_a[8] = {0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x66};
unsigned char font_n[8] = {0x66, 0x76, 0x7E, 0x7E, 0x6E, 0x66, 0x66, 0x66};
unsigned char font_d[8] = {0x7C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x7C};
unsigned char font_y[8] = {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x18};

void draw_char_scaled(int x, int y, int color, int scale, unsigned char *data) {
    // Draws an 8x8 character scaled up
    int row, col;
    int px, py;
    int i, j;
    
    for (row = 0; row < 8; row++) {
        for (col = 0; col < 8; col++) {
            if (data[row] & (0x80 >> col)) {
                for (i = 0; i < scale; i++) {
                    for (j = 0; j < scale; j++) {
                        px = x + col * scale + j;
                        py = y + row * scale + i;
                        if (px >= 0 && px < 320 && py >= 0 && py < 200) {
                            framebuffer[(unsigned long)py * 320 + px] = (unsigned char)color;
                        }
                    }
                }
            }
        }
    }
}

void draw_palette_cycling_demo(int frame) {
    int base_color = (frame >> 2) % 15 + 1;
    int scale = 10;
    int start_x = 20;
    int y = 60;
    int spacing = 60;
    
    memset(framebuffer, 0, 64000);
    
    draw_char_scaled(start_x, y, base_color, scale, font_t);
    draw_char_scaled(start_x + spacing, y, (base_color + 1) % 15 + 1, scale, font_a);
    draw_char_scaled(start_x + spacing * 2, y, (base_color + 2) % 15 + 1, scale, font_n);
    draw_char_scaled(start_x + spacing * 3, y, (base_color + 3) % 15 + 1, scale, font_d);
    draw_char_scaled(start_x + spacing * 4, y, (base_color + 4) % 15 + 1, scale, font_y);
}

void draw_sprite_scaled(int x, int y, int color, int scale, const unsigned short *data) {
    // Draws a 16x16 sprite with scaling
    // scale: Fixed point 8.8 (256 = 1.0)
    int row, col;
    unsigned short row_data;
    int px, py;
    int w = (16 * scale) >> 8;
    int h = (16 * scale) >> 8;
    int src_x, src_y;
    
    for (py = 0; py < h; py++) {
        if (y + py < 0 || y + py >= 200) continue;
        
        src_y = (py << 8) / scale;
        if (src_y >= 16) continue;
        
        row_data = data[src_y];
        
        for (px = 0; px < w; px++) {
            if (x + px < 0 || x + px >= 320) continue;
            
            src_x = (px << 8) / scale;
            if (src_x >= 16) continue;
            
            if (row_data & (0x8000 >> src_x)) {
                framebuffer[(unsigned long)(y + py) * 320 + (x + px)] = (unsigned char)color;
            }
        }
    }
}

int check_sprite_collision(Sprite *s1, Sprite *s2) {
    // Simple AABB Collision Detection
    if (s1->x < s2->x + 16 &&
        s1->x + 16 > s2->x &&
        s1->y < s2->y + 16 &&
        s1->y + 16 > s2->y) {
        return 1;
    }
    return 0;
}

/* ============================================================================
   HARDWARE CONTROL & EFFECTS
   ============================================================================ */

void set_crtc_start(unsigned int offset) {
    /* Hardware Scrolling (CRTC Start Address)
       Tells the video controller where to start reading video memory
       offset: Word offset (byte offset / 2) */
    outp(0x3D4, 0x0C); /* Register 0Ch: Start Address High */
    outp(0x3D5, (offset >> 8) & 0xFF);
    outp(0x3D4, 0x0D); /* Register 0Dh: Start Address Low */
    outp(0x3D5, offset & 0xFF);
}

void draw_hardware_scrolling_demo(void) {
    /* Fills video memory with a pattern to demonstrate scrolling */
    unsigned long i;
    for (i = 0; i < 64000; i++) {
        framebuffer[i] = (i % 256);
    }
}

void set_video_page(int page) {
    /* Hardware Page Flipping
       Sets the active display page
       page: 0 or 1 (assuming 32KB pages in 64KB video memory) */
    unsigned int offset = page * 16384; /* 32KB pages (16384 words) */
    set_crtc_start(offset);
}

void blit_page(int page) {
    /* Blits framebuffer to a specific video page
       Useful for double buffering with hardware page flipping */
    unsigned char far *video_mem = (unsigned char far *)0xB8000000L;
    int x, y;
    int bank;
    unsigned int offset;
    unsigned char packed;
    unsigned int page_offset = page * 0x8000; /* 32KB offset */
    
    for (y = 0; y < 200; y++) {
        bank = y & 3;
        offset = page_offset + (bank * 0x2000) + ((y / 4) * 160);
        for (x = 0; x < 320; x += 2) {
            packed = (framebuffer[(unsigned long)y * 320 + x] << 4) | (framebuffer[(unsigned long)y * 320 + x + 1] & 0x0F);
            video_mem[offset + (x / 2)] = packed;
        }
    }
}

void wait_vsync(void) {
    /* Wait for Vertical Retrace
       Prevents tearing and is essential for raster effects
       Port 0x3DA, Bit 3: 1 = Vertical Retrace Active */
    while (inp(0x3DA) & 0x08); /* Wait for end of current retrace */
    while (!(inp(0x3DA) & 0x08)); /* Wait for start of next retrace */
}

void draw_raster_bars(int offset) {
    /* "Copper Bar" Effect
       Changes the palette color for Color 1 on every scanline
       Creates a smooth gradient background without using many colors
       WARNING: This function relies on precise timing and is experimental.
       It may not work correctly on all systems or emulators. */
    int line;
    int color_val;
    
    wait_vsync();
    
    _disable(); /* Disable interrupts to ensure precise timing */
    
    for (line = 0; line < 200; line++) {
        /* Wait for Horizontal Retrace (Port 0x3DA, Bit 0) */
        while (inp(0x3DA) & 1); 
        while (!(inp(0x3DA) & 1));
        
        color_val = (line + offset) % 16;
        
        /* Update Palette Register 1 (Color 1) directly via Port I/O
           1. Reset Attribute Controller Flip-Flop */
        inp(0x3DA); 
        /* 2. Write Index 1 (Bit 5=0 to allow CPU write) */
        outp(0x3C0, 0x01); 
        /* 3. Write Data (New Color) */
        outp(0x3C0, color_val); 
        /* 4. Re-enable Video (Index 1 + PAS Bit 5=1) */
        outp(0x3C0, 0x01 | 0x20); 
    }
    
    _enable();
    
    /* Restore Palette 1 to Blue (1) */
    inp(0x3DA);
    outp(0x3C0, 0x01);
    outp(0x3C0, 0x01);
    outp(0x3C0, 0x01 | 0x20);
}

void cycle_palette_bars(int offset) {
    /* Full Screen Color Cycling
       Updates all 16 palette colors per frame */
    int i;
    int color_val;
    
    wait_vsync();
    
    for (i = 0; i < 16; i++) {
        color_val = (i + offset) & 0x0F;
        
        /* Use Port I/O for speed (BIOS is too slow) */
        inp(0x3DA); /* Reset flip-flop */
        outp(0x3C0, i); /* Index i (Bit 5=0) */
        outp(0x3C0, color_val); /* Data */
        outp(0x3C0, i | 0x20); /* Re-enable Video immediately */
    }
}

void draw_rect_page(int x, int y, int w, int h, int color, int page) {
    /* Draws a filled rectangle directly to VRAM on a specific page
       Optimized for clearing/drawing UI elements on the back buffer */
    unsigned char far *video_mem = (unsigned char far *)0xB8000000L;
    int i, j;
    int draw_x, draw_y;
    int bank;
    unsigned int offset;
    unsigned int page_offset = page * 0x8000;
    unsigned char mask, val;
    
    for (j = 0; j < h; j++) {
        draw_y = y + j;
        if (draw_y < 0 || draw_y >= 200) continue;
        
        bank = draw_y & 3;
        offset = page_offset + (bank * 0x2000) + ((draw_y / 4) * 160);
        
        for (i = 0; i < w; i++) {
            draw_x = x + i;
            if (draw_x < 0 || draw_x >= 320) continue;
            
            /* 2 pixels per byte. Even x = high nibble, Odd x = low nibble.
               Byte offset = x / 2 */
            
            val = video_mem[offset + (draw_x / 2)];
            
            if ((draw_x & 1) == 0) { /* Even: High nibble */
                val &= 0x0F; /* Clear high */
                val |= (color << 4);
            } else { /* Odd: Low nibble */
                val &= 0xF0; /* Clear low */
                val |= (color & 0x0F);
            }
            
            video_mem[offset + (draw_x / 2)] = val;
        }
    }
}
