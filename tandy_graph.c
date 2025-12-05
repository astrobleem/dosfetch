#include <i86.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <dos.h>
#include "tandy_graph.h"

void mode(int m) {
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = (unsigned char)m;
    int86(0x10, &regs, &regs);
}

void point(int x, int y, int color) {
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
            point(x, y, color);
        }
    }
}

void line(int x1, int y1, int x2, int y2, int color) {
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
    // Clear screen by drawing a box over it
    // This is slower than BIOS scroll but safer for graphics modes initially
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

// Framebuffer (640x200 bytes max)
unsigned char framebuffer[128000];

void clear_framebuffer(int color) {
    memset(framebuffer, color, 128000);
}

void set_pixel(int x, int y, int color) {
    if (x >= 0 && x < 320 && y >= 0 && y < 200) {
        framebuffer[y * 320 + x] = (unsigned char)color;
    }
}

void blit(void) {
    // Tandy Mode 9 Video Memory starts at 0xB800:0000
    unsigned char far *video_mem = (unsigned char far *)0xB8000000L;
    int x, y;
    int bank;
    unsigned int offset;
    unsigned char packed;
    
    // Tandy Mode 9 (320x200, 16 colors) uses a unique 4-way interleaved memory layout.
    // Unlike CGA (2-way) or VGA (planar), the scanlines are distributed across 4 banks
    // of 8KB (0x2000) each.
    //
    // Bank 0 (Offset 0x0000): Lines 0, 4, 8, 12...
    // Bank 1 (Offset 0x2000): Lines 1, 5, 9, 13...
    // Bank 2 (Offset 0x4000): Lines 2, 6, 10, 14...
    // Bank 3 (Offset 0x6000): Lines 3, 7, 11, 15...
    //
    // Pixels are packed 2 per byte (4 bits per pixel).
    // High nibble = Even X pixel
    // Low nibble  = Odd X pixel
    
    for (y = 0; y < 200; y++) {
        // Determine which bank this scanline belongs to (0-3)
        bank = y & 3;
        
        // Calculate offset within the bank.
        // Each bank contains every 4th line, so we divide y by 4.
        // Each line is 160 bytes wide (320 pixels / 2 pixels per byte).
        // Base offset for the bank is bank * 0x2000.
        offset = (bank * 0x2000) + ((y / 4) * 160);
        
        for (x = 0; x < 320; x += 2) {
            // Pack 2 pixels from the linear framebuffer into 1 byte for video memory.
            // Pixel at x goes to high nibble (bits 7-4).
            // Pixel at x+1 goes to low nibble (bits 3-0).
            packed = (framebuffer[y * 320 + x] << 4) | (framebuffer[y * 320 + x + 1] & 0x0F);
            
            // Write to video memory
            video_mem[offset + (x / 2)] = packed;
        }
    }
}

void draw_sprite_fast(int x, int y, int color, const unsigned short *data) {
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
            
            if (row_data & (0x8000 >> col)) {
                framebuffer[py * 320 + px] = (unsigned char)color;
            }
        }
    }
}

void draw_background(int scroll_x, int scroll_y) {
    int x, y;
    int col;
    // Draw a scrolling grid pattern
    for (y = 0; y < 200; y++) {
        for (x = 0; x < 320; x++) {
            // Simple XOR pattern for "tech" look
            if (((x + scroll_x) ^ (y + scroll_y)) & 32) {
                col = DARKGRAY;
            } else {
                col = BLACK;
            }
            framebuffer[y * 320 + x] = (unsigned char)col;
        }
    }
}

