#include <stdint.h>

#include "stm32f3xx_hal.h"

#include "display.h"
#include "usdelay.h"

static struct displaypindef pindefs[DISPLAY_PINCOUNT];
static int (*displaypins)(int p2, int p3, int p4, int p5, int p6,
	int p7, int p8, int p9, int p10, int p11) = NULL;

int downpins()
{
	int i;

	for (i = 0; i < DISPLAY_PINCOUNT; ++i) {
		HAL_GPIO_WritePin(pindefs[i].gpio,
			pindefs[i].pin, GPIO_PIN_RESET);
	}

	return 0;
}

int displaypins8(int p2, int p3, int p4, int p5, int p6, int p7,
	int p8, int p9, int p10, int p11)
{
	HAL_GPIO_WritePin(pindefs[0].gpio, pindefs[0].pin,
		pinstate(p2));
	HAL_GPIO_WritePin(pindefs[1].gpio, pindefs[1].pin,
		pinstate(p3));

	HAL_GPIO_WritePin(pindefs[3].gpio, pindefs[3].pin,
		pinstate(p4));
	HAL_GPIO_WritePin(pindefs[4].gpio, pindefs[4].pin,
		pinstate(p5));
	HAL_GPIO_WritePin(pindefs[5].gpio, pindefs[5].pin,
		pinstate(p6));
	HAL_GPIO_WritePin(pindefs[6].gpio, pindefs[6].pin,
		pinstate(p7));

	HAL_GPIO_WritePin(pindefs[7].gpio, pindefs[7].pin,
		pinstate(p8));
	HAL_GPIO_WritePin(pindefs[8].gpio, pindefs[8].pin,
		pinstate(p9));
	HAL_GPIO_WritePin(pindefs[9].gpio, pindefs[9].pin,
		pinstate(p10));
	HAL_GPIO_WritePin(pindefs[10].gpio, pindefs[10].pin,
		pinstate(p11));

	USDELAY(40);
	HAL_GPIO_WritePin(pindefs[2].gpio, pindefs[2].pin,
		pinstate(1));
	USDELAY(40);
	HAL_GPIO_WritePin(pindefs[2].gpio, pindefs[2].pin,
		pinstate(0));
	USDELAY(40);
	
	return 0;
}


int displaypins4_half(int p0, int p1, int p2, int p3)
{
	HAL_GPIO_WritePin(pindefs[7].gpio, pindefs[7].pin,
		pinstate(p0));
	HAL_GPIO_WritePin(pindefs[8].gpio, pindefs[8].pin,
		pinstate(p1));
	HAL_GPIO_WritePin(pindefs[9].gpio, pindefs[9].pin,
		pinstate(p2));
	HAL_GPIO_WritePin(pindefs[10].gpio, pindefs[10].pin,
		pinstate(p3));

	USDELAY(40);
	HAL_GPIO_WritePin(pindefs[2].gpio, pindefs[2].pin,
		pinstate(1));
	USDELAY(40);
	HAL_GPIO_WritePin(pindefs[2].gpio, pindefs[2].pin,
		pinstate(0));
	USDELAY(40);
	
	return 0;
}



int displaypins4(int p2, int p3, int p4, int p5, int p6, int p7,
	int p8, int p9, int p10, int p11)
{
	HAL_GPIO_WritePin(pindefs[0].gpio, pindefs[0].pin,
		pinstate(p2));
	HAL_GPIO_WritePin(pindefs[1].gpio, pindefs[1].pin,
		pinstate(p3));

	displaypins4_half(p8, p9, p10, p11);
	displaypins4_half(p4, p5, p6, p7);

	return 0;
}

uint8_t mapchar(uint16_t c)
{
	uint8_t c0, c1;

	c0 = c >> 8;
	c1 = c &= 0xff;

	if (c0 == 0xd0) {
		switch (c1) {
		case 0x90: return 0x41;		case 0x91: return 0xa0;
		case 0x92: return 0x42;		case 0x93: return 0xa1;
		case 0x94: return 0xe0;		case 0x95: return 0x45;
		case 0x81: return 0xa2;		case 0x96: return 0xa3;
		case 0x97: return 0xa4;		case 0x98: return 0xa5;
		case 0x99: return 0xa6;		case 0x9a: return 0x4b;
		case 0x9b: return 0xa7;		case 0x9c: return 0x4d;
		case 0x9d: return 0x48;		case 0x9e: return 0x4f;
		case 0x9f: return 0xa8;		case 0xa0: return 0x50;
		case 0xa1: return 0x43;		case 0xa2: return 0x54;
		case 0xa3: return 0xa9;		case 0xa4: return 0xaa;
		case 0xa5: return 0x58;		case 0xa6: return 0xe1;
		case 0xa7: return 0xab;		case 0xa8: return 0xac;
		case 0xa9: return 0xe2;		case 0xaa: return 0xad;
		case 0xab: return 0xae;		case 0xac: return 0xad;
		case 0xad: return 0xaf;		case 0xae: return 0xb0;
		case 0xaf: return 0xb1;		case 0xb0: return 0x61;
		case 0xb1: return 0xb2;		case 0xb2: return 0xb3;
		case 0xb3: return 0xb4;		case 0xb4: return 0xe3;
		case 0xb5: return 0x65;		case 0xb6: return 0xb6;
		case 0xb7: return 0xb7;		case 0xb8: return 0xb8;
		case 0xb9: return 0xb9;		case 0xba: return 0xba;
		case 0xbb: return 0xbb;		case 0xbc: return 0xbc;
		case 0xbd: return 0xbd;		case 0xbe: return 0x6f;
		case 0xbf: return 0xbe;
		}
	}
	else if (c0 == 0xd1) {
		switch (c1) {
		case 0x80: return 0x70;		case 0x81: return 0x63;
		case 0x82: return 0xbf;		case 0x83: return 0x79;	
		case 0x84: return 0xe4;		case 0x85: return 0x78;
		case 0x86: return 0xe5;		case 0x87: return 0xc0;
		case 0x88: return 0xc1;		case 0x89: return 0xe6;
		case 0x8a: return 0xc2;		case 0x8b: return 0xc3;
		case 0x8c: return 0xc4;		case 0x8d: return 0xc5;
		case 0x8e: return 0xc6;		case 0x8f: return 0xc7;
		case 0x91: return 0xb5;
		}
	}
	else if (c1 >= 0x21 || c1 < 0x7a)
		return c1;

	return c;
}

