/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// 2 leds: red on GPIO6, green on GPIO7

#define	LED0_pin	6
#define	LED0_polarity	1		/* high == on */

#define	LED1_pin	7
#define	LED1_polarity	1

// Don't do LEDS_SAVE / LEDS_RESTORE which is obsolete (only required for
// glacier)
