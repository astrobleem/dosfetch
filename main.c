#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <i86.h>
#include <graph.h>
#include <dos.h>

// Color definitions
#define BLACK 0
#define BLUE 1
#define GREEN 2
#define CYAN 3
#define RED 4
#define MAGENTA 5
#define BROWN 6
#define LIGHTGRAY 7
#define LIGHTBLACK 8
#define LIGHTBLUE 9
#define LIGHTGREEN 10
#define LIGHTCYAN 11
#define LIGHTRED 12
#define LIGHTMAGENTA 13
#define YELLOW 14
#define WHITE 15

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
void base_memory(void);
void extended_memory(void);
void disksize(unsigned char disk);
void dosver(void);
void floppy(void);
void fpu(void);
void colorline(const char* s);
void detect_tandy(void);
void detect_tandy_mode(void);
void detect_cpu(void);
void detect_cpu_speed(void);
unsigned long get_ticks(void);
int detect_sn76496(void);
int detect_8253_timer(void);
void print_color_bars(void);
void sn_set_tone(int channel, unsigned int period);
void sn_set_volume(int channel, unsigned int vol4);
void wait_ticks(unsigned int ticks);
void play_startup_sound(void);
unsigned int freq_to_period(unsigned int freq_hz);

int detect_8253_timer(void) {
    unsigned char test_value = 0x36;  // Arbitrary test value for control register
    unsigned char read_value;

    // Write test value to the Timer Control port
    outp(TIMER_CONTROL_PORT, test_value);

    // Write and read back from Timer Counter 0
    outp(TIMER_COUNTER_0, 0xFF);
    read_value = inp(TIMER_COUNTER_0);
    if (read_value != 0xFF) return 0;

    // Write and read back from Timer Counter 1
    outp(TIMER_COUNTER_1, 0xFF);
    read_value = inp(TIMER_COUNTER_1);
    if (read_value != 0xFF) return 0;

    // Write and read back from Timer Counter 2
    outp(TIMER_COUNTER_2, 0xFF);
    read_value = inp(TIMER_COUNTER_2);
    if (read_value != 0xFF) return 0;

    // If all ports return the test value, the chip is likely present
    return 1;
}


// PSG Sound Functions (based on psgtest.c)
// Set tone: period is 10-bit (1..1023), channel 0-2
void sn_set_tone(int channel, unsigned int period) {
    unsigned char latch = 0x80 | ((channel & 3) << 5) | (period & 0x0F);
    unsigned char data = (unsigned char)((period >> 4) & 0x3F);
    outp(SN76496_PORT_0, latch);
    outp(SN76496_PORT_0, data);
}

// Set volume: vol4 0=loudest, 15=silent
void sn_set_volume(int channel, unsigned int vol4) {
    outp(SN76496_PORT_0, 0x90 | ((channel & 3) << 5) | (vol4 & 0x0F));
}

// Wait for BIOS ticks (~18.2 Hz, ~55ms per tick)
void wait_ticks(unsigned int ticks) {
    unsigned long start = get_ticks();
    while ((get_ticks() - start) < ticks) {
        /* spin */
    }
}

// Convert frequency to PSG period
// SN76496: fout = CLK / (32 * period)
// NTSC clock ~3.579545 MHz
unsigned int freq_to_period(unsigned int freq_hz) {
    unsigned long n;
    if (freq_hz == 0) return 1023;
    n = (3579545UL / 32UL) / (unsigned long)freq_hz;
    if (n < 1) n = 1;
    if (n > 1023) n = 1023;
    return (unsigned int)n;
}

