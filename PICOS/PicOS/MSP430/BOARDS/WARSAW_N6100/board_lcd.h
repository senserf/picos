/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "pins.h"
//
// Basic pin access for Nokia LCD. Note: on this board we cannot take
// advantage of the SPI as the respective pins are not available
//
#define	nlcd_rst_up		_BIS (P4OUT, 0x10)
#define	nlcd_rst_down		_BIC (P4OUT, 0x10)
#define nlcd_cs_up		_BIS (P4OUT, 0x80)
#define	nlcd_cs_down		_BIC (P4OUT, 0x80)
#define	nlcd_data_up		_BIS (P4OUT, 0x20)
#define	nlcd_data_down		_BIC (P4OUT, 0x20)
#define	nlcd_clk_up		_BIS (P4OUT, 0x40)
#define	nlcd_clk_down		_BIC (P4OUT, 0x40)

// This tells the low-level driver to use raw interface
#define	NOKIA_LCD_RAW
#define	LCDG_FONT_BASE		0