void blit_high(void) {
    unsigned char far *video_mem = (unsigned char far *)0xB8000000L;
    int x, y;
    int bank;
    unsigned int offset;
    unsigned char packed;
    
    // Mode 0Ah: 640x200, 4 colors (2 bits/pixel)
    // 4-way interleave (same as Mode 9)
    
    for (y = 0; y < 200; y++) {
        bank = y & 3;
        offset = (bank * 0x2000) + ((y / 4) * 160);
        
        for (x = 0; x < 640; x += 4) {
            // Pack 4 pixels into 1 byte
            // Pixels are 0-3 (2 bits each)
            // x   -> bits 7-6
            // x+1 -> bits 5-4
            // x+2 -> bits 3-2
            // x+3 -> bits 1-0
            
            packed =  (framebuffer[y * 640 + x]     & 0x03) << 6;
            packed |= (framebuffer[y * 640 + x + 1] & 0x03) << 4;
            packed |= (framebuffer[y * 640 + x + 2] & 0x03) << 2;
            packed |= (framebuffer[y * 640 + x + 3] & 0x03);
            
            video_mem[offset + (x / 4)] = packed;
        }
    }
}

void draw_sprite_high(int x, int y, int color, const unsigned short *data) {
    int row, col;
    unsigned short row_data;
    int px, py;
    
    // Draw sprite in high res (640 width)
    
    for (row = 0; row < 16; row++) {
        py = y + row;
        if (py < 0 || py >= 200) continue;
        
        row_data = data[row];
        for (col = 0; col < 16; col++) {
            px = x + col;
            if (px < 0 || px >= 640) continue;
            
            if (row_data & (0x8000 >> col)) {
                framebuffer[py * 640 + px] = (unsigned char)color;
            }
        }
    }
}

void draw_background_high(int scroll_x, int scroll_y) {
    int x, y;
    int col;
    // Draw a scrolling grid pattern (640 width)
    for (y = 0; y < 200; y++) {
        for (x = 0; x < 640; x++) {
            // Simple XOR pattern
            if (((x + scroll_x) ^ (y + scroll_y)) & 32) {
                col = 1; // Cyan/White depending on palette
            } else {
                col = 0; // Black
            }
            framebuffer[y * 640 + x] = (unsigned char)col;
        }
    }
}

void blit_low(void) {
    unsigned char far *video_mem = (unsigned char far *)0xB8000000L;
    int x, y;
    int bank;
    unsigned int offset;
    unsigned char packed;
    
    // Mode 08h: 160x200, 16 colors
    // 2-way interleave (Standard CGA)
    // Bank 0: Even lines
    // Bank 1: Odd lines
    
    for (y = 0; y < 200; y++) {
        bank = y & 1;
        offset = (bank * 0x2000) + ((y / 2) * 80);
        
        for (x = 0; x < 160; x += 2) {
            // Pack 2 pixels into 1 byte (same as Mode 9)
            // High nibble = x
            // Low nibble  = x+1
            
            packed = (framebuffer[y * 160 + x] << 4) | (framebuffer[y * 160 + x + 1] & 0x0F);
            
            video_mem[offset + (x / 2)] = packed;
        }
    }
}

void draw_sprite_low(int x, int y, int color, const unsigned short *data) {
    int row, col;
    unsigned short row_data;
    int px, py;
    
    // Draw sprite in low res (160 width)
    
    for (row = 0; row < 16; row++) {
        py = y + row;
        if (py < 0 || py >= 200) continue;
        
        row_data = data[row];
        for (col = 0; col < 16; col++) {
            px = x + col;
            if (px < 0 || px >= 160) continue;
            
            if (row_data & (0x8000 >> col)) {
                framebuffer[py * 160 + px] = (unsigned char)color;
            }
        }
    }
}

void draw_background_low(int scroll_x, int scroll_y) {
    int x, y;
    int col;
    // Draw a scrolling grid pattern (160 width)
    for (y = 0; y < 200; y++) {
        for (x = 0; x < 160; x++) {
            // Simple XOR pattern
            if (((x + scroll_x) ^ (y + scroll_y)) & 16) { // Smaller bit for lower res
                col = DARKGRAY;
            } else {
                col = BLACK;
            }
            framebuffer[y * 160 + x] = (unsigned char)col;
        }
    }
}