// Play C major triad arpeggio + noise tick (from psgtest.c)
void play_startup_sound(void) {
    unsigned int v;
    
    // Start silent
    sn_set_volume(0, 15);
    sn_set_volume(1, 15);
    sn_set_volume(2, 15);
    
    // Simple arpeggio on channels 0..2 (C4, E4, G4)
    sn_set_tone(0, freq_to_period(262));  // C4
    sn_set_volume(0, 4);
    wait_ticks(4);  // ~220ms
    
    sn_set_tone(1, freq_to_period(330));  // E4
    sn_set_volume(1, 4);
    wait_ticks(4);  // ~220ms
    
    sn_set_tone(2, freq_to_period(392));  // G4
    sn_set_volume(2, 4);
    wait_ticks(7);  // ~385ms
    
    // Hold the triad briefly
    wait_ticks(5);  // ~275ms
    
    // Quick noise tick
    outp(SN76496_PORT_0, 0xE0 | 0x05);  // low-rate white noise
    sn_set_volume(3, 6);  // noise channel volume
    wait_ticks(2);  // ~110ms
    sn_set_volume(3, 15);  // silence noise
    
    // Fade out
    for (v = 4; v <= 15; v++) {
        sn_set_volume(0, v);
        sn_set_volume(1, v);
        sn_set_volume(2, v);
        wait_ticks(1);  // ~55ms per step
    }
    
    // All silent
    sn_set_volume(0, 15);
    sn_set_volume(1, 15);
    sn_set_volume(2, 15);
}

// Improved SN76496 detection
int detect_sn76496(void) {
    unsigned char saved_port;
    int detected = 0;
    
    // Try to silence all channels first
    sn_set_volume(0, 15);
    sn_set_volume(1, 15);
    sn_set_volume(2, 15);
    sn_set_volume(3, 15);
    
    // The PSG is write-only, so we can't read back
    // Instead, check if we're on a Tandy machine
    // PSG is only present on Tandy 1000 series
    unsigned char far *tandy_check = (unsigned char far *)0xFC000000L;
    
    if (*tandy_check == 0x21) {
        detected = 1;  // Likely has PSG
    }
    
    return detected;
}

// New function to print Tandy ASCII art
void print_tandy_logo(void) {
    _settextcolor(YELLOW);
    _outtext("   _____  ___    _   _ ______  __   __\n");
    _outtext("  |_   _|/ _ \\  | \\ | ||  _  \\ \\ \\ / /\n");
    _outtext("    | | / /_\\ \\ |  \\| || | | |  \\ V / \n");
    _outtext("    | | |  _  | | . ` || | | |  /   \\ \n");
    _outtext("    | | | | | | | |\\  || |/ /  / /^\\ \\\n");
    _outtext("    \\_/ \\_| |_/ \\_| \\_/|___/   \\/   \\/\n");

_setbkcolor(BLACK);
    _outtext("         ");


_setbkcolor(LIGHTRED);
_outtext("     ");
_setbkcolor(BLACK);
_outtext("   ");

_setbkcolor(LIGHTGREEN);
_outtext("     ");
_setbkcolor(BLACK);
_outtext("   ");

_setbkcolor(LIGHTBLUE);
_outtext("     \n");

_setbkcolor(BLACK);

    _settextcolor(WHITE);
}

void print_tandy_logo2(void){

_outtext(" ,--------. ,---.  ,--.  ,--.,------.,--.   ,--.\n");
_outtext("'  '--.  .--'/  O  \\ |  ,'.|  ||  .-.  \\\  `.'  /\n");
_outtext("'     |  |  |  .-.  ||  |' '  ||  |  \  :'.    /\n");
_outtext("'     |  |  |  | |  ||  | `   ||  '--'  /  |  |\n");
_outtext("'     `--'  `--' `--'`--'  `--'`-------'   `--'\n");
}


void print_dosbox_logo(void){
    _settextcolor(LIGHTCYAN);
    _outtext("  ___   __  ____\n");
    _outtext(" |   \\ /  \\/ ___|\n");
    _outtext(" | |\\ |  . \___ \\\n");
    _outtext(" |___/ \\__/\\____/\n");
    _settextcolor(YELLOW);
    _outtext("   fetch");
    _settextcolor(LIGHTGRAY);
    _outtext("\n\n");
    
    // Color indicator blocks
    _setbkcolor(BLUE); _outtext(" "); 
    _setbkcolor(GREEN); _outtext(" ");
    _setbkcolor(CYAN); _outtext(" ");
    _setbkcolor(RED); _outtext(" ");
    _setbkcolor(MAGENTA); _outtext(" ");
    _setbkcolor(YELLOW); _outtext(" ");
    _setbkcolor(BLACK);
    _outtext("  for MS-DOS\n\n");
    _settextcolor(WHITE);
}

