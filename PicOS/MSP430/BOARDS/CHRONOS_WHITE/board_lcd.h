/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// LCD memory base
#define LCD_MEM_BASE          			((byte*)0x0A20)

// LCD blink memory base
#define	LCD_BLN_BASE				(LCD_MEM_BASE + 0x20)

#define	lcd_diag_start		ezlcd_diag_start ()
#define	lcd_diag_wchar(c)	ezlcd_diag_char (c)
#define	lcd_diag_wait		CNOP
#define	lcd_diag_stop		CNOP

#include "ez430_lcd.h"
