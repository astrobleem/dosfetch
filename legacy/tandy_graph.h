#ifndef TANDY_GRAPH_H
#define TANDY_GRAPH_H

#define CGA_4 4     // CGA 320x200 4-color
#define CGA_5 5     // CGA 320x200 4-color (BW)
#define CGA_6 6     // CGA 640x200 2-color
#define MDA_7 7     // MDA 80x25 Monochrome Text
#define LOW_16 8    // Tandy Mode 8 (160x200, 16 colors)
#define MEDIUM_16 9 // Tandy Mode 9 (320x200, 16 colors)
#define HIGH_4 0x0A // Tandy Mode 0Ah (640x200, 4 colors)

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

typedef struct {
    int x, y;
    int dx, dy;
    int color;
    int active;
} Sprite;

// Function Prototypes
void mode(int m);
void point(int x, int y, int color);
void box(int x1, int y1, int x2, int y2, int color);
void line(int x1, int y1, int x2, int y2, int color);
void cls(void);
void gotoxy(int x, int y);

// Framebuffer
extern unsigned char framebuffer[128000];
void clear_framebuffer(int color);
void set_pixel(int x, int y, int color);
void blit(void);

// Sprites & Backgrounds
void draw_sprite_fast(int x, int y, int color, const unsigned short *data);
void draw_background(int scroll_x, int scroll_y);

// High Res (640x200)
void blit_high(void);
void draw_sprite_high(int x, int y, int color, const unsigned short *data);
void draw_background_high(int scroll_x, int scroll_y);

// Low Res (160x200)
void blit_low(void);
void draw_sprite_low(int x, int y, int color, const unsigned short *data);
void draw_background_low(int scroll_x, int scroll_y);

// CGA Modes
void blit_cga_4(void); // Modes 4 & 5
void blit_cga_6(void); // Mode 6
void draw_background_cga(int scroll_x, int scroll_y); // CGA specific background

// Mode 7 (Software Affine)
void draw_mode7_background(int angle, int cx, int cy);

// New Modes
void draw_wireframe_cube(int angle_x, int angle_y, int angle_z, int scale);
void draw_parallax_background(int scroll_x);
void draw_palette_cycling_demo(int frame);
void draw_sprite_scaled(int x, int y, int color, int scale, const unsigned short *data);
int check_sprite_collision(Sprite *s1, Sprite *s2);

// Mouse
int mouse_init(void);
void mouse_show(void);
void mouse_hide(void);
int mouse_status(int *x, int *y);

// Mode H
void set_crtc_start(unsigned int offset);
void draw_hardware_scrolling_demo(void);

// Mode I
void set_video_page(int page);
void blit_page(int page);
void wait_vsync(void);
void draw_rect_page(int x, int y, int w, int h, int color, int page);

// Mode J
void draw_copper_bars(int speed);

// Music
void init_music(void);
void cleanup_music(void);

// Speech
void play_pcm_sample(unsigned char *data, int length, int sample_rate);
void generate_robot_voice(unsigned char *buffer, int length);

#endif
