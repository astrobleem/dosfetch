#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <i86.h>
#include <conio.h>
#include "graphics.h"
#include "sound.h"
#include "input.h"

// Color definitions are now in grfx/graphics.h
// #define BLACK 0
// ...


// Define the I/O ports for the 8253 Timer
#define TIMER_CONTROL_PORT 0x43
#define TIMER_COUNTER_0 0x40
#define TIMER_COUNTER_1 0x41
#define TIMER_COUNTER_2 0x42

// Define the I/O ports for the SN76496
#define SN76496_PORT_0 0xC0
#define SN76496_PORT_1 0xC1
#define SN76496_PORT_2 0xC2
#define SN76496_PORT_3 0xC3
#define SN76496_PORT_4 0xC4
#define SN76496_PORT_5 0xC5
#define SN76496_PORT_6 0xC6
#define SN76496_PORT_7 0xC7


// Function prototypes
unsigned char cmos(unsigned char cmd);
void base_memory(int compact);
void extended_memory(int compact);
void disksize(unsigned char disk, int compact);
void dosver(int compact);
void floppy(int compact);
void fpu(int compact);
void detect_tandy(int compact);
void detect_tandy_mode(int compact);
void detect_cpu(int compact);
void detect_cpu_speed(int compact);
unsigned long get_ticks(void);
int detect_sn76496(void);
void print_color_bars(int is_graphics);
void sn_set_tone(int channel, unsigned int period);
void sn_set_volume(int channel, unsigned int vol4);
void wait_ticks(unsigned int ticks);
void play_startup_sound(void);
unsigned int freq_to_period(unsigned int freq_hz);
int detect_dosbox(void);
char* get_dosbox_version(void);
void uptime(int compact);
void print_user_host(int compact);
void print_dosbox_logo(void);
void draw_sprite(int x, int y, int color, int type);

// Detect if running in DOSBox
int detect_dosbox(void) {
    return 1;
}

// Get DOSBox version string  
char* get_dosbox_version(void) {
    return getenv("DOSBOX");
}

// Display system uptime (time since midnight/boot)
void uptime(int compact) {
    unsigned long ticks = get_ticks();
    unsigned long seconds = ticks / 18;  // approx 18.2 ticks/sec
    unsigned int h, m, s;
    
    h = (unsigned int)(seconds / 3600);
    seconds -= (unsigned long)h * 3600;
    m = (unsigned int)(seconds / 60);
    s = (unsigned int)(seconds - m * 60);
    
    printf("%uh %um %us\n", h, m, s);
}

// Print User@Host header
void print_user_host(int compact) {
    char *user = getenv("USER");
    char *host = getenv("HOSTNAME");
    int len, i;
    
    // Fallbacks for DOS environment
    // Separator line
    len = strlen(user) + strlen(host) + 1;
    if (len > 78) len = 78; // Safety cap
    // In compact mode (40 cols), cap at 35
    if (compact && len > 35) len = 35;
    
    for(i=0; i<len; i++) printf("-");
    printf("\n");
}

// PSG Sound Functions (based on psgtest.c)
void sn_set_tone(int channel, unsigned int period) {
    unsigned char latch = 0x80 | ((channel & 3) << 5) | (period & 0x0F);
    unsigned char data = (unsigned char)((period >> 4) & 0x3F);
    outp(SN76496_PORT_0, latch);
    outp(SN76496_PORT_0, data);
}

void sn_set_volume(int channel, unsigned int vol4) {
    outp(SN76496_PORT_0, 0x90 | ((channel & 3) << 5) | (vol4 & 0x0F));
}

void wait_ticks(unsigned int ticks) {
    unsigned long start = get_ticks();
    while ((get_ticks() - start) < ticks) {
        /* spin */
    }
}

unsigned int freq_to_period(unsigned int freq_hz) {
    unsigned long n;
    if (freq_hz == 0) return 1023;
    n = (3579545UL / 32UL) / (unsigned long)freq_hz;
    if (n < 1) n = 1;
    if (n > 1023) n = 1023;
    return (unsigned int)n;
}

void play_startup_sound(void) {
    unsigned int v;
    
    sn_set_volume(0, 15);
    sn_set_volume(1, 15);
    sn_set_volume(2, 15);
    
    sn_set_tone(0, freq_to_period(262));  // C4
    sn_set_volume(0, 4);
    wait_ticks(4);
    
    sn_set_tone(1, freq_to_period(330));  // E4
    sn_set_volume(1, 4);
    wait_ticks(4);
    
    sn_set_tone(2, freq_to_period(392));  // G4
    sn_set_volume(2, 4);
    wait_ticks(7);
    
    wait_ticks(5);
    
    outp(SN76496_PORT_0, 0xE0 | 0x05);
    sn_set_volume(3, 6);
    wait_ticks(2);
    sn_set_volume(3, 15);
    
    for (v = 4; v <= 15; v++) {
        sn_set_volume(0, v);
        sn_set_volume(1, v);
        sn_set_volume(2, v);
        wait_ticks(1);
    }
    
    sn_set_volume(0, 15);
    sn_set_volume(1, 15);
    sn_set_volume(2, 15);
}

int detect_sn76496(void) {
    int detected = 0;
    unsigned char far *tandy_check = (unsigned char far *)0xFC000000L;
    
    sn_set_volume(0, 15);
    sn_set_volume(1, 15);
    sn_set_volume(2, 15);
    sn_set_volume(3, 15);
    
    if (*tandy_check == 0x21) {
        detected = 1;
    }
    
    return detected;
}

// 16x16 Tandy "T" Logo Bitmap (Sprite)
unsigned short tandy_logo_16x16[] = {
    0x0000, // ................
    0x7FFE, // .#############..
    0x7FFE, // .#############..
    0x7FFE, // .#############..
    0x0180, // .......##.......
    0x0180, // .......##.......
    0x0180, // .......##.......
    0x0180, // .......##.......
    0x0180, // .......##.......
    0x0180, // .......##.......
    0x0180, // .......##.......
    0x0180, // .......##.......
    0x0180, // .......##.......
    0x0180, // .......##.......
    0x0180, // .......##.......
    0x0000  // ................
};