void detect_tandy(void) {
    union REGS regs;
    struct SREGS sregs;
    
    // Use INT 10h AH=1Ah - the proper BIOS detection method
    // This is what the Windows driver uses - much more reliable!
    regs.w.ax = 0x1A00;
    int86(0x10, &regs, &regs);
    
    if (regs.h.bl == 0xFF) {
        printf("Tandy 1000/PCjr Graphics Adapter detected\n");
        
        // Check for specific Tandy 1000 SL/TL models using INT 15h AH=C0h
        regs.h.ah = 0xC0;
        int86x(0x15, &regs, &regs, &sregs);
        
        if (!regs.x.cflag) {
            unsigned char far *model_id = MK_FP(sregs.es, regs.x.bx + 2);
            if (*model_id == 0xFF) {
                printf("  (Tandy 1000 SL/TL variant)\n");
                return;
            }
        }
        return;
    }
    
    // Fallback: Check BIOS ROM markers (less reliable)
    unsigned char far *bios_check = (unsigned char far *)0xFFFF000EL;
    unsigned char far *tandy_check = (unsigned char far *)0xFC000000L;
    
    if (*bios_check != 0xFF) {
        printf("Not an IBM PC compatible\n");
        return;
    }
    
    if (*tandy_check == 0x21) {
        printf("Tandy 1000 series (ROM check)\n");
        return;
    }
    
    printf("Non-Tandy system\n");
}

// Detect current Tandy video mode
void detect_tandy_mode(void) {
    union REGS regs;
    
    // Get current video mode using INT 10h AH=0Fh
    regs.h.ah = 0x0F;
    int86(0x10, &regs, &regs);
    
    switch (regs.h.al) {
        case 0x00:
            printf("40x25 Text (Mode 0)\n");
            break;
        case 0x01:
            printf("40x25 Color Text (Mode 1)\n");
            break;
        case 0x02:
            printf("80x25 Text (Mode 2)\n");
            break;
        case 0x03:
            printf("80x25 Color Text (Mode 3)\n");
            break;
        case 0x04:
            printf("320x200 4-color Graphics (Mode 4/CGA)\n");
            break;
        case 0x05:
            printf("320x200 4-color Graphics BW (Mode 5)\n");
            break;
        case 0x06:
            printf("640x200 2-color Graphics (Mode 6/CGA)\n");
            break;
        case 0x08:
            printf("160x200 16-color Graphics (Mode 8/PCjr)\n");
            break;
        case 0x09:
            printf("320x200 16-color Graphics (Mode 9/Tandy)\n");
            break;
        case 0x0A:
            printf("640x200 4-color Graphics (Mode A/Tandy)\n");
            break;
        default:
            printf("Unknown Mode 0x%02X\n", regs.h.al);
            break;
    }
}

// CMOS function
unsigned char cmos(unsigned char cmd) {
    outp(0x70, cmd);
    return inp(0x71);
}


// Base memory function
void base_memory(void) {
    union REGS regs;
    int86(0x12, &regs, &regs);
    printf("%u KB\n", regs.w.ax);
}

// Extended memory function
void extended_memory(void) {
    union REGS regs;
    unsigned long mem = 0;

    regs.h.ah = 0x88;
    int86(0x15, &regs, &regs);

    if (regs.x.cflag) {
        printf("none\n");  // XXX could be wrong on XT
    } else {
        regs.w.ax = 0xE801;
        int86(0x15, &regs, &regs);

        if (regs.x.cflag) {
            mem = (unsigned long)cmos(0x17) + 256UL * cmos(0x18);
        } else {
            mem = (unsigned long)regs.w.cx + 64UL * (unsigned long)regs.w.dx;
        }
        printf("%lu KB\n", mem);
    }
}

// Disk size function
void disksize(unsigned char disk) {
    union REGS regs;
    unsigned long total, free;

    regs.h.ah = 0x36;
    regs.h.dl = disk;
    int86(0x21, &regs, &regs);

    if (regs.w.ax != 0xFFFF) {
        free = (unsigned long)regs.w.ax * (unsigned long)regs.w.cx * (unsigned long)regs.w.bx;
        total = (unsigned long)regs.w.ax * (unsigned long)regs.w.cx * (unsigned long)regs.w.dx;
        printf("%lu/%lu KB (%d%% free)\n", 
               (total - free) / 1024UL, total / 1024UL, 
               (int)((free * 100UL) / total));
    } else {
        printf("Error reading disk\n");
    }
}

