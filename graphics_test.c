#include <i86.h>
#include <dos.h>
#include <conio.h>

void draw_copper_bars(int speed) {
    union REGS r;
    r.h.ah = 0x10;
    r.h.al = 0x00;
}