unsigned short pcjr_logo_16x16[] = {
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0020, //          .
    0x0000, //
    0x7725, // ### ###  . # #
    0x5426, // # # #    . ##
    0x7424, // ### #    . #
    0x4424, // #   #    . #
    0x4724, // #   ###  . #
    0x0060, //         ##
    0x0000, 0x0000, 0x0000, 0x0000
};

void draw_sprite(int x, int y, int color, int type) {
    int row, col;
    unsigned short row_data;
    unsigned short *logo_data;
    
    if (type == 2) logo_data = pcjr_logo_16x16;
    else logo_data = tandy_logo_16x16;
    
    // Use Framebuffer for high performance
    // Only clear if it's the first draw or needed
    // clear_framebuffer(BLACK); 
    
    for (row = 0; row < 16; row++) {
        row_data = logo_data[row];
        for (col = 0; col < 16; col++) {
            if (row_data & (0x8000 >> col)) {
                set_pixel(x + col, y + row, color);
            }
        }
    }
    
    // Blit the framebuffer to video memory
    blit();
}

void print_dosbox_logo(void){
    printf("  ___   __  ____\n");
    printf(" |   \\ /  \\/ ___|\n");
    printf(" | |\\ |  . \\___ \\\n");
    printf(" |___/ \\__/\\____/\n");
    printf("   fetch\n\n");
    printf("  for MS-DOS\n\n");
}

void detect_tandy(int compact) {
    union REGS regs;
    struct SREGS sregs;
    // Check F000:C000 for the Tandy signature byte 0x21
    unsigned char far *bios_check = (unsigned char far *)0xFFFF000EL; // F000:FF0E (Model ID byte location varies)
    // Actually, the signature is often at F000:C000
    unsigned char far *tandy_check = (unsigned char far *)0xFC000000L; // F000:C000
    
    // Model ID at F000:FFFE is standard for PC detection
    unsigned char far *machine_id = MK_FP(0xF000, 0xFFFE);
    
    // Try BIOS Call INT 15h, AH=C0h (Get System Configuration) first
    // This is the most reliable method for later Tandys (TL/SL/RL)
    regs.w.ax = 0x1A00; // Wait, 1A00 is Read Real Time Clock? 
                        // Ah, previous code used 1A00? That might be a mistake or a specific check.
                        // Let's stick to the memory checks which are robust for 1000/A/EX/HX.
    int86(0x10, &regs, &regs); // INT 10h? That's video.
    
    // The previous code was a bit of a mix. Let's clarify:
    // 1. Check F000:C000 == 0x21 (Tandy 1000 Signature)
    // 2. Check F000:FFFE == 0xFF (PCjr)
    
    if (regs.h.bl == 0xFF) {
        if (*machine_id == 0xFD) {
            printf("IBM PCjr\n");
        } else {
            printf("Tandy 1000\n");
            
            regs.h.ah = 0xC0;
            int86x(0x15, &regs, &regs, &sregs);
            
            if (!regs.x.cflag) {
                unsigned char far *model_id = MK_FP(sregs.es, regs.x.bx + 2);
                if (*model_id == 0xFF) {
                    printf(" (SL/TL variant)");
                }
            }
        }
        return;
    }
    
    if (*machine_id == 0xFD) {
        printf("IBM PCjr\n");
        return;
    }
    
    if (*bios_check != 0xFF) {
        printf("Not an IBM PC compatible\n");
        return;
    }
    
    if (*tandy_check == 0x21) {
        printf("Tandy 1000 (ROM)\n");
        return;
    }
    
    printf("Non-Tandy system\n");
}

void detect_tandy_mode(int compact) {
    union REGS regs;
    
    regs.h.ah = 0x0F;
    int86(0x10, &regs, &regs);
    
    switch (regs.h.al) {
        case 0x00: printf("40x25 Text (Mode 0)\n"); break;
        case 0x01: printf("40x25 Color Text (Mode 1)\n"); break;
        case 0x02: printf("80x25 Text (Mode 2)\n"); break;
        case 0x03: printf("80x25 Color Text (Mode 3)\n"); break;
        case 0x04: 
            if (compact) printf("320x200x4 (CGA)\n");
            else printf("320x200 4-color Graphics (Mode 4/CGA)\n");
            break;
        case 0x05: printf("320x200 4-color Graphics BW (Mode 5)\n"); break;
        case 0x06: printf("640x200 2-color Graphics (Mode 6/CGA)\n"); break;
        case 0x07: printf("80x25 Monochrome Text (Mode 7/MDA)\n"); break;
        case 0x08: printf("160x200 16-color Graphics (Mode 8/PCjr)\n"); break;
        case 0x09: 
            if (compact) printf("320x200x16 (Tandy)\n");
            else printf("320x200 16-color Graphics (Mode 9/Tandy)\n");
            break;
        case 0x0A: printf("640x200 4-color Graphics (Mode A/Tandy)\n"); break;
        default: printf("Unknown Mode\n"); break;
    }
}

unsigned char cmos(unsigned char cmd) {
    outp(0x70, cmd);
    return inp(0x71);
}

void base_memory(int compact) {
    union REGS regs;
    int86(0x12, &regs, &regs);
    printf("%u KB\n", regs.w.ax);
}