int displaychar(uint16_t c)
{
	c = mapchar(c);

	displaypins(1, 0,
		(c >> 0) & 0x1, (c >> 1) & 0x1,
		(c >> 2) & 0x1, (c >> 3) & 0x1, 
		(c >> 4) & 0x1, (c >> 5) & 0x1,
		(c >> 6) & 0x1, (c >> 7) & 0x1
	);
	
	return 0;
}

int displaystring(char *s)
{
	char *p;

	for (p = s; *p != '\0'; ++p) {
		uint16_t c;

		c = *p & 0xff;

		if (*p == 0xd0 || *p == 0xd1)
			c = (c << 8) | *++p;

		displaychar(c);
	}
	
	return 0;
}

int displayclear()
{
	displaypins(0, 0, 1, 0, 0, 0, 0, 0, 0, 0);
	HAL_Delay(2);
	
	return 0;
}

int displayset(int bits, int lines, int size)
{
	displaypins(0, 0, 0, 0, size, lines, bits, 1, 0, 0);
	
	return 0;
}

int displayonoff(int display, int cursor, int blink)
{
	displaypins(0, 0, blink, cursor, display, 1, 0, 0, 0, 0);
	
	return 0;
}

int displayentry(int direction, int screenshift)
{
	displaypins(0, 0, screenshift, direction, 1, 0, 0, 0, 0, 0);
	
	return 0;
}

int displaypos(int v, int h)
{
	uint8_t pos;

	pos = (v ? 0x40 : 0x0) + h;

	displaypins(0, 0,
		(pos >> 0) & 0x1, (pos >> 1) & 0x1,
		(pos >> 2) & 0x1, (pos >> 3) & 0x1,
		(pos >> 4) & 0x1, (pos >> 5) & 0x1,
		(pos >> 6) & 0x1, 1
	);
	
	return 0;
}

int displayinit(struct displaypindef defs[DISPLAY_PINCOUNT], int mode)
{
	int i;

	for (i = 0; i < DISPLAY_PINCOUNT; ++i)
		pindefs[i] = defs[i];

	for (i = 0; i < DISPLAY_PINCOUNT; ++i) {
		if (mode == DISPLAY_4BIT && i >= 3 && i <= 6)
			continue;

		HAL_GPIO_WritePin(pindefs[i].gpio,
			pindefs[i].pin, GPIO_PIN_RESET);
	}

	displaypins = (mode == DISPLAY_8BIT)
		? displaypins8 : displaypins4;

	if (mode == DISPLAY_8BIT) {
		HAL_Delay(20);
		displaypins8(0, 0, 0, 0, 0, 0, 1, 1, 0, 0);
		HAL_Delay(5);
		displaypins8(0, 0, 0, 0, 0, 0, 1, 1, 0, 0);
		HAL_Delay(1);
		displaypins8(0, 0, 0, 0, 0, 0, 1, 1, 0, 0);
	}
	if (mode == DISPLAY_4BIT) {
		HAL_Delay(20);
		displaypins4_half(1, 1, 0, 0);
		HAL_Delay(5);
		displaypins4_half(1, 1, 0, 0);
		HAL_Delay(1);
		displaypins4_half(1, 1, 0, 0);


		HAL_Delay(1);
		displaypins4(0, 0,	0, 1, 0, 0, 1, 0, 0, 0);
		HAL_Delay(1);
	}
	
	displayset(mode, DISPLAY_2LINE, DISPLAY_SMALLFONT);
	displayonoff(DISPLAY_OFF, DISPLAY_NOCURSOR, DISPLAY_NOBLINK);
	displayclear();
	displayentry(DISPLAY_FORWARD, DISPLAY_NOSHIFTSCREEN);
	
	return 0;
}
