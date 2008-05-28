#ifndef __board_second_h
#define __board_second_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/*
 * Action to be performed every second. Preempts (replaces)
 * second_iflash.h and second_eeprom.h
 */

#ifdef EEPROM_INIT_ON_KEY_PRESSED

	if (EEPROM_INIT_ON_KEY_PRESSED) {

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
			 if (!EEPROM_INIT_ON_KEY_PRESSED) {
#if INFO_FLASH
		// This may be convenient to switch between functional
		// node types, e.g. router / leaf, with no UI available.
		// This here is specific for BoardTest praxis.
				 if (if_read (IFLASH_SIZE -1) == 0xFFFE)
					 if_write (IFLASH_SIZE -1, 0xFFFC);
#endif
				 reset();
			 }
		 }
#if INFO_FLASH
		 // erase fim, eeprom
		 if_erase (-1);
#endif
#ifdef	EEPROM_PRESENT
		 ee_erase (WNONE, 0, 0);
#endif
		 for (zz_lostk = 0; zz_lostk < 8; zz_lostk++) {
			 leds (0,1); leds (1,1); leds (2,1);
			 mdelay (200);
			 leds (0,0); leds (1,0); leds (2,0);
			 mdelay (200);
		 }
		 while (EEPROM_INIT_ON_KEY_PRESSED) ;
		 reset();
	}
#endif

#endif