void blit_cga_4(void) {
    unsigned char far *video_mem = (unsigned char far *)0xB8000000L;
    int x, y;
    int bank;
    unsigned int offset;
    unsigned char packed;
    
    // Mode 4/5: 320x200, 4 colors
    // 2-way interleave
    
    for (y = 0; y < 200; y++) {
        bank = y & 1;
        offset = (bank * 0x2000) + ((y / 2) * 80);
        
        for (x = 0; x < 320; x += 4) {
            // Pack 4 pixels into 1 byte (2 bits each)
            // x   -> bits 7-6
            // x+1 -> bits 5-4
            // x+2 -> bits 3-2
            // x+3 -> bits 1-0
            
            packed =  (framebuffer[y * 320 + x]     & 0x03) << 6;
            packed |= (framebuffer[y * 320 + x + 1] & 0x03) << 4;
            packed |= (framebuffer[y * 320 + x + 2] & 0x03) << 2;
            packed |= (framebuffer[y * 320 + x + 3] & 0x03);
            
            video_mem[offset + (x / 4)] = packed;
        }
    }
}

void blit_cga_6(void) {
    unsigned char far *video_mem = (unsigned char far *)0xB8000000L;
    int x, y;
    int bank;
    unsigned int offset;
    unsigned char packed;
    int i;
    
    // Mode 6: 640x200, 2 colors (B&W)
    // 2-way interleave
    
    for (y = 0; y < 200; y++) {
        bank = y & 1;
        offset = (bank * 0x2000) + ((y / 2) * 80);
        
        for (x = 0; x < 640; x += 8) {
            // Pack 8 pixels into 1 byte (1 bit each)
            packed = 0;
            for(i=0; i<8; i++) {
                if (framebuffer[y * 640 + x + i] & 0x01) {
                    packed |= (0x80 >> i);
                }
            }
            video_mem[offset + (x / 8)] = packed;
        }
    }
}

void draw_background_cga(int scroll_x, int scroll_y) {
    int x, y;
    int col;
    // Draw a scrolling grid pattern (320 width) for CGA
    for (y = 0; y < 200; y++) {
        for (x = 0; x < 320; x++) {
            // Simple XOR pattern
            if (((x + scroll_x) ^ (y + scroll_y)) & 32) {
                col = 1; // Color 1 (Cyan/Magenta/White depending on palette)
            } else {
                col = 0; // Black
            }
            framebuffer[y * 320 + x] = (unsigned char)col;
        }
    }
}

// Fixed point math constants
#define FP_SHIFT 8
#define FP_ONE (1 << FP_SHIFT)
#define PI 3.14159265359

int sin_table[256];
int cos_table[256];
int tables_initialized = 0;

void init_math_tables(void) {
    int i;
    for (i = 0; i < 256; i++) {
        // Map 0-255 to 0-2PI
        double angle = (double)i * 2.0 * PI / 256.0;
        sin_table[i] = (int)(sin(angle) * 256.0);
        cos_table[i] = (int)(cos(angle) * 256.0);
    }
    tables_initialized = 1;
}

void draw_mode7_background(int angle, int cx, int cy) {
    int x, y;
    int horizon = 30;
    int space_z;
    int step_x, step_y;
    long u, v; 
    int tx, ty, color;
    unsigned char *line_ptr;
    int cos_a, sin_a;
    long wx_left, wy_left;
    long u_start, v_start;
    int scale;
    
    if (!tables_initialized) init_math_tables();
    
    // Get rotation values
    angle = angle & 0xFF; // Clamp to 0-255
    cos_a = cos_table[angle];
    sin_a = sin_table[angle];
    
    // Clear top part (sky)
    memset(framebuffer, 0, 320 * horizon); // Black sky
    
    // Render floor
    for (y = horizon; y < 200; y++) {
        if (y == horizon) continue;
        
        // Distance calculation
        space_z = 2000 / (y - horizon); 
        
        // Scale factor
        scale = space_z >> 4;
        
        // Calculate steps based on rotation
        step_x = (scale * cos_a) >> 8;
        step_y = (scale * sin_a) >> 8;
        
        // Calculate start point (Screen X = -160)
        wx_left = -160 * scale;
        wy_left = space_z;
        
        // Rotate start point
        u = (wx_left * cos_a - wy_left * sin_a) + ((long)cx << 8);
        v = (wx_left * sin_a + wy_left * cos_a) + ((long)cy << 8);
        
        line_ptr = &framebuffer[y * 320];
        
        for (x = 0; x < 320; x++) {
            tx = (u >> 8) & 0xFF;
            ty = (v >> 8) & 0xFF;
            
            color = ((tx ^ ty) & 0x20) ? 10 : 2; 
            *line_ptr++ = (unsigned char)color;
            
            u += step_x;
            v += step_y;
        }
    }
}

