/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "lcd_ccrf.h"
#include "lcd_sys.h"

// Note: I have no clue what I am doing (couldn't find any documentation of the
// LCD). I am just copying some code and data from the sloppy example that came
// with the board, optimizing things (rather drastically) along the way.

#define	lcd_code(c0,b0,c1,b1) \
	((c0) << 6) | (((b0) - 32) << 4) | ((c1) << 2) | ((b1) - 32)

static const byte lcd_bits [8] = { 
//
// There are two elements of a segment description (16 segments per position):
//	- com (one of 0, 1, 2, 3)
//	- bit (one of 32, 33, 34, 35 offset by -position number * 4)
// I am encoding those in nibbles, e.g., 1001 means com == 2, bit == 33 (i.e.,
// 32+1 (32 being the floor). Note that for position, say 3, the "bit" is
// decremented by 3*4 == 12.
//
	lcd_code (3, 34, 3, 32),
	lcd_code (0, 32, 0, 34),
	lcd_code (2, 35, 1, 35),
	lcd_code (1, 34, 3, 35),
	lcd_code (1, 33, 3, 33),
	lcd_code (1, 32, 2, 32),
	lcd_code (2, 33, 2, 34),
	lcd_code (0, 33, 0, 35)
};

#define	LCD_LOWEST_CHAR	33

static const word lcd_chars [] = {

  0x1100, /* ! */
  0x0280, /* " */
  0x0000, /* # */
  0x0000, /* $ */
  0x0000, /* % */
  0x0000, /* & */
  0x0000, /* £ */
  0x0039, /* ( */
  0x000f, /* ) */
  0x0463, /* * ->> modified to a degree symbol */ 
  0x1540, /* + */
  0x0000, /* , */
  0x0440, /* - */
  0x1000, /* . */
  0x2200, /* / */

  0x003f, /* 0 */
  0x0006, /* 1 */
  0x045b, /* 2 */
  0x044f, /* 3 */
  0x0466, /* 4 */
  0x046d, /* 5 */
  0x047d, /* 6 */
  0x0007, /* 7 */
  0x047f, /* 8 */
  0x046f, /* 9 */

  0x0000, /* : */
  0x0000, /* ; */
  0x0a00, /* < */
  0x0000, /* = */
  0x2080, /* > */
  0x0000, /* ? */
  0xffff, /* @ */

  0x0477, /* A */
  0x0a79, /* B */
  0x0039, /* C */
  0x20b0, /* D */
  0x0079, /* E */
  0x0071, /* F */
  0x047d, /* G */
  0x0476, /* H */
//  0x0006, /* I */
  0x0030, /* I edit */
  0x000e, /* J */
  0x0a70, /* K */
  0x0038, /* L */
  0x02b6, /* M */
  0x08b6, /* N */
  0x003f, /* O */
  0x0473, /* P */
  0x083f, /* Q */
  0x0c73, /* R */
  0x046d, /* S */
  0x1101, /* T */
  0x003e, /* U */
  0x2230, /* V */
  0x2836, /* W */
  0x2a80, /* X */
  0x046e, /* Y */
  0x2209, /* Z */

  0x0039, /* [ */
  0x0880, /* backslash */
  0x000f, /* ] */
  0x0001, /* ^ */
  0x0008, /* _ */
  0x0100, /* ` */

  0x1058, /* a */
  0x047c, /* b */
  0x0058, /* c */
  0x045e, /* d */
  0x2058, /* e */
  0x0471, /* f */
  0x0c0c, /* g */
  0x0474, /* h */
  0x0004, /* i */
  0x000e, /* j */
  0x0c70, /* k */
  0x0038, /* l */
  0x1454, /* m */
  0x0454, /* n */
  0x045c, /* o */
  0x0473, /* p */
  0x0467, /* q */
  0x0450, /* r */
  0x0c08, /* s */
  0x0078, /* t */
  0x001c, /* u */
  0x2010, /* v */
  0x2814, /* w */
  0x2a80, /* x */
  0x080c, /* y */
  0x2048  /* z */

};

