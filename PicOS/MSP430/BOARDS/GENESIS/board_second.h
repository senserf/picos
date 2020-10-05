/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __board_second_h
#define __board_second_h

#define	EMERGENCY_RESET_CONDITION	SOFT_RESET_BUTTON_PRESSED

// Blink all LEDs, wait for 2 sec, if key still pressed, blink LEDs 8 times,
// erase EEPROM, and blink once; finally reset
#define	EMERGENCY_RESET_ACTION	do { \
		all_leds_blink; \
		mdelay (2048); \
		if (SOFT_RESET_BUTTON_PRESSED) { \
			for (__pi_mintk = 0; __pi_mintk < 8; __pi_mintk++) \
				all_leds_blink; \
			MEMORY_ERASE_ACTION; \
			all_leds_blink; \
			while (SOFT_RESET_BUTTON_PRESSED); \
		} \
	} while (0)

#define	MEMORY_ERASE_ACTION	ee_init_erase ()

#define	EMERGENCY_STARTUP_CONDITION	SOFT_RESET_BUTTON_PRESSED
#define	EMERGENCY_STARTUP_ACTION	EMERGENCY_RESET_ACTION

#endif
