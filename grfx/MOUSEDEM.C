/* Demo of MS-Mouse Driver routines for MSC 4.0 */
#include <graphics.h>
#include <mouse.h>
#include <stdio.h>

main()
{
extern int mse_xin;
extern int mse_yin;

mode(6);
palette(0, 7);
gotoxy(11,0);
printf("Left Button DRAW  -  Right Button ERASE  -  Both QUIT");
m_reset();
m_showcur();
while (m_keyprs(0)!=3)
    {
    if (m_keyprs(0)==1)
         {
         m_readloc();
         point(mse_xin, mse_yin, 7);
         }
    if (m_keyprs(0)==2)
         {
         m_readloc();
         point(mse_xin, mse_yin, 0);
         }
    }
m_reset();
mode(CO80);
}
