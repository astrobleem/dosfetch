/* A demo of the SOUND routine from GRAPHICS.LIB
   written by J. R. Applegate - Colorado School of Mines */

#include "graphics.h"

main()
{
int i;
for (i=440;i<1600;i++)
    {
    sound(1, i, 16);
    sound(2, i+20, 16);
    sound(3, i+50, 16);
    }
for (i=1600;i>400;i--)
    {
    sound(1, i, 16);
    sound(2, i-20, 16);
    sound(3, i+50, 16);
    }
sound(1, 20, 0);
sound(2, 20, 0);
sound(3, 20, 0);
}