#define	LCD_HIGHEST_CHAR (LCD_LOWEST_CHAR + (sizeof (lcd_chars)/sizeof (word)))

// ============================================================================

#define	LCD_SEGT_SIZE	18

static byte lcd_bitmap [LCD_SEGT_SIZE];
static byte Cursor = 0;

// ============================================================================

static void lcd_useg (word ix, word se, word on) {
//
// Set the segment number se at position ix to on
//
	word co;
	byte m;

	co = lcd_bits [se >> 1];
	if ((se & 1) == 0)
		co >>= 4;

	se = (co & 3) + 32 - 4 * ix;
	co = (co >> 2) & 3;

	if (se & 1)
		co |= 0x04;
	se >>= 1;

	m = 0x80 >> co;
	if (on)
		lcd_bitmap [se] |= m;
	else
		lcd_bitmap [se] &= ~m;
}

static void lcd_putc (word ix, char c) {

	word w, b;

	if ((b = (word)c) < LCD_LOWEST_CHAR || b >= LCD_HIGHEST_CHAR)
		w = 0;
	else
		w = lcd_chars [b - LCD_LOWEST_CHAR];

	for (b = 0; b < 16; b++)
		lcd_useg (ix, b, w & (1 << b));
}

static void lcd_start () {

	lcd_ccrf_sda_low;
	lcd_ccrf_scl_low;
	lcd_ccrf_delay;
}

static void lcd_stop () {

	lcd_ccrf_scl_high;
	lcd_ccrf_sda_high;
	lcd_ccrf_delay;
}

static void lcd_wb (byte b) {

	word i;

	for (i = 0; i < 8; i++) {
		if (b & 0x80)
			lcd_ccrf_sda_high;
		else
			lcd_ccrf_sda_low;
		b <<= 1;
		lcd_ccrf_scl_high;
		lcd_ccrf_delay;
		lcd_ccrf_scl_low;
	}

	// Read the ACK
	lcd_ccrf_sda_high;
	lcd_ccrf_scl_high;
	lcd_ccrf_delay;
	lcd_ccrf_scl_low;
}

static void lcd_upd (word c) {

	word i;

	lcd_start ();

	lcd_wb (0x70);
	lcd_wb (0xE0);
	lcd_wb (0x00);

	for (i = 0; i < LCD_SEGT_SIZE; i++)
		lcd_wb (c ? 0 : lcd_bitmap [i]);

	lcd_stop ();
}

void lcd_on (word params) {

	lcd_start ();

	lcd_wb (0x70);	// Device address??

	if (params == 0)
		params = 0xC8F0;

	// I have no clue what these are, except that the example uses
	// C8 and F0 calling the two "mode register" and "blink register"

	lcd_wb ((byte)(params >> 8));
	lcd_wb ((byte) params      );

	lcd_stop ();

	lcd_upd (0);
}

void lcd_off () {

	lcd_upd (1);

}

void lcd_clear (word from, word n) {

	if (n == 0) {
		from = 0;
		n = LCD_N_CHARS;
	}

	Cursor = from;

	while (from < LCD_N_CHARS) {
		lcd_putc (from, 0);
		from++;
	}

	lcd_upd (0);
}

void lcd_putchar (char c) {

	if (Cursor < LCD_N_CHARS) {
		lcd_putc (Cursor, c);
		Cursor++;
		lcd_upd (0);
	}
}

void lcd_write (word from, const char *str) {

	if (from == WNONE)
		from = Cursor;

	while (*str != '\0' && from < LCD_N_CHARS) {
		lcd_putc (from, *str);
		str++;
		from++;
	}

	Cursor = from;
	lcd_upd (0);
}

void lcd_setp (word n) {

	if ((Cursor = n) > LCD_N_CHARS)
		Cursor = LCD_N_CHARS;
}
