/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "lcd_st7036.h"

static byte Cursor;

static byte put_byte (byte b) {

	register int i;

	for (i = 0; i < 8; i++) {
		lcd_clkl;
		if (b & 0x80)
			lcd_outh;
		else
			lcd_outl;
		lcd_clkh;
		b <<= 1;
	}
}

static void put_cmd (byte b) {

	lcd_select_cmd;
	put_byte (b);
	udelay (27);
}

#define	set_addr(pos)	put_cmd ((byte)((pos) | 0x80))

static void put_data (byte b) {

	lcd_select_data;
	put_byte (b);
	udelay (27);
}

void zz_lcd_init () {

	Cursor = 0;

	lcd_ini_regs;

	lcd_start;
	mdelay (100);
	put_cmd (0x39);
	put_cmd (0x14);
	put_cmd (0x55);
	put_cmd (0x6d);
	mdelay (200);
	put_cmd (0x78);
	put_cmd (0x0f);
	put_cmd (0x01);
	mdelay (10);
	put_cmd (0x06);
	mdelay (10);
	lcd_stop;

	lcd_off ();
}

void lcd_on (word params) {

	lcd_start;
	put_cmd ((params & LCD_CURSOR_ON) ? 0x0f : 0x0c);
	lcd_stop;
}
	
void lcd_off () {

	lcd_start;
	put_cmd (0x08);
	lcd_stop;

}

void lcd_clear (word from, word n) {

	if (n == 0 || (from == 0 && n == LCD_N_CHARS)) {
		Cursor = 0;
		lcd_start;
		put_cmd (0x01);
		mdelay (2);
		put_cmd (0x02);
		mdelay (2);
		lcd_stop;
	} else if (from < LCD_N_CHARS) {
		lcd_start;
		set_addr (from > 15 ? (0x40 - 16) + from : from);
		while (n-- && from < 32) {
			put_data (' ');
			if (from == 15) 
				// Switch to the second row
				set_addr (0x40);
			from++;
		}
		lcd_stop;
	}
}


void lcd_write (word from, const char *str) {

	if (from == WNONE)
		from = Cursor;
	if (*str != '\0' && from < LCD_N_CHARS) {
		lcd_start;
		// Set the memory address
		set_addr (from > 15 ? (0x40 - 16) + from : from);
		while (from < 32) {
			put_data (*str++);
			if (*str == '\0')
				return;
			if (from == 15) 
				// Switch to the second row
				set_addr (0x40);
			from++;
		}
		lcd_stop;
	}
	Cursor = from;
}

void lcd_putchar (char c) {

	if (Cursor < LCD_N_CHARS) {
		lcd_start;
		set_addr (Cursor > 15 ? (0x40 - 16) + Cursor : Cursor);
		put_data (c);
		Cursor++;
		lcd_stop;
	}
}

word lcd_nlines () { return LCD_N_LINES; }
word lcd_llength () { return LCD_LINE_LENGTH; }