void extended_memory(int compact) {
    union REGS regs;
    unsigned long mem = 0;

    regs.h.ah = 0x88;
    int86(0x15, &regs, &regs);

    if (regs.x.cflag) {
        printf("none\n");
    } else {
        regs.w.ax = 0xE801;
        int86(0x15, &regs, &regs);

        if (regs.x.cflag) {
            mem = (unsigned long)cmos(0x17) + 256UL * cmos(0x18);
            mem += (unsigned long)cmos(0x30) + 256UL * cmos(0x31);
        } else {
            mem = (unsigned long)regs.w.ax + (unsigned long)regs.w.bx * 64UL;
        }
        printf("%lu KB\n", mem);
    }
}

void disksize(unsigned char disk, int compact) {
    union REGS regs;
    unsigned long size, free;
    
    regs.h.ah = 0x36;
    regs.h.dl = disk;
    int86(0x21, &regs, &regs);

    if (regs.w.ax == 0xFFFF) {
        printf("Unknown\n");
    } else {
        size = (unsigned long)regs.w.bx * (unsigned long)regs.w.ax * (unsigned long)regs.w.cx;
        free = (unsigned long)regs.w.bx * (unsigned long)regs.w.ax * (unsigned long)regs.w.dx;
        if (compact) {
            printf("%lu/%lu KB (%lu%%)\n", free/1024, size/1024, (free*100)/size);
        } else {
            printf("%lu/%lu KB (%lu%% free)\n", free/1024, size/1024, (free*100)/size);
        }
    }
}

void dosver(int compact) {
    union REGS regs;
    int maj, min, ven;
    
    regs.h.ah = 0x30;
    int86(0x21, &regs, &regs);
    
    maj = regs.h.al;
    min = regs.h.ah;
    ven = regs.h.bh;

    if (ven == 0xFF) printf("MS-DOS %d.%d\n", maj, min);
    else if (ven == 0x00) printf("IBM DOS %d.%d\n", maj, min);
    else printf("Unknown DOS %d.%d\n", maj, min);
}

void floppy(int compact) {
    union REGS regs;
    int86(0x11, &regs, &regs);
    if (regs.w.ax & 0x0001) {
        int n = (regs.w.ax >> 6) & 0x03;
        printf("%d\n", n + 1);
    } else {
        printf("0\n");
    }
}

void fpu(int compact) {
    union REGS regs;
    int86(0x11, &regs, &regs);
    if (regs.w.ax & 0x0002) printf("YES\n");
    else printf("NO\n");
}

void detect_cpu(int compact) {
    unsigned int flags_lo, flags_hi;
    
    _asm {
        pushf
        pop ax
        mov flags_lo, ax
        mov ax, 0xF000
        push ax
        popf
        pushf
        pop ax
        mov flags_hi, ax
        push flags_lo
        popf
    }
    
    if ((flags_hi & 0xF000) == 0xF000) {
        printf("Intel 8086/8088\n");
    } else if ((flags_hi & 0xF000) == 0x0000) {
        printf("Intel 286\n");
    } else {
        printf("Intel 386+\n");
    }
}

void detect_cpu_speed(int compact) {
    unsigned long start, end;
    unsigned long loops = 0;
    double mhz;
    
    start = get_ticks();
    while (get_ticks() == start); 

    start = get_ticks();
    while (get_ticks() < start + 2) {
        loops++;
    }
    
    if (loops < 500) mhz = 4.77;
    else if (loops < 1000) mhz = 8.0;
    else if (loops < 2000) mhz = 12.0;
    else if (loops < 4000) mhz = 16.0;
    else if (loops < 8000) mhz = 20.0;
    else if (loops < 12000) mhz = 25.0;
    else if (loops < 16000) mhz = 33.0;
    else if (loops < 25000) mhz = 40.0;
    else if (loops < 35000) mhz = 50.0;
    else if (loops < 50000) mhz = 66.0;
    else mhz = 100.0;
    
    if (compact) printf("~%.2f MHz\n", mhz);
    else printf("~%.2f MHz (Estimated)\n", mhz);
}

unsigned long get_ticks(void) {
    unsigned long far *ticks = (unsigned long far *)0x0040006CL;
    return *ticks;
}

void print_color_bars(int is_graphics) {
    int width, height, y_start;
    int i;
    
    if (is_graphics) {
        // Graphics Mode Bars (Mode 9 is 320x200)
        width = 320 / 8;
        height = 15;
        y_start = 160; 
        
        // Use box(x1, y1, x2, y2, color) from GRAPHICS.LIB
        // Colors 1-7
        for(i=0; i<8; i++) {
             box(i*width, y_start, (i+1)*width, y_start+height, i);
        }
        // Colors 8-15
        for(i=0; i<8; i++) {
             box(i*width, y_start+height, (i+1)*width, y_start+height*2, i+8);
        }
    } else {
        // Text Mode Bars
        for (i = 1; i < 8; i++) {
            printf("\033[%dm  ", 40 + i);
        }
        printf("\033[0m\n");
    }
}

