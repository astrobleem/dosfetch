#ifndef INPUT_H
#define INPUT_H

// Mouse Support
int mouse_init(void);
void mouse_show(void);
void mouse_hide(void);
int mouse_status(int *x, int *y);
void mouse_reset(void);

#endif
