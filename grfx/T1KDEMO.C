/* T1KDEMO.C - a demo of J. R. Applegate's GRAPHICS.LIB */

#include <stdlib.h>
#include <stdio.h>
#include "graphics.h"
#define MAXLINES 100

main()
{
int x, y, r, clr, timer, seed;
int x1[MAXLINES], y1[MAXLINES], x2[MAXLINES], y2[MAXLINES];
int lcount, x1cur, y1cur, x2cur, y2cur;
int mlflag, dx1, dy1, dx2, dy2;

cls();
gotoxy(10,10);
printf("Enter a seed integer between 1 and 32767: ");
scanf("%d", &seed);
srand(seed);

mode(MEDIUM_16);
palette(1, 0);
x = 160;
y = 100;
for (r=10 ; r <= 100 ; r+=5)
    {
    clr = rand()/2184+1;
    circle(x, y, r, clr, ASPECT);
    }
for (r=100 ; r > 5 ; r-=5)
    circle(x, y, r, BLACK, ASPECT);

x1cur=rand()/102;
y1cur=rand()/163;
x2cur=rand()/102;
y2cur=rand()/163;
dx1=rand()/2978;
dy1=rand()/2978;
dx2=rand()/2978;
dy2=rand()/2978;

mlflag=0;
lcount=0;
timer=250;

while( timer > 0 )
     {
     x1[lcount]=x1cur+dx1;
     if (x1[lcount] < 1 || x1[lcount] > 319)
          {
          dx1=-dx1;
          x1[lcount]=x1cur;
          }
     else
          x1cur=x1[lcount];

     x2[lcount]=x2cur+dx2;
     if (x2[lcount] < 1 || x2[lcount] > 319)
          {
          dx2=-dx2;
          x2[lcount]=x2cur;
          }
     else
          x2cur=x2[lcount];

     y1[lcount]=y1cur+dy1;
     if (y1[lcount] < 1 || y1[lcount] > 199)
          {
          dy1=-dy1;
          y1[lcount]=y1cur;
          }
     else
          y1cur=y1[lcount];

     y2[lcount]=y2cur+dy2;
     if (y2[lcount] < 1 || y2[lcount] > 199)
          {
          dy2=-dy2;
          y2[lcount]=y2cur;
          }
     else
          y2cur=y2[lcount];

     clr = rand()/2184+1;
     line(x1cur, y1cur, x2cur, y2cur, clr);
     timer--;

     if (++lcount == MAXLINES)
          {
          lcount=0;
          mlflag=1;
          }

     if (mlflag)
          line(x1[lcount], y1[lcount], x2[lcount], y2[lcount], BLACK);

     }
mode(CO80);
}
