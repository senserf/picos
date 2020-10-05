/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_lcdg_n6100p_h__
#define	__pg_lcdg_n6100p_h__

#include "pins.h"
#include "lcd_sys.h"

//+++ lcdg_n6100p.c

#define	COLOR_WHITE	0
#define COLOR_BLACK	1
#define COLOR_RED	2
#define COLOR_GREEN	3
#define COLOR_BLUE	4
#define COLOR_CYAN	5
#define COLOR_MAGENTA	6
#define COLOR_YELLOW	7
#define COLOR_BROWN	8
#define COLOR_ORANGE	9
#define COLOR_PINK	10

// We ignore the boundary
#define	LCDG_MAXX	129
#define	LCDG_MAXY	129

void lcdg_cmd (byte, const byte*, byte);
void lcdg_on (byte);
void lcdg_off (void);
void lcdg_set (byte, byte, byte, byte);
void lcdg_get (byte*, byte*, byte*, byte*);
void lcdg_setc (byte, byte);
void lcdg_clear ();
void lcdg_render (byte, byte, const byte*, word);

#define lcdg_end()	CNOP

#ifdef	LCDG_SETTABLE_CTABLE
void lcdg_setct (byte, word);
#else
#define	lcdg_setct(a,b)	CNOP
#endif

#ifdef	LCDG_FONT_BASE

word lcdg_font (byte);
byte lcdg_cwidth (void);
byte lcdg_cheight (void);
word lcdg_sett (byte, byte, byte, byte);
void lcdg_ec (byte, byte, byte);
void lcdg_el (byte, byte);
void lcdg_wl (const char*, word, byte, byte);

#endif	/* LCDG_FONT_BASE */

#endif