// DOS version function
void dosver(void) {
    union REGS regs;

    regs.w.ax = 0x3000;
    int86(0x21, &regs, &regs);
    unsigned char ven = regs.h.bh;

    regs.w.ax = 0x3306;
    int86(0x21, &regs, &regs);
    unsigned char maj = regs.h.bl;
    unsigned char min = regs.h.bh;

    switch (ven) {
        case 0x00: printf("IBM DOS "); break;
        case 0xFD: printf("FreeDOS "); break;
        case 0xFF: printf("MS DOS "); break;
        default:   printf("Unknown DOS "); break;
    }
    printf("%d.%d\n", maj, min);
}

// Floppy drive count function
void floppy(void) {
    union REGS regs;
    int86(0x11, &regs, &regs);

    int f = ((regs.h.al & 0x1) == 0x1) ? (regs.h.al >> 6) + 1 : 0;
    printf("%d\n", f);
}

// FPU check function
void fpu(void) {
    union REGS regs;
    int86(0x11, &regs, &regs);

    printf("%s\n", (regs.h.al & 0x2) == 0x2 ? "YES" : "no");
}

// Color line function
void colorline(const char* s) {
    _setcolor(YELLOW);
    _outtext(s);
    _setcolor(LIGHTBLUE);
    _outtext(s + 14);
    _setcolor(LIGHTRED);
    _outtext(s + 28);
    _setcolor(WHITE);
    _outtext("\n");
}

// Print color palette bars (classic neofetch style)
void print_color_bars(void) {
    int i;
    
    // First row: Dark/normal colors (0-7)
    for (i = 0; i < 8; i++) {
        _setbkcolor(i);
        _outtext("   ");  // 3 spaces per color block
    }
    _setbkcolor(BLACK);  // Reset background
    _outtext("\n");
    
    // Second row: Bright colors (8-15)
    for (i = 8; i < 16; i++) {
        _setbkcolor(i);
        _outtext("   ");  // 3 spaces per color block
    }
    _setbkcolor(BLACK);  // Reset background
    _outtext("\n");
}

// Detect if CPU is NEC V20/V30 vs Intel 8088/8086
// Uses the FLAGS register behavior difference
int detect_nec_cpu(void) {
    unsigned int flags1, flags2;
    
    // NEC and Intel handle the MUL instruction differently for flags
    // This uses a documented difference in flag behavior
    _asm {
        pushf                   ; Save original flags
        push ax
        push bx
        
        mov al, 0FFh           ; Set AL to 0xFF
        mov bl, 02h            ; Set BL to 0x02  
        mul bl                 ; Multiply AL * BL
        lahf                   ; Load flags into AH
        mov flags1, ax         ; Save result
        
        ; On Intel: OF and CF are undefined after MUL
        ; On NEC: OF and CF are cleared
        
        pop bx
        pop ax
        popf                   ; Restore flags
    }
    
    // Check if overflow flag behavior indicates NEC
    // This is a simplified check
    return 0; // For now, return Intel (we'll refine this)
}

