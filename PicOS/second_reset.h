#ifndef	__second_reset_h
#define	__second_reset_h
//
// This is the standard code for reset/erase on a button (checked at second
// intervals)
//

#ifdef	RESET_ON_KEY_PRESSED

	// This is the dynamic condition evaluated every second
	if (RESET_ON_KEY_PRESSED) {

#if WATCHDOG_ENABLED
		 WATCHDOG_STOP;
#endif
		 // Disable all interrupts, we are going down
		 cli;
		 leds (0, 1); leds (1, 1); leds (2, 1);
		 mdelay (512);
		 leds (0, 0); leds (1, 0); leds (2, 0);

		 for (zz_lostk = 0; zz_lostk < 4; zz_lostk++) {

			 mdelay (1024);

			 if (!RESET_ON_KEY_PRESSED) {
				// The key has been dropped: do regular reset
#ifdef	board_key_reset_action
				board_key_reset_action;
#endif
				reset ();
			}
		}

		// The key remains pressed for 4 seconds

#ifdef	board_key_erase_action
		board_key_erase_action;
#endif
		for (zz_lostk = 0; zz_lostk < 8; zz_lostk++) {
			leds (0,1); leds (1,1); leds (2,1);
			mdelay (200);
			leds (0,0); leds (1,0); leds (2,0);
			mdelay (200);
		}
		while (RESET_ON_KEY_PRESSED);
		reset();
	}
#endif

#endif