// Main function
int main(int argc, char *argv[]) {
    unsigned char far *machine_id = MK_FP(0xF000, 0xFFFE);
    unsigned char far *tandy_check = (unsigned char far *)0xFC000000L;
    unsigned char far *bios_check = (unsigned char far *)0xFFFF000EL;
    int graphics_mode = 0;
    int current_mode = 0; // 0=Text, 9=Medium, 10=High
    int machine_type = 0; // 0=Generic, 1=Tandy, 2=PCjr
    int forced = 0;
    
    // Mode Specific State
    int copper_speed_j = 1;
    int copper_speed_k = 1;
    int copper_offset = 0;
    
    char *comspec;
    char *version;
    
    // Animation Loop Variables
    Sprite sprites[10];
    int i;
    int scroll_x = 0, scroll_y = 0;
    int scroll_dx = 1, scroll_dy = 1; // Default scroll direction
    unsigned short *logo_data;
    int screen_width;
    int angle = 0; // Mode 7 rotation / Mode B Y-rotation
    int speed = 0; // Mode 7 speed / Mode B X-rotation
    int angle_z = 0; // Mode B Z-rotation
    int cube_scale = 64; // Mode B Scale
    int num_sprites = 10; // Default sprite count
    int key;
    
    // Ensure we clean up on exit!
    atexit(cleanup_music);
    
    printf("Starting dosfetch...\n");
    
    // Parse Arguments
    
    // Parse Arguments
    if (argc > 1) {
        forced = 1;
        if (strcmp(argv[1], "4") == 0) {
            current_mode = CGA_4;
            graphics_mode = 1;
            printf("Forcing Mode 4 (320x200 4-color)\n");
        } else if (strcmp(argv[1], "5") == 0) {
            current_mode = CGA_5;
            graphics_mode = 1;
            printf("Forcing Mode 5 (320x200 4-color BW)\n");
        } else if (strcmp(argv[1], "6") == 0) {
            current_mode = CGA_6;
            graphics_mode = 1;
            printf("Forcing Mode 6 (640x200 2-color)\n");
        } else if (strcmp(argv[1], "7") == 0) {
            // User requested "Mode 7" - Let's give them the SNES effect!
            // We'll use Mode 9 (Tandy 320x200x16) as the base for the software renderer
            current_mode = MEDIUM_16; 
            graphics_mode = 2; // 2 = Mode 7 Demo
            printf("Forcing Mode 7 (Software Affine Scaling Demo)\n");
        } else if (strcmp(argv[1], "8") == 0) {
            current_mode = LOW_16;
            graphics_mode = 1;
            printf("Forcing Mode 8 (160x200 16-color)\n");
        } else if (strcmp(argv[1], "9") == 0) {
            current_mode = MEDIUM_16;
            graphics_mode = 1;
            printf("Forcing Mode 9 (320x200 16-color)\n");
        } else if (strcmp(argv[1], "10") == 0 || strcmp(argv[1], "A") == 0 || strcmp(argv[1], "a") == 0) {
            current_mode = HIGH_4;
            graphics_mode = 1;
            printf("Forcing Mode 10 (640x200 4-color)\n");
        } else if (strcmp(argv[1], "11") == 0 || strcmp(argv[1], "B") == 0 || strcmp(argv[1], "b") == 0) {
            current_mode = MEDIUM_16; // Use Mode 9 resolution
            graphics_mode = 3; // 3 = Mode B Wireframe
            printf("Forcing Mode B (3D Wireframe Cube)\n");
        } else if (strcmp(argv[1], "12") == 0 || strcmp(argv[1], "C") == 0 || strcmp(argv[1], "c") == 0) {
            current_mode = MEDIUM_16; // Use Mode 9 resolution
            graphics_mode = 4; // 4 = Mode C Parallax
            printf("Forcing Mode C (Parallax Scrolling)\n");
        } else if (strcmp(argv[1], "13") == 0 || strcmp(argv[1], "D") == 0 || strcmp(argv[1], "d") == 0) {
            current_mode = MEDIUM_16;
            graphics_mode = 5; // 5 = Mode D Palette
            printf("Forcing Mode D (Palette Cycling)\n");
        } else if (strcmp(argv[1], "14") == 0 || strcmp(argv[1], "E") == 0 || strcmp(argv[1], "e") == 0) {
            current_mode = MEDIUM_16;
            graphics_mode = 6; // 6 = Mode E Scaled Sprite
            printf("Forcing Mode E (Sprite Scaling)\n");
        } else if (strcmp(argv[1], "15") == 0 || strcmp(argv[1], "F") == 0 || strcmp(argv[1], "f") == 0) {
            current_mode = MEDIUM_16;
            graphics_mode = 7; // 7 = Mode F Collision
            printf("Forcing Mode F (Sprite Collision)\n");
        } else if (strcmp(argv[1], "16") == 0 || strcmp(argv[1], "G") == 0 || strcmp(argv[1], "g") == 0) {
            current_mode = MEDIUM_16;
            graphics_mode = 8; // 8 = Mode G Mouse Draw
            printf("Forcing Mode G (Mouse Drawing)\n");
        } else if (strcmp(argv[1], "17") == 0 || strcmp(argv[1], "H") == 0 || strcmp(argv[1], "h") == 0) {
            current_mode = MEDIUM_16;
            graphics_mode = 9; // 9 = Mode H Hardware Scroll
            printf("Forcing Mode H (Hardware Scrolling)\n");
        } else if (strcmp(argv[1], "18") == 0 || strcmp(argv[1], "I") == 0 || strcmp(argv[1], "i") == 0) {
            current_mode = MEDIUM_16;
            graphics_mode = 10; // 10 = Mode I Page Flipping
            printf("Forcing Mode I (Hardware Page Flipping)\n");
        } else if (strcmp(argv[1], "19") == 0 || strcmp(argv[1], "J") == 0 || strcmp(argv[1], "j") == 0) {
            current_mode = MEDIUM_16;
            graphics_mode = 11; // 11 = Mode J Copper Bars
            printf("Forcing Mode J (Copper Bars)\n");
        } else if (strcmp(argv[1], "20") == 0 || strcmp(argv[1], "K") == 0 || strcmp(argv[1], "k") == 0) {
            current_mode = MEDIUM_16;
            graphics_mode = 12; // 12 = Mode K Reverse Copper Bars
            printf("Forcing Mode K (Reverse Copper Bars)\n");
        } else if (strcmp(argv[1], "21") == 0 || strcmp(argv[1], "R") == 0 || strcmp(argv[1], "r") == 0) {
            current_mode = MEDIUM_16;
            graphics_mode = 13; // 13 = Mode R Robot Speech
            printf("Forcing Mode R (Robot Speech)\n");
        } else {
            printf("Unknown mode: %s. Using auto-detect.\n", argv[1]);
            forced = 0;
        }
    }
    
    // Auto-detect if not forced
    if (!forced) {
        // Check for PCjr first (Machine ID 0xFD)
        if (*machine_id == 0xFD) {
            current_mode = MEDIUM_16;
            // graphics_mode = 1; // Don't force graphics by default
            machine_type = 2;
        }
        // Check for Tandy
        else if (*tandy_check == 0x21) {
            current_mode = MEDIUM_16;
            // graphics_mode = 1; // Don't force graphics by default
            machine_type = 1;
        }
    }
    
    if (!graphics_mode) {
        cls();
        // Standard text mode setup if needed, but we're just using printf
    }
    
    // Print Logo / Demo
    if (graphics_mode) {
        // Switch Mode
        if (graphics_mode == 8) {
            mode(9); // Force Mode 9 (320x200) for Mode G
        } else {
            mode(current_mode);
        }
        
        if (current_mode == HIGH_4 || current_mode == CGA_6) screen_width = 640;
        else if (current_mode == LOW_16) screen_width = 160;
        else screen_width = 320;
        
        // Initial Scale for Mode E
        if (graphics_mode == 6) cube_scale = 256; // 1.0x scale
        
        // Switch Mode
        if (graphics_mode == 8) {
            mode(9); // Force Mode 9 (320x200) for Mode G
        } else {
            mode(current_mode);
        }
        
        // Initialize Sprites
        for(i=0; i<10; i++) {
            sprites[i].x = rand() % (screen_width - 20);
            sprites[i].y = rand() % 180;
            sprites[i].dx = (rand() % 3) + 1;
            sprites[i].dy = (rand() % 3) + 1;
            
            if (current_mode == HIGH_4 || current_mode == CGA_4 || current_mode == CGA_5) {
                sprites[i].color = (i % 3) + 1; // 4 colors
            } else if (current_mode == CGA_6) {
                sprites[i].color = 1; // 2 colors (0, 1)
            } else {
                sprites[i].color = (i % 15) + 1; // 16 colors
            }
            sprites[i].active = 1;
        }
        
        printf("Press ESC to exit demo...\n");
        if (graphics_mode == 2) {
            printf("Controls: Left/Right to Rotate, Up/Down to Speed\n");
            printf("          +/- to Add/Remove Sprites\n");
        } else if (graphics_mode == 3) {
            printf("Controls: Left/Right to Rotate Y, Up/Down to Rotate Z\n");
            printf("          * / / to Scale, +/- to Add/Remove Sprites\n");
        } else if (graphics_mode == 4) {
            printf("Controls: Left/Right to Scroll\n");
            printf("          +/- to Add/Remove Sprites\n");
        } else if (graphics_mode == 5) {
            printf("Controls: None (Auto Cycling)\n");
        } else if (graphics_mode == 6) {
            printf("Controls: +/- to Scale Sprite\n");
        } else if (graphics_mode == 7) {
            printf("Controls: Arrows to Move Player (Green)\n");
            printf("          Avoid the Red Sprites!\n");
        } else if (graphics_mode == 8) {
            printf("Controls: Mouse Left Click to Draw\n");
            printf("          Mouse Right Click to Erase\n");
        } else if (graphics_mode == 9) {
            printf("Controls: Arrows to Pan Screen (Hardware)\n");
            printf("          Watch the smooth scrolling!\n");
        } else if (graphics_mode == 10) {
            printf("Controls: Watch the Bouncing Ball (60 FPS)\n");
            printf("          Hardware Page Flipping Active\n");
        } else if (graphics_mode == 11) {
            printf("Controls: Stare at the Copper Bars\n");
            printf("          Raster Interrupts (Polling)\n");
        } else {
            printf("Controls: Arrows to Change Scroll Direction\n");
            printf("          +/- to Add/Remove Sprites\n");
        }
        
        while (1) {
            // Input Handling
            if (kbhit()) {
                key = getch();
                if (key == 27) break; // ESC to exit
                
                // Global Sprite Count Control (Except Mode E)
                if (graphics_mode != 6) {
                    if (key == '+' || key == '=') {
                        if (num_sprites < 10) num_sprites++;
                    }
                    if (key == '-' || key == '_') {
                        if (num_sprites > 0) num_sprites--;
                    }
                } else {
                    // Mode E: Scale Control
                    if (key == '+' || key == '=') cube_scale += 16;
                    if (key == '-' || key == '_') {
                        cube_scale -= 16;
                        if (cube_scale < 16) cube_scale = 16;
                    }
                }
                
                if (graphics_mode == 3) {
                    if (key == '*') cube_scale += 2;
                    if (key == '/') {
                        cube_scale -= 2;
                        if (cube_scale < 2) cube_scale = 2;
                    }
                }
                
                // Mode J/K Speed Control
                if (graphics_mode == 11) {
                    // Mode J: Up/Down
                    if (key == 0) key = getch(); // Extended
                    if (key == 72) copper_speed_j = 1; // Up
                    if (key == 80) copper_speed_j = -1; // Down
                }
                if (graphics_mode == 12) {
                    // Mode K: Left/Right
                    if (key == 0) key = getch(); // Extended
                    if (key == 75) copper_speed_k = -1; // Left
                    if (key == 77) copper_speed_k = 1; // Right
                }
                
                if (key == 0 || key == 0xE0) {
                    key = getch(); // Extended code
                    if (graphics_mode == 2) {
                        // Mode 7 Controls
                        if (key == 75) angle = (angle - 4) & 0xFF; // Left
                        if (key == 77) angle = (angle + 4) & 0xFF; // Right
                        if (key == 72) speed += 2; // Up (Increase Speed)
                        if (key == 80) speed -= 2; // Down (Decrease Speed)
                    } else if (graphics_mode == 3) {
                        // Mode B Controls
                        if (key == 75) angle = (angle - 4) & 0xFF; // Left (Y Rot)
                        if (key == 77) angle = (angle + 4) & 0xFF; // Right (Y Rot)
                        if (key == 72) angle_z = (angle_z + 4) & 0xFF; // Up (Z Rot)
                        if (key == 80) angle_z = (angle_z - 4) & 0xFF; // Down (Z Rot)
                    } else if (graphics_mode == 4) {
                        // Mode C Controls
                        if (key == 75) scroll_dx = -2; // Left Fast
                        if (key == 77) scroll_dx = 2;  // Right Fast
                        if (key == 72) scroll_dx = 0;  // Stop
                        if (key == 80) scroll_dx = 0;  // Stop
                    } else if (graphics_mode == 7) {
                        // Mode F Controls (Player Movement)
                        // Player is sprite[0]
                        if (key == 75) sprites[0].x -= 4; // Left
                        if (key == 77) sprites[0].x += 4; // Right
                        if (key == 72) sprites[0].y -= 4; // Up
                        if (key == 80) sprites[0].y += 4; // Down
                    } else {
                        // Standard Scroll Controls
                        if (key == 75) scroll_dx = -1; // Left
                        if (key == 77) scroll_dx = 1;  // Right
                        if (key == 72) scroll_dy = -1; // Up
                        if (key == 80) scroll_dy = 1;  // Down
                    }
                }
            }
            
            // Update Scroll
            if (graphics_mode == 2) {
                // Mode 7: "Speed" moves us in Y direction (forward/back)
                scroll_y += speed;
            } else {
                scroll_x += scroll_dx;
                scroll_y += scroll_dy;
            }
            
            // Draw Background
            if (graphics_mode == 2) {
                // Mode 7 Demo
                draw_mode7_background(angle, scroll_x * 10, -scroll_y * 10);
            } else if (graphics_mode == 3) {
                // Mode B (Wireframe Cube)
                // angle -> Y rotation
                // angle_z -> Z rotation
                // speed -> X rotation (let's keep it 0 or auto?)
                // Let's auto-rotate X slowly for effect? Or just 0.
                speed = (speed + 1) & 0xFF; // Auto rotate X slowly
                
                draw_wireframe_cube(speed, angle, angle_z, cube_scale);
            } else if (graphics_mode == 4) {
                // Mode C (Parallax)
                draw_parallax_background(scroll_x);
            } else if (graphics_mode == 5) {
                // Mode D (Palette Cycling)
                scroll_x++; // Use scroll_x as frame counter
                draw_palette_cycling_demo(scroll_x);
            } else if (graphics_mode == 6) {
                // Mode E (Sprite Scaling)
                memset(framebuffer, 0, 64000); // Clear screen
                logo_data = (machine_type == 2) ? pcjr_logo_16x16 : tandy_logo_16x16;
                // Center sprite
                draw_sprite_scaled(160 - ((16 * cube_scale) >> 9), 100 - ((16 * cube_scale) >> 9), 14, cube_scale, logo_data);
            } else if (graphics_mode == 7) {
                // Mode F (Collision)
                draw_background(0, 0); // Static background
                
                // Sprite 0 is Player (Green)
                sprites[0].color = 2; 
                sprites[0].active = 1;
                // Clamp Player
                if (sprites[0].x < 0) sprites[0].x = 0;
                if (sprites[0].x > 304) sprites[0].x = 304;
                if (sprites[0].y < 0) sprites[0].y = 0;
                if (sprites[0].y > 184) sprites[0].y = 184;
                
                draw_sprite_fast(sprites[0].x, sprites[0].y, sprites[0].color, logo_data);
                
                // Other sprites bounce around
                for (i = 1; i < num_sprites; i++) {
                    if (!sprites[i].active) continue;
                    
                    // Move
                    sprites[i].x += sprites[i].dx;
                    sprites[i].y += sprites[i].dy;
                    
                    // Bounce
                    if (sprites[i].x <= 0 || sprites[i].x >= 304) sprites[i].dx = -sprites[i].dx;
                    if (sprites[i].y <= 0 || sprites[i].y >= 184) sprites[i].dy = -sprites[i].dy;
                    
                    // Check Collision with Player
                    if (check_sprite_collision(&sprites[0], &sprites[i])) {
                        sprites[i].color = 15; // White (Hit!)
                    } else {
                        sprites[i].color = 4; // Red (Danger)
                    }
                    
                    draw_sprite_fast(sprites[i].x, sprites[i].y, sprites[i].color, logo_data);
                }
            } else if (graphics_mode == 8) {
                // Mode G (Mouse Drawing)
                int mx, my, mbuttons;
                static int mouse_initialized = 0;
                static int prev_mx = -1, prev_my = -1;
                
                if (!mouse_initialized) {
                    clear_framebuffer(BLACK); // Clear screen for drawing
                    
                    // Draw Exit Button (Red Box at Top Right)
                    box(280, 5, 315, 20, 4); // Red Box
                    line_fast(290, 8, 305, 17, 15); // White X
                    line_fast(305, 8, 290, 17, 15);
                    
                    if (mouse_init()) {
                        mouse_show();
                        mouse_initialized = 1;
                    }
                }
                
                if (mouse_initialized) {
                    mbuttons = mouse_status(&mx, &my);
                    mx = mx >> 1; // Scale 640 -> 320
                    
                    // Debug: Draw Coordinates
                    // (Requires font data, let's just draw a pixel at the raw coordinate to see where it is)
                    // Actually, let's use the console for debug if possible, or just rely on the fix.
                    // But wait, if I can't click, I need to know why.
                    // Let's try to make the hit box bigger first.
                    
                    // Debug: Draw a pixel at the mouse cursor position (inverted color)
                    // set_pixel_fast(mx, my, 15);
                    
                    if (mbuttons & 1) { // Left Button
                        // Check for Exit Button Click
                        if (mx >= 280 && mx <= 315 && my >= 5 && my <= 20) {
                            break; // Exit loop
                        }
                        
                        if (mx >= 0 && mx < 320 && my >= 0 && my < 200) {
                            if (prev_mx != -1) {
                                line_fast(prev_mx, prev_my, mx, my, 15); // Draw Line
                            } else {
                                framebuffer[(unsigned long)my * 320 + mx] = 15; // Dot
                            }
                        }
                        prev_mx = mx;
                        prev_my = my;
                    } else if (mbuttons & 2) { // Right Button
                        if (mx >= 0 && mx < 320 && my >= 0 && my < 200) {
                             if (prev_mx != -1) {
                                line_fast(prev_mx, prev_my, mx, my, 0); // Erase Line
                            } else {
                                framebuffer[(unsigned long)my * 320 + mx] = 0; // Erase Dot
                            }
                        }
                        prev_mx = mx;
                        prev_my = my;
                    } else {
                        prev_mx = -1; // Reset previous position when button released
                        prev_my = -1;
                    }
                    
                    mouse_hide(); // Hide cursor before blit
                    blit();       // Update screen
                    mouse_show(); // Show cursor on top
                } else {
                    blit();
                }
            } else if (graphics_mode == 9) {
                // Mode H (Hardware Scroll)
                // Only draw the pattern ONCE at the start?
                // Or draw it every frame?
                // If we draw every frame to 0, and scroll the hardware, we won't see scrolling because we overwrite memory.
                // We should draw ONCE.
                static int mode_h_initialized = 0;
                if (!mode_h_initialized) {
                    draw_hardware_scrolling_demo();
                    mode_h_initialized = 1;
                }
                
                // Update Hardware Scroll
                set_crtc_start(scroll_x);
                
                // NO BLIT! We are viewing VRAM directly.
                // But wait, our 'framebuffer' is in RAM. 'blit()' copies it to VRAM.
                // If we don't blit, VRAM stays static.
                // So we draw ONCE to framebuffer, BLIT ONCE, then stop blitting.
                if (mode_h_initialized == 1) {
                    blit(); 
                    mode_h_initialized = 2; // Done blitting
                }
                
            } else if (graphics_mode == 10) {
                // Mode I (Hardware Page Flipping) - OPTIMIZED
                static int active_page = 0; // The page we are DRAWING to (Hidden)
                static int visual_page = 0; // The page we are SHOWING
                static int ball_x = 160, ball_y = 100;
                static int ball_dx = 2, ball_dy = 2;
                static int initialized = 0;
                
                // Track the ball position on EACH page separately
                static int p_x[2] = {160, 160};
                static int p_y[2] = {100, 100};
                
                if (!initialized) {
                    // Clear both pages initially
                    memset(framebuffer, 0, 64000); // Use framebuffer as black source
                    blit_page(0);
                    blit_page(1);
                    initialized = 1;
                }
                
                // 1. Update Physics
                ball_x += ball_dx;
                ball_y += ball_dy;
                if (ball_x < 10 || ball_x > 310) ball_dx = -ball_dx;
                if (ball_y < 10 || ball_y > 190) ball_dy = -ball_dy;
                
                // 2. Draw directly to HIDDEN Page (VRAM)
                active_page = 1 - visual_page; 
                
                // Erase the ball that is ON THIS PAGE (from 2 frames ago)
                draw_rect_page(p_x[active_page] - 5, p_y[active_page] - 5, 11, 11, 0, active_page);
                
                // Draw new ball
                draw_rect_page(ball_x - 5, ball_y - 5, 11, 11, 14, active_page);
                
                // Update tracking for this page
                p_x[active_page] = ball_x;
                p_y[active_page] = ball_y;
                
                // 3. Flip!
                wait_vsync(); // Sync with monitor
                set_video_page(active_page);
                visual_page = active_page;
                
            } else if (graphics_mode == 11) {
                // Mode J: Vertical Scrolling (Horizontal Stripes)
                static int initialized = 0;
                
                if (!initialized) {
                    // Fill screen with SOLID COLOR 1
                    // The raster effect will change what Color 1 looks like per line.
                    int x, y;
                    for (y = 0; y < 200; y++) {
                        for (x = 0; x < 320; x++) {
                            framebuffer[(unsigned long)y * 320 + x] = 1;
                        }
                    }
                    blit(); 
                    initialized = 1;
                }
                
                draw_raster_bars(copper_offset);
                copper_offset += copper_speed_j;
                
            } else if (graphics_mode == 12) {
                // Mode K: Horizontal Scrolling (Vertical Stripes)
                static int initialized = 0;
                
                if (!initialized) {
                    // Fill screen with VERTICAL stripes (x % 16)
                    // Use (x/2) to make bars wider (2 pixels wide)
                    int x, y;
                    for (y = 0; y < 200; y++) {
                        for (x = 0; x < 320; x++) {
                            framebuffer[(unsigned long)y * 320 + x] = (x / 2) % 16;
                        }
                    }
                    blit(); 
                    initialized = 1;
                }
                
                cycle_palette_bars(copper_offset);
                copper_offset += copper_speed_k;
                
            } else if (graphics_mode == 13) {
                // Mode R: Robot Speech
                static int initialized = 0;
                if (!initialized) {
                    // Increase buffer to 24000 for ~3 seconds of audio
                    unsigned char *speech_buffer = (unsigned char *)malloc(24000);
                    if (speech_buffer) {
                        generate_robot_voice(speech_buffer, 24000);
                        play_pcm_sample(speech_buffer, 24000, 8000);
                        free(speech_buffer);
                    }
                    initialized = 1;
                }
                
                // Draw background to prevent sprite trails!
                draw_background(scroll_x, scroll_y);
                
            } else if (current_mode == HIGH_4 || current_mode == CGA_6) {
                draw_background_high(scroll_x, scroll_y);
            } else if (current_mode == LOW_16) {
                draw_background_low(scroll_x, scroll_y);
            } else if (current_mode == CGA_4 || current_mode == CGA_5) {
                draw_background_cga(scroll_x, scroll_y);
            } else {
                // Only draw background if NOT Mode G (to avoid clearing canvas)
                if (graphics_mode != 8) draw_background(scroll_x, scroll_y);
            }
            
            // Handle Music Logic
            if (graphics_mode == 11 || graphics_mode == 12) {
                // Start music if not playing
                init_music();
            } else {
                // Stop music if playing
                cleanup_music();
            }
            
            // Update and Draw Sprites
            // Update and Draw Sprites (Skip for Mode E, F, G, H, I, J, K)
            if (graphics_mode != 6 && graphics_mode != 7 && graphics_mode != 8 && graphics_mode != 9 && graphics_mode != 10 && graphics_mode != 11 && graphics_mode != 12) {
                for(i=0; i<num_sprites; i++) {
                    // Move
                    sprites[i].x += sprites[i].dx;
                    sprites[i].y += sprites[i].dy;
                    
                    // Bounce
                    if (sprites[i].x < 0 || sprites[i].x > (screen_width - 16)) sprites[i].dx = -sprites[i].dx;
                    if (sprites[i].y < 0 || sprites[i].y > 184) sprites[i].dy = -sprites[i].dy;
                    
                    // Draw
                    logo_data = (machine_type == 2) ? pcjr_logo_16x16 : tandy_logo_16x16;
                    
                    if (current_mode == HIGH_4 || current_mode == CGA_6) {
                        draw_sprite_high(sprites[i].x, sprites[i].y, sprites[i].color, logo_data);
                    } else if (current_mode == LOW_16) {
                        draw_sprite_low(sprites[i].x, sprites[i].y, sprites[i].color, logo_data);
                    } else {
                        draw_sprite_fast(sprites[i].x, sprites[i].y, sprites[i].color, logo_data);
                    }
                }
            }
            
            // Blit
            if (current_mode == HIGH_4) {
                blit_high();
            } else if (current_mode == LOW_16) {
                blit_low();
            } else if (current_mode == CGA_4 || current_mode == CGA_5) {
                blit_cga_4();
            } else if (current_mode == CGA_6) {
                blit_cga_6();
            } else {
                if (graphics_mode != 9 && graphics_mode != 10 && graphics_mode != 11) blit();
            }
        }

        // getch(); // Consume key - Removed to prevent blocking on mouse exit
        
        // Text positioning
        if (current_mode == LOW_16) gotoxy(2, 2); 
        else gotoxy(5, 2); 
    } else {
        // Standard Text Mode Logo
        if (*bios_check != 0xFF) {
            print_dosbox_logo();
        } else {
            // Generic PC
            printf("PC Compatible\n");
        }
        
        // Move cursor to the right of the logo for system info
        // gotoxy(40, 2); // Standard text mode
    }

    print_user_host(graphics_mode);
    
    printf("OS: "); dosver(graphics_mode);
    printf("Uptime: "); uptime(graphics_mode);
    printf("Shell: "); 
    
    comspec = getenv("COMSPEC");
    if (comspec) {
        printf("%s\n", comspec);
    } else {
        printf("Unknown\n");
    }
    
    if (graphics_mode) printf("FDD: ");
    else printf("Floppy drives: "); 
    floppy(graphics_mode);
    
    printf("Disk: "); disksize(0, graphics_mode);
    
    if (graphics_mode) printf("Base: ");
    else printf("Base Memory: "); 
    base_memory(graphics_mode);
    
    if (graphics_mode) printf("Ext: ");
    else printf("Ext. Memory: "); 
    extended_memory(graphics_mode);
    
    if (graphics_mode) printf("FPU: ");
    else printf("Floating Point Unit: "); 
    fpu(graphics_mode);
    
    if (graphics_mode) printf("Model: ");
    else printf("Computer Type: "); 
    detect_tandy(graphics_mode);
    
    if (graphics_mode) printf("Video: ");
    else printf("Video Mode: "); 
    detect_tandy_mode(graphics_mode);
    
    if (graphics_mode) printf("CPU: ");
    else printf("CPU Type: "); 
    detect_cpu(graphics_mode);
    
    if (graphics_mode) printf("Speed: ");
    else printf("CPU Speed: "); 
    detect_cpu_speed(graphics_mode);
    
    // DOSBox detection
    if (detect_dosbox()) {
        version = get_dosbox_version();
        if (version != NULL) {
            if (graphics_mode) printf("Emu: DOSBox %s\n", version);
            else printf("Emulator: DOSBox %s\n", version);
        } else {
            if (graphics_mode) printf("Emu: DOSBox\n");
            else printf("Emulator: DOSBox (or compatible)\n");
        }
    }

    if (detect_sn76496()) {
        if (graphics_mode) printf("Sound: SN76496");
        else printf("Texas Instruments SN76496 Sound Chip.");
        play_startup_sound();  // Play a quick beep!
    } else {
        if (graphics_mode) printf("Sound: None");
        else printf("SN76496 Sound Chip not detected.");
    }
    if (!graphics_mode) printf("\n");  // Extra newline only for non-compact
    
    
    // Display color palette bars
    print_color_bars(graphics_mode);
    
    // Reset mode on exit?
    // User wanted persistent graphics.
    // GRAPHICS.LIB doc says: "On a Tandy 1000 do not exit from the program while in modes 8-10 or the system will HANG. Issue a mode(CO80); mode(BW80); before termination"
    // Uh oh. That contradicts the "persistent graphics" goal.
    // Let's try NOT resetting and see if DOSBox hangs. It might be a real hardware issue that DOSBox doesn't emulate.
    // Or we can wait for a keypress.
    
    // Cleanup
    // Cleanup
    // if (graphics_mode) {
    //     getch(); // Wait for keypress
    //     mouse_reset(); // Reset mouse driver
    //     mode(3); // Restore Text Mode
    // }
    
    return 0;
}