// CPU detection function - determines CPU type
void detect_cpu(void) {
    unsigned int cpu_type = 0;
    unsigned int is_nec = 0;
    
    // Detect CPU type using FLAGS register manipulation
    _asm {
        pushf                   ; Save flags
        pop ax                 ; Get flags into AX
        mov bx, ax             ; Save copy
        and ax, 0FFFh          ; Try to clear bits 12-15
        push ax
        popf                   ; Put back into flags
        pushf
        pop ax                 ; Get flags again
        and ax, 0F000h         ; Check if bits 12-15 are set
        cmp ax, 0F000h         ; If set, it's 8086/8088 (bits stuck high)
        je cpu_8086
        
        push bx                ; Restore original flags
        popf
        
        ; Try to set bits 12-15
        or bx, 0F000h
        push bx
        popf
        pushf
        pop ax
        and ax, 0F000h
        jz cpu_286             ; If cleared, it's 80286 (bits stuck low)
        
        ; If bits are changeable, it's 386+
        mov cpu_type, 3
        jmp cpu_done
        
    cpu_286:
        mov cpu_type, 2
        jmp cpu_done
        
    cpu_8086:
        mov cpu_type, 1
        
    cpu_done:
        nop
    }
    
    // Display CPU type
    if (cpu_type == 1) {
        // For 8086/8088, try to detect NEC
        // Use the SALC instruction (D6h) - behaves differently
        _asm {
            push ax
            push bx
            xor al, al          ; Clear AL
            stc                 ; Set carry flag
            db 0D6h             ; SALC on 8088, different on NEC
            mov bl, al          ; Save result
            pop bx  
            cmp bl, 0FFh        ; On Intel, AL = FFh; on NEC it may differ
            pop ax
            je is_intel
            mov is_nec, 1
        is_intel:
            nop
        }
        
        if (is_nec) {
            printf("NEC V20/V30\n");
        } else {
            printf("Intel 8088/8086\n");
        }
    } else if (cpu_type == 2) {
        printf("Intel 80286\n");
    } else if (cpu_type == 3) {
        printf("Intel 386+\n");
    } else {
        printf("Unknown CPU\n");
    }
}

// CPU speed detection function
void detect_cpu_speed(void) {
    unsigned long start_ticks, end_ticks, elapsed_ticks;
    unsigned int delay_count = 0;
    unsigned long ticks_per_sec = 18.2065; // DOS timer ticks per second
/*
    // Get the initial tick count
    start_ticks = get_ticks();

    // Delay loop for approximately 1 second
    while (get_ticks() - start_ticks < ticks_per_sec) {
        delay_count++;
    }

    // Get the final tick count
    end_ticks = get_ticks();

    // Calculate elapsed ticks
    elapsed_ticks = end_ticks - start_ticks;

    // Calculate CPU speed in MHz
    unsigned long speed = delay_count / elapsed_ticks;
    printf("%lu MHz\n", speed);
*/
printf("0 Mhz\n");
}

// Get system ticks since startup
unsigned long get_ticks(void) {
    union REGS regs;
    regs.h.ah = 0x00;
    int86(0x1A, &regs, &regs);
    return (((unsigned long)regs.x.cx << 16) | regs.x.dx);
}

// Main function
int main(void) {
    _clearscreen(_GCLEARSCREEN);
    _settextwindow(1, 1, 25, 80);
    
    // Print Tandy logo
    unsigned char far *bios_check = (unsigned char far *)0xFFFF000EL;
    unsigned char far *tandy_check = (unsigned char far *)0xFC000000L;

    if (*bios_check != 0xFF) {
print_dosbox_logo();
    }


  if (*tandy_check != 0x21) {
// print_pcjr_logo ??
    } else {    print_tandy_logo(); }


    
    // Move cursor to the right of the logo for system info
    _settextwindow(2, 40, 25, 80);
    
    _outtext("OS: "); dosver();
    _outtext("\nShell: "); printf("%s\n", getenv("COMSPEC"));
    _outtext("\nFloppy drives: "); floppy();
    _outtext("\nDisk: "); disksize(0);
    _outtext("\nBase Memory: "); base_memory();
    _outtext("\nExt. Memory: "); extended_memory();
    _outtext("\nFloating Point Unit: "); fpu();
    _outtext("\nComputer Type: "); detect_tandy();
    _outtext("\nVideo Mode: "); detect_tandy_mode();
    _outtext("\nCPU Type: "); detect_cpu();
    _outtext("\nCPU Speed: "); detect_cpu_speed();

 if (detect_sn76496()) {
        _outtext("\nTexas Instruments SN76496 Sound Chip.");
        play_startup_sound();  // Play a quick beep!
    } else {
        _outtext("\nSN76496 Sound Chip not detected.");
    }
    

 if (detect_8253_timer()) {
        _outtext("\nIntel 8253 Timer Chip detected.\n");
    } else {
        _outtext("\n8253 Timer Chip not detected.\n");
    }
    _settextwindow(9, 1, 25, 80);  // Reset text window
    _outtext("\n");
    
    // Display color palette bars
    print_color_bars();
    
    return 0;
}