void line_fast(int x1, int y1, int x2, int y2, int color) {
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;) {
        set_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void draw_wireframe_cube(int angle_x, int angle_y, int angle_z, int scale) {
    // Cube vertices (x, y, z) centered at 0
    // Scaled by 64 for fixed point
    int vertices[8][3] = {
        {-64, -64, -64}, {64, -64, -64}, {64, 64, -64}, {-64, 64, -64},
        {-64, -64, 64}, {64, -64, 64}, {64, 64, 64}, {-64, 64, 64}
    };
    
    // Edges connecting vertices
    int edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0}, // Front face
        {4,5}, {5,6}, {6,7}, {7,4}, // Back face
        {0,4}, {1,5}, {2,6}, {3,7}  // Connecting lines
    };
    
    int i;
    int p2d[8][2]; // Projected 2D points
    int x, y, z;
    int rx, ry, rz;
    int cx, cy, cz; // Cos/Sin values
    int dist = 256;
    int z_off = 200;
    
    if (!tables_initialized) init_math_tables();
    
    // Clamp angles
    angle_x &= 0xFF; angle_y &= 0xFF; angle_z &= 0xFF;
    
    // Clear screen (black)
    memset(framebuffer, 0, 64000);
    
    // Rotate and Project
    for (i = 0; i < 8; i++) {
        x = vertices[i][0];
        y = vertices[i][1];
        z = vertices[i][2];
        
        // Rotate X
        // y' = y*cos - z*sin
        // z' = y*sin + z*cos
        cx = cos_table[angle_x]; cy = sin_table[angle_x]; // Reuse cy as sin for convenience
        ry = (y * cx - z * cy) >> 8;
        rz = (y * cy + z * cx) >> 8;
        y = ry; z = rz;
        
        // Rotate Y
        // x' = x*cos + z*sin
        // z' = -x*sin + z*cos
        cx = cos_table[angle_y]; cy = sin_table[angle_y];
        rx = (x * cx + z * cy) >> 8;
        rz = (-x * cy + z * cx) >> 8;
        x = rx; z = rz;
        
        // Rotate Z
        // x' = x*cos - y*sin
        // y' = x*sin + y*cos
        cx = cos_table[angle_z]; cy = sin_table[angle_z];
        rx = (x * cx - y * cy) >> 8;
        ry = (x * cy + y * cx) >> 8;
        x = rx; y = ry;
        
        // Scale
        x = (x * scale) >> 6; // Scale is roughly 64 for 1:1
        y = (y * scale) >> 6;
        z = (z * scale) >> 6;
        
        // Project
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
        
        // Color 10 (Light Green)
        line_fast(p2d[p1][0], p2d[p1][1], p2d[p2][0], p2d[p2][1], 10);
    }
}

