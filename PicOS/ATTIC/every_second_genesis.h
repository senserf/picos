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
#if EEPROM_DRIVER

	if (GENESIS_RESET_KEY_PRESSED) {
		// Disable all interrupts, we are going down
		cli;
		leds (0, 1); leds (2, 1); leds (3, 1);
		mdelay (512);
		leds (0, 0); leds (2, 0); leds (3, 0);
		// If the key is released within 2 seconds, do normal reset
		mdelay (4096);
		if (GENESIS_RESET_KEY_PRESSED) {
			leds (0, 1); leds (2, 1); leds (3, 1);
			for (zz_mintk = 0; zz_mintk < (EE_SIZE+31)/32;
			    zz_mintk++) {
				ee_read (zz_mintk << 5, (byte*)__PCB, 32);
				// Do not erase what appears to be erased
				for (zz_lostk = 0; zz_lostk < 32; zz_lostk++)
					if (((byte*)__PCB) [zz_lostk] != 0xff)
						break;
				if (zz_lostk >= 32)
					continue;
				for (zz_lostk = 0; zz_lostk < 32; zz_lostk++)
					((byte*)__PCB) [zz_lostk] = 0xff;
				ee_write (zz_mintk << 5, (byte*)__PCB, 32);
			}
			for (zz_lostk = 0; zz_lostk < 8; zz_lostk++) {
				leds (0, 1); leds (2, 1); leds (3, 1);
				mdelay (200);
				leds (0, 0); leds (2, 0); leds (3, 0);
				mdelay (200);
			}
		}
		reset ();
	}
#endif

#endif
