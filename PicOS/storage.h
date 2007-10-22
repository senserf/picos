#ifndef	__pg_storage_h
#define	__pg_storage_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "board_storage.h"

#ifdef	EEPROM_INIT_ON_KEY_PRESSED

#define	ee_postinit \
	do { \
		int i; \
		if (EEPROM_INIT_ON_KEY_PRESSED) { \
			for (i = 0; i < 4; i++) { \
				leds (0,1); leds (1,1); leds (2,1); leds (3,1);\
				mdelay (256); \
				leds (0,0); leds (1,0); leds (2,0); leds (3,0);\
				mdelay (256); \
			} \
			ee_erase (WNONE, 0, 0); \
			while (EEPROM_INIT_ON_KEY_PRESSED); \
		} \
	} while (0)
#else

#define	ee_postinit do { } while (0)

#endif	/* EEPROM_INIT_ON_KEY_PRESET */

#endif
