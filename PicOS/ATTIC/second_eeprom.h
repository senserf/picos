#ifndef	__pg_second_eeprom_h
#define __pg_second_eeprom_h

#ifdef EEPROM_PRESENT
#ifdef EEPROM_INIT_ON_KEY_PRESSED

	if (EEPROM_INIT_ON_KEY_PRESSED) {

#if WATCHDOG_ENABLED
		WATCHDOG_STOP;
#endif
		// Disable all interrupts, we are going down
		cli;
		leds (0, 1); leds (1, 1); leds (2, 1); leds (3, 1);
		mdelay (512);
		leds (0, 0); leds (1, 0); leds (2, 0); leds (3, 0);
		// If the key is released within 2 seconds, do normal reset
		mdelay (4096);
		if (EEPROM_INIT_ON_KEY_PRESSED) {
			leds (0, 1); leds (1, 1); leds (2, 1); leds (3, 1);
			// EEPROM_RAW_ERASE;
			ee_erase (WNONE, 0, 0);
			for (zz_lostk = 0; zz_lostk < 8; zz_lostk++) {
				leds (0,1); leds (1,1); leds (2,1); leds (3,1);
				mdelay (200);
				leds (0,0); leds (1,0); leds (2,0); leds (3,0);
				mdelay (200);
			}
		}
		while (EEPROM_INIT_ON_KEY_PRESSED);
		reset ();
	}
#endif	/* EEPROM_INIT_ON_KEY_PRESSED */
#endif	/* EEPROM_PRESENT */

#endif
