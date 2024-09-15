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


// Define the I/O ports for the SN76496
#define SN76496_PORT_0 0xC0
#define SN76496_PORT_1 0xC1
#define SN76496_PORT_2 0xC2
#define SN76496_PORT_3 0xC3

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
void detect_cpu(void);
void detect_cpu_speed(void);
unsigned long get_ticks(void);
int detect_sn76496(void);



// Function to detect SN76496
int detect_sn76496(void) {
    unsigned char test_value = 0x55;  // Arbitrary test value
    unsigned char read_value;

    // Write test value to the SN76496 ports
    outp(SN76496_PORT_0, test_value);
    outp(SN76496_PORT_1, test_value);
    outp(SN76496_PORT_2, test_value);
    outp(SN76496_PORT_3, test_value);

    // Read back the values
    read_value = inp(SN76496_PORT_0);
    if (read_value != test_value) return 0;

    read_value = inp(SN76496_PORT_1);
    if (read_value != test_value) return 0;

    read_value = inp(SN76496_PORT_2);
    if (read_value != test_value) return 0;

    read_value = inp(SN76496_PORT_3);
    if (read_value != test_value) return 0;

    // If all ports return the test value, the chip is likely present
    return 1;
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
    _settextcolor(WHITE);
}

void detect_tandy(void) {
    unsigned char far *bios_check = (unsigned char far *)0xFFFF000EL;
    unsigned char far *tandy_check = (unsigned char far *)0xFC000000L;
    
    if (*bios_check != 0xFF) {
        printf("Not an IBM PC compatible\n");
        return;
    }
    
    if (*tandy_check != 0x21) {
        printf("Not a Tandy 1000 series\n");
        return;
    }
    
    // Check for Tandy 1000 SL/TL
    union REGS regs;
    struct SREGS sregs;
    regs.h.ah = 0xC0;
    int86x(0x15, &regs, &regs, &sregs);
    
    if (!regs.x.cflag) {
        unsigned char far *model_id = MK_FP(sregs.es, regs.x.bx + 2);
        if (*model_id == 0xFF) {
            printf("Tandy 1000 SL/TL detected\n");
            return;
        }
    }
    
    printf("Tandy 1000 series detected\n");
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

// CPU detection function
void detect_cpu(void) {
    union REGS regs;
    regs.h.ah = 0x00;  // CPU identification function
    int86(0x11, &regs, &regs);

    switch (regs.h.al) {
        case 0x00:
            printf("Intel 8088 or NEC V20\n");
            break;
        case 0x01:
            printf("Intel 8086\n");
            break;
        case 0x02:
            printf("Intel 80286\n");
            break;
        case 0x03:
            printf("Intel 386\n");
            break;
        case 0x04:
            printf("Intel 486\n");
            break;
        case 0x05:
            printf("Pentium\n");
            break;
        case 0x06:
            printf("Pentium Pro\n");
            break;
        default:
            printf("Unknown CPU\n");
            break;
    }
}

// CPU speed detection function
void detect_cpu_speed(void) {
    unsigned long start_ticks, end_ticks, elapsed_ticks;
    unsigned int delay_count = 0;
    unsigned long ticks_per_sec = 18.2065; // DOS timer ticks per second

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
    print_tandy_logo();
    
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
    _outtext("\nCPU Type: "); detect_cpu();
    _outtext("\nCPU Speed: "); detect_cpu_speed();
 if (detect_sn76496()) {
        _outtext("\nTexas Instruments SN76496 Sound Chip detected.\n");
    } else {
        _outtext("\nSN76496 Sound Chip not detected.\n");
    }
    
    _settextwindow(9, 1, 25, 80);  // Reset text window
    _outtext("\n");
    return 0;
}