void draw_parallax_background(int scroll_x) {
    int x, y;
    int layer1_x, layer2_x, layer3_x;
    int h;
    int idx;
    
    if (!tables_initialized) init_math_tables();
    
    // Clear Sky (Blue - Color 1)
    memset(framebuffer, 1, 320 * 120); 
    
    // Layer 1: Distant Mountains (Slowest)
    // Speed: scroll_x / 4
    layer1_x = scroll_x >> 2;
    for (x = 0; x < 320; x++) {
        // Frequency ~2
        idx = ((x + layer1_x) * 2) & 0xFF;
        // Amplitude 20
        h = 80 + ((sin_table[idx] * 20) >> 8);
        
        for (y = h; y < 200; y++) {
             framebuffer[y * 320 + x] = 5; // Magenta
        }
    }
    
    // Layer 2: Near Hills (Medium)
    // Speed: scroll_x / 2
    layer2_x = scroll_x >> 1;
    for (x = 0; x < 320; x++) {
        // Frequency ~3
        idx = ((x + layer2_x) * 3) & 0xFF;
        // Amplitude 15
        h = 120 + ((sin_table[idx] * 15) >> 8);
        
        for (y = h; y < 200; y++) {
             framebuffer[y * 320 + x] = 2; // Green
        }
    }
    
    // Layer 3: Ground (Fastest)
    // Speed: scroll_x
    layer3_x = scroll_x;
    for (x = 0; x < 320; x++) {
        h = 160;
        for (y = h; y < 200; y++) {
             // Checkerboard
             if (((x + layer3_x) ^ y) & 16) {
                 framebuffer[y * 320 + x] = 10; // Light Green
             } else {
                 framebuffer[y * 320 + x] = 2; // Green
             }
        }
    }
    
    // Sun (Static)
    for (y = 20; y < 40; y++) {
        for (x = 280; x < 300; x++) {
            if ((x-290)*(x-290) + (y-30)*(y-30) < 80) {
                framebuffer[y * 320 + x] = 14; // Yellow
            }
        }
    }
}

// Letter Data (8x8)
unsigned char font_t[8] = {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18};
unsigned char font_a[8] = {0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x66};
unsigned char font_n[8] = {0x66, 0x76, 0x7E, 0x7E, 0x6E, 0x66, 0x66, 0x66};
unsigned char font_d[8] = {0x7C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x7C};
unsigned char font_y[8] = {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x18};

