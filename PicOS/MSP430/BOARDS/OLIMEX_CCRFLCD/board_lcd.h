/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// SDA and SCL are pulled up externally
#define	lcd_ccrf_sda_low	_BIS (P3DIR, 0x80)
#define	lcd_ccrf_sda_high	_BIC (P3DIR, 0x80)
#define	lcd_ccrf_scl_low	_BIS (P3DIR, 0x40)
#define	lcd_ccrf_scl_high	_BIC (P3DIR, 0x40)
#define	lcd_ccrf_delay		udelay (10)

#if 0
#define	lcd_diag_start		lcd_ccrf_diag_start ()
#define	lcd_diag_wchar(c)	lcd_ccrf_diag_char (c)
#define	lcd_diag_wait		CNOP
#define	lcd_diag_stop		CNOP
#endif

#include "lcd_ccrf.h"
