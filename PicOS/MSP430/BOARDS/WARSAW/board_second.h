#ifndef __board_second_h
#define __board_second_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/*
 * Action to be performed every second. Preempts (replaces)
 * second_iflash.h and second_eeprom.h
 */

#ifdef	RESET_ON_KEY_PRESSED

#define	board_key_erase_action	do { bkea_ee; bkea_sd; bkea_if; } while (0)

#ifdef	EEPROM_PRESENT
#define	bkea_ee	ee_init_erase ()
#else
#define	bkea_ee	CNOP
#endif

#ifdef	SDCARD_PRESENT
#define	bkea_sd	sd_init_erase ()
#else
#define	bkea_sd	CNOP
#endif

#if	INFO_FLASH
#define	bkea_if	if_erase (-1)
#else
#define	bkea_if	CNOP
#endif

#endif	/* RESET_ON_KEY_PRESSED */

#endif