void draw_char_scaled(int x, int y, int color, int scale, unsigned char *data) {
    int row, col;
    int px, py;
    int i, j;
    
    for (row = 0; row < 8; row++) {
        for (col = 0; col < 8; col++) {
            if (data[row] & (0x80 >> col)) {
                // Draw scaled pixel
                for (i = 0; i < scale; i++) {
                    for (j = 0; j < scale; j++) {
                        px = x + col * scale + j;
                        py = y + row * scale + i;
                        if (px >= 0 && px < 320 && py >= 0 && py < 200) {
                            framebuffer[py * 320 + px] = (unsigned char)color;
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
    
    // Clear screen
    memset(framebuffer, 0, 64000);
    
    // Draw T A N D Y
    draw_char_scaled(start_x, y, base_color, scale, font_t);
    draw_char_scaled(start_x + spacing, y, (base_color + 1) % 15 + 1, scale, font_a);
    draw_char_scaled(start_x + spacing * 2, y, (base_color + 2) % 15 + 1, scale, font_n);
    draw_char_scaled(start_x + spacing * 3, y, (base_color + 3) % 15 + 1, scale, font_d);
    draw_char_scaled(start_x + spacing * 4, y, (base_color + 4) % 15 + 1, scale, font_y);
}

void draw_sprite_scaled(int x, int y, int color, int scale, const unsigned short *data) {
    // scale is fixed point 8.8 (256 = 1.0)
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
                framebuffer[(y + py) * 320 + (x + px)] = (unsigned char)color;
            }
        }
    }
}

int check_sprite_collision(Sprite *s1, Sprite *s2) {
    // Simple AABB Collision
    // Sprites are 16x16
    if (s1->x < s2->x + 16 &&
        s1->x + 16 > s2->x &&
        s1->y < s2->y + 16 &&
        s1->y + 16 > s2->y) {
        return 1;
    }
    return 0;
}

int mouse_init(void) {
    union REGS r;
    r.w.ax = 0;
    int86(0x33, &r, &r);
    return r.w.ax;
}

void mouse_show(void) {
    union REGS r;
    r.w.ax = 1;
    int86(0x33, &r, &r);
}

void mouse_hide(void) {
    union REGS r;
    r.w.ax = 2;
    int86(0x33, &r, &r);
}

int mouse_status(int *x, int *y) {
    union REGS r;
    r.w.ax = 3;
    int86(0x33, &r, &r);
    *x = r.w.cx;
    *y = r.w.dx;
    return r.w.bx; // Button status
}


void set_crtc_start(unsigned int offset) {
    // CRTC Index: 0x3D4
    // CRTC Data:  0x3D5
    // Reg 0x0C: Start Address High
    // Reg 0x0D: Start Address Low
    
    unsigned char high = (offset >> 8) & 0xFF;
    unsigned char low = offset & 0xFF;
    
    // Disable interrupts to ensure atomicity? Not strictly necessary for demo but good practice
    _disable();
    
    outp(0x3D4, 0x0C);
    outp(0x3D5, high);
    
    outp(0x3D4, 0x0D);
    outp(0x3D5, low);
    
    // Wait for VSync to apply? 
    // Changes take effect immediately but might tear if not timed.
    // For this demo, immediate is fine to show speed.
    
    _enable();
}

void draw_hardware_scrolling_demo(void) {
    int x, y;
    // Draw a pattern that makes scrolling obvious
    // A diagonal grid or numbered lines
    
    // Clear
    memset(framebuffer, 1, 64000); // Blue background
    
    for (y = 0; y < 200; y++) {
        for (x = 0; x < 320; x++) {
            // Diagonal Lines
            if ((x + y) % 32 == 0) {
                framebuffer[y * 320 + x] = 15; // White
            }
            // Horizontal Lines every 20 pixels
            if (y % 20 == 0) {
                framebuffer[y * 320 + x] = 14; // Yellow
            }
            // Vertical Lines every 20 pixels
            if (x % 20 == 0) {
                framebuffer[y * 320 + x] = 14; // Yellow
            }
        }
    }
    
    // Draw some text or markers?
    // We can't easily draw text with our current font system at arbitrary positions efficiently enough to fill the screen
    // But the grid should be enough.
}

void set_video_page(int page) {
    // Page 0: Offset 0
    // Page 1: Offset 0x4000 (16384 words) = 32768 bytes
    // We use 0x8000 bytes per page to be safe and align with 32KB banks.
    unsigned int offset = (page == 1) ? 0x4000 : 0;
    set_crtc_start(offset);
}

void blit_page(int page) {
    unsigned char far *video_mem = (unsigned char far *)0xB8000000L;
    unsigned int page_offset = (page == 1) ? 0x8000 : 0; // 32768 bytes
    int x, y;
    int bank;
    unsigned int offset;
    unsigned char packed;
    
    // We must manually pack and interleave, just like blit()
    // But we add page_offset to the final address.
    
    for (y = 0; y < 200; y++) {
        bank = y & 3;
        offset = (bank * 0x2000) + ((y / 4) * 160);
        
        offset += page_offset;
        
        for (x = 0; x < 320; x += 2) {
            packed = (framebuffer[y * 320 + x] << 4) | (framebuffer[y * 320 + x + 1] & 0x0F);
            video_mem[offset + (x / 2)] = packed;
        }
    }
}

void wait_vsync(void) {
    // Port 0x3DA - Input Status Register 1
    // Bit 3: Vertical Retrace (1 = Active)
    
    // Wait for end of current retrace (if we are in one)
    while (inp(0x3DA) & 0x08);
    
    // Wait for start of new retrace
    while (!(inp(0x3DA) & 0x08));
}

void draw_rect_page(int x, int y, int w, int h, int color, int page) {
    unsigned char far *video_mem = (unsigned char far *)0xB8000000L;
    unsigned int page_offset = (page == 1) ? 0x8000 : 0;
    int i, j;
    int px, py;
    int bank;
    unsigned int offset;
    unsigned char mask, val;
    
    for (j = 0; j < h; j++) {
        py = y + j;
        if (py < 0 || py >= 200) continue;
        
        bank = py & 3;
        offset = (bank * 0x2000) + ((py / 4) * 160) + page_offset;
        
        for (i = 0; i < w; i++) {
            px = x + i;
            if (px < 0 || px >= 320) continue;
            
            // Read-Modify-Write
            // High nibble = even pixel, Low nibble = odd pixel
            // offset + (px / 2)
            
            val = video_mem[offset + (px / 2)];
            
            if (px % 2 == 0) {
                // High nibble
                val &= 0x0F; // Clear high
                val |= (color << 4);
            } else {
                // Low nibble
                val &= 0xF0; // Clear low
                val |= (color & 0x0F);
            }
            
            video_mem[offset + (px / 2)] = val;
        }
    }
}

void draw_copper_bars(int speed) {
    // This function runs the copper bar effect for ONE FRAME.
    // It must be called in a loop.
    
    static int offset = 0;
    int i;
    union REGS r;
    
    // Wait for VSync to ensure smooth update
    wait_vsync();
    
    offset += speed;
    
    // Cycle all 16 palette registers
    for (i = 0; i < 16; i++) {
        // Calculate color for this index
        // Create a rolling gradient effect
        // Use bitwise AND for modulo 16 to handle negative numbers correctly
        int color_val = (i + offset) & 0x0F;
        
        // Set Palette Register 'i' to 'color_val' using BIOS
        // This is safe and works on all emulators
        r.h.ah = 0x10;
        r.h.al = 0x00;
        r.h.bl = i;      // Palette Index
        r.h.bh = color_val; // Color Value
        int86(0x10, &r, &r);
    }
}

// ==========================================
// Background Music Implementation (INT 1C)
// ==========================================

// Global Music State
void (__interrupt __far *old_handler)();
volatile int music_tick_counter = 0;
volatile int current_note_index = 0;
volatile int note_duration_counter = 0;
volatile int music_playing = 0;
volatile int is_active_handler = 0; // Safety flag to prevent "Ghost" handlers

// Simple Music Data (Frequency, Duration in Ticks)
// 0 Frequency = Silence
typedef struct {
    int freq;
    int duration;
} Note;

// A simple tune (Arpeggios)
Note song[] = {
    {261, 4}, {329, 4}, {392, 4}, {523, 8}, // C Major
    {261, 4}, {329, 4}, {392, 4}, {523, 8},
    {293, 4}, {349, 4}, {440, 4}, {587, 8}, // D Minor
    {293, 4}, {349, 4}, {440, 4}, {587, 8},
    {329, 4}, {415, 4}, {493, 4}, {659, 8}, // E Major
    {329, 4}, {415, 4}, {493, 4}, {659, 8},
    {261, 4}, {329, 4}, {392, 4}, {523, 16}, // C Major End
    {0, 0} // End of song
};

void silence_all_voices(void) {
    // Silence all 3 Tone Channels and 1 Noise Channel
    outp(0xC0, 0x9F); // Voice 0 Off
    outp(0xC0, 0xBF); // Voice 1 Off
    outp(0xC0, 0xDF); // Voice 2 Off
    outp(0xC0, 0xFF); // Noise Off
}

void play_tandy_frequency(int freq, int volume) {
    // Tandy SN76496 Sound Chip (Port 0xC0)
    // Frequency = 3579545 / (32 * freq)
    // 10-bit counter value
    
    unsigned int count;
    unsigned char byte1, byte2;
    
    if (freq == 0) {
        // Silence Voice 0
        outp(0xC0, 0x9F); // 1001 1111 (Voice 0, Attenuation 15/Off)
        return;
    }
    
    count = 3579545L / (32L * freq);
    if (count > 1023) count = 1023;
    
    // Byte 1: 1ccf ffff (1, Channel 0, Freq Low 4 bits)
    byte1 = 0x80 | (count & 0x0F);
    // Byte 2: 00ff ffff (0, Freq High 6 bits)
    byte2 = (count >> 4) & 0x3F;
    
    outp(0xC0, byte1);
    outp(0xC0, byte2);
    
    // Set Volume (0=Loudest, 15=Silent)
    // 1001 vvvv (1, Channel 0, Attenuation)
    outp(0xC0, 0x90 | (volume & 0x0F));
}

void __interrupt __far timer_handler(void) {
    // This function is called 18.2 times per second
    
    // SAFETY CHECK: If this handler is a "Ghost" (from a previous crashed run),
    // is_active_handler will likely be 0 (cleared by cleanup) or garbage.
    // We only play if we are the ACTIVE handler.
    if (is_active_handler && music_playing) {
        if (note_duration_counter <= 0) {
            // Load next note
            int freq = song[current_note_index].freq;
            int dur = song[current_note_index].duration;
            
            if (dur == 0) {
                // End of song, loop
                current_note_index = 0;
                freq = song[0].freq;
                dur = song[0].duration;
            }
            
            play_tandy_frequency(freq, 2); // Volume 2 (Pretty loud)
            note_duration_counter = dur;
            current_note_index++;
        } else {
            note_duration_counter--;
        }
    }
    
    // Call the original handler to keep system time ticking
    _chain_intr(old_handler);
}

void init_music(void) {
    if (music_playing) return;
    
    // Silence everything first
    silence_all_voices();
    
    // Save old interrupt vector
    old_handler = _dos_getvect(0x1C);
    
    // Reset state
    current_note_index = 0;
    note_duration_counter = 0;
    music_playing = 1;
    is_active_handler = 1; // Mark this handler as ACTIVE
    
    // Set new interrupt vector
    _dos_setvect(0x1C, timer_handler);
}

void cleanup_music(void) {
    if (!music_playing) return;
    
    // Mark this handler as INACTIVE immediately
    is_active_handler = 0;
    music_playing = 0;
    
    // Restore old interrupt vector
    _dos_setvect(0x1C, old_handler);
    
    // Silence the chip
    silence_all_voices();
}

// ==========================================
// Speech Synthesis (PCM Playback)
// ==========================================

void play_pcm_sample(unsigned char *data, int length, int sample_rate) {
    // Emulate a 4-bit DAC using the Volume Register of Voice 0
    // Data is expected to be 4-bit (0-15)
    
    int i;
    int delay_loops;
    
    // 1. Set Voice 0 to 0Hz (DC Offset) or High Frequency
    // Setting it to a very low frequency (or 0) essentially makes it a DC output
    // controlled by the volume register.
    // 1000 0000 0000 0000 (Voice 0, Freq 0)
    outp(0xC0, 0x80);
    outp(0xC0, 0x00);
    
    // Calculate rough delay loop for sample rate
    // This is highly dependent on CPU speed (cycles per loop)
    // On a fast DOSBox (3000 cycles), a loop is maybe 10-20 cycles?
    // Let's try a heuristic.
    delay_loops = 10000 / sample_rate; 
    if (delay_loops < 1) delay_loops = 1;
    
    // 2. Play Loop
    for (i = 0; i < length; i++) {
        unsigned char sample = data[i] & 0x0F;
        
        // Invert sample because 0=Loudest, 15=Silent in SN76496
        // We want 15=Loudest, 0=Silent
        sample = 15 - sample;
        
        // Write Volume: 1001 vvvv
        outp(0xC0, 0x90 | sample);
        
        // Delay
        // Use a volatile variable to prevent optimization
        {
            volatile int d;
            for (d = 0; d < delay_loops; d++);
        }
    }
    
    // Silence
    outp(0xC0, 0x9F);
}

void generate_robot_voice(unsigned char *buffer, int length) {
    // Generate a synthetic "Robot" sound
    // AM/FM modulated square wave
    int i;
    for (i = 0; i < length; i++) {
        // Carrier: Fast sine/square
        int carrier = (i % 20) < 10 ? 15 : 0;
        
        // Modulator: Slow sine
        double mod = sin((double)i * 0.05); // Slow LFO
        
        // Apply modulation
        int sample = (int)(carrier * ((mod + 1.0) / 2.0));
        
        // Add some noise/glitch
        if ((i % 50) == 0) sample = rand() % 16;
        
        buffer[i] = (unsigned char)(sample & 0x0F);
    }
}
