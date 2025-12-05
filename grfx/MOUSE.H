#include <dos.h>

union REGS m_inregs;   /*holds interrupt input parameters*/
union REGS m_outregs;  /*holds interrupt output parameters*/
int mse_xin;           /*x-input coordinate*/
int mse_yin;           /*y-input coordinate*/
int prscnt[2];         /*array for count of button presses*/
int x_lstprs[2];       /*array of x-coords at last press*/
int y_lstprs[2];       /*array of y-coords at last press*/
int rlscnt[2];         /*array for count of button releases*/
int x_lstrls[2];       /*array of x-coords at last release*/
int y_lstrls[2];       /*array of y-coords at last release*/
int h_micks;           /*horizontal motion counter*/
int v_micks;           /*vertical motion counters*/

#define M_LEFT 0
#define M_RIGHT 1
