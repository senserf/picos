#ifndef	__pg_lcdg_n6100p_h__
#define	__pg_lcdg_n6100p_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "kernel.h"
#include "lcd_sys.h"
#include "pins.h"

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

// Flag bits
#define	LCDGF_8BPP	1

// We ignore the boundary
#define	LCDG_MAXX	129
#define	LCDG_MAXY	129
#define	LCDG_XOFF	1
#define	LCDG_YOFF	1
#define	LCDG_MAXXP	(LCDG_XOFF+LCDG_MAXX)
#define	LCDG_MAXYP	(LCDG_YOFF+LCDG_MAXY)

#endif
