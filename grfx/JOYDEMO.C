/* JOYDEMO.C - a demo of J. R. Applegate Joystick functions for C */

#include "graphics.h"

main()
{
cls();
gotoxy(30,2);
printf("Joystick Function Test");
gotoxy(22,8);
printf("Left");
gotoxy(51,8);
printf("Right");
gotoxy(10,10);
printf("X");
gotoxy(10,11);
printf("Y");
gotoxy(10,15);
printf("RED");
gotoxy(10,16);
printf("BLACK");

while (button(LEFT_RED))
    {
    gotoxy(25,10);
    printf("%3d", stick(LEFT_X));
    gotoxy(25,11);
    printf("%3d", stick(LEFT_Y));
    gotoxy(55,10);
    printf("%3d", stick(RIGHT_X));
    gotoxy(55,11);
    printf("%3d", stick(RIGHT_Y));
    gotoxy(25,15);
    printf("%d", button(LEFT_RED));
    gotoxy(25,16);
    printf("%d", button(LEFT_BLACK));
    gotoxy(55,15);
    printf("%d", button(RIGHT_RED));
    gotoxy(55,16);
    printf("%d", button(RIGHT_BLACK));
    }
}
