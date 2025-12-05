#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include <graph.h>

// 16x16 Tandy "T" Logo Bitmap
// 1 = Pixel On, 0 = Pixel Off
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

void draw_sprite(int x, int y, int color) {
    int row, col;
    unsigned short row_data;
    
    _setcolor(color);
    
    for (row = 0; row < 16; row++) {
        row_data = tandy_logo_16x16[row];
        for (col = 0; col < 16; col++) {
            // Check if the bit at this column is set (from left to right)
            // 0x8000 is the leftmost bit (10000000 00000000)
            if (row_data & (0x8000 >> col)) {
                _setpixel(x + col, y + row);
            }
        }
    }
}

void tandy_sprite_test(void) {
    printf("Switching to CGA Graphics Mode (Mode 4)...\n");
    printf("Press any key to draw the logo...\n");
    getch();
    
    // Switch to CGA Mode (320x200x4)
    // This is the "Safe Mode" that worked in the debug test
    if (_setvideomode(_MRES4COLOR) == 0) {
        printf("Error: Could not set Mode 4!\n");
        return;
    }
    
    // Draw the Logo in the center
    // Screen is 320x200. Center is 160,100.
    // Logo is 16x16. Top-left should be 152, 92.
    
    // Draw in White (Color 3)
    draw_sprite(152, 92, 3);
    
    // Draw some text below it
    _settextcolor(3);
    _settextposition(15, 10);
    _outtext("The Tandy Logo (Sprite)");
    
    getch();
    _setvideomode(_DEFAULTMODE);
}

int main(void) {
    tandy_sprite_test();
    return 0;
}
