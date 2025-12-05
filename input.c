#include <i86.h>
#include <dos.h>
#include "input.h"

// ==========================================
// Mouse Support (INT 33h)
// ==========================================

int mouse_init(void) {
    union REGS regs;
    regs.w.ax = 0;
    int86(0x33, &regs, &regs);
    return regs.w.ax; // 0 if not installed, -1 (FFFF) if installed
}

void mouse_show(void) {
    union REGS regs;
    regs.w.ax = 1;
    int86(0x33, &regs, &regs);
}

void mouse_hide(void) {
    union REGS regs;
    regs.w.ax = 2;
    int86(0x33, &regs, &regs);
}

int mouse_status(int *x, int *y) {
    union REGS regs;
    regs.w.ax = 3;
    int86(0x33, &regs, &regs);
    *x = regs.w.cx;
    *y = regs.w.dx;
    *y = regs.w.dx;
    return regs.w.bx; // Button status
}

void mouse_reset(void) {
    union REGS regs;
    regs.w.ax = 0;
    int86(0x33, &regs, &regs);
}
