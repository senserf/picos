#ifndef	__pg_every_second_genesis_h
#define __pg_every_second_genesis_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/*
 * Resets the Genesis board: note we only do this if EEPROM is present. This
 * gives us a way to disable the reset button (which is on low) for experiments
 * with prototype boards.
 */
#if TARGET_BOARD == BOARD_VERSA2

	if (VERSA2_RESET_KEY_PRESSED) {
		// Disable all interrupts, we are going down
		cli;
		leds (0, 1); leds (1, 1); leds (2, 1);
		mdelay (512);
		leds (0, 0); leds (1, 0); leds (2, 0);
#if INFO_FLASH
		// If the key is released within 2 seconds, do normal reset
		mdelay (4096);
		if (VERSA2_RESET_KEY_PRESSED) {
			leds (0, 1); leds (1, 1); leds (2, 1);
			if_erase ();
			for (zz_lostk = 0; zz_lostk < 8; zz_lostk++) {
				leds (0, 1); leds (1, 1); leds (2, 1);
				mdelay (200);
				leds (0, 0); leds (1, 0); leds (2, 0);
				mdelay (200);
			}
		}
#endif
		while (VERSA2_RESET_KEY_PRESSED);
		reset ();
	}
#endif

#endif