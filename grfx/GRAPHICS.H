/* Screen Modes */
#define BW40         0
#define CO40         1
#define BW80         2
#define CO80         3
#define MEDIUM_4     4
#define MEDIUM_2     5
#define HIGH_2       6
/* Tandy 1000 - PC Jr. Modes */
#define LOW_16       8
#define MEDIUM_16    9
#define HIGH_4       10

/* Colors */
#define BLACK         0
#define BLUE          1
#define GREEN         2
#define CYAN          3
#define RED           4
#define MAGENTA       5
#define YELLOW        6
#define LIGHT_GREY    7
#define DARK_GREY     8
#define LIGHT_BLUE    9
#define LIGHT_GREEN   10
#define LIGHT_CYAN    11
#define LIGHT_RED     12
#define LIGHT_MAGENTA 13
#define LIGHT_YELLOW  14
#define WHITE         15

/* normal aspect for circles */
#define ASPECT        270

/* JOYSTICK Definitions */
#define LEFT_X        0
#define LEFT_Y        1
#define RIGHT_X       2
#define RIGHT_Y       3
/* BUTTONS */
#define LEFT_RED      4
#define LEFT_BLACK    5
#define RIGHT_RED     6
#define RIGHT_BLACK   7

void mode();
unsigned char getmode();
unsigned char wherex();
unsigned char wherey();
void gotoxy();
void cls();
unsigned char ppoint();
void palette();
void point();
void line();
void box();
void circle();
void sound();
int stick();
int button();
