/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_lcd_ccrf_h__
#define	__pg_lcd_ccrf_h__

// Driver for Olimex CCRF LCD
//+++ lcd_ccrf.c

#define	LCD_N_CHARS	9

void lcd_on (word), lcd_off (), lcd_clear (word, word), lcd_putchar (char),
	lcd_write (word, const char*), lcd_setp (word);


#define	lcd_nlines() 1
#define	lcd_llength() LCD_N_CHARS

#endif
