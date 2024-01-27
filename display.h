#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#define DISPLAY_8BIT 1
#define DISPLAY_4BIT 0
#define DISPLAY_2LINE 1
#define DISPLAY_1LINE 0
#define DISPLAY_BIGFONT 1
#define DISPLAY_SMALLFONT 0

#define DISPLAY_ON 1
#define DISPLAY_OFF 0
#define DISPLAY_CURSOR 1
#define DISPLAY_NOCURSOR 0
#define DISPLAY_BLINK 1
#define DISPLAY_NOBLINK 0

#define DISPLAY_FORWARD 1
#define DISPLAY_BACKWARD 0
#define DISPLAY_SHIFTSCREEN 1
#define DISPLAY_NOSHIFTSCREEN 0

#define DISPLAY_PINCOUNT 11

#define pinstate(s) ((s) ? GPIO_PIN_SET : GPIO_PIN_RESET)

struct displaypindef {
	uint32_t gpio;
	uint32_t pin;
};

int downpins();

uint8_t mapchar(uint16_t c);

int displaypins8(int p2, int p3, int p4, int p5, int p6, int p7,
	int p8, int p9, int p10, int p11);

int displaychar(uint16_t c);

int displaystring(char *s);

int displayclear();

int displayset(int bits, int lines, int size);

int displayonoff(int display, int cursor, int blink);

int displayentry(int direction, int screenshift);

int displaypos(int v, int h);

int displayinit(struct displaypindef def[DISPLAY_PINCOUNT], int mode);

#endif
