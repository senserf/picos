#ifndef __board_second_h
#define __board_second_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/*
 * Action to be performed every second. Preempts (replaces)
 * second_iflash.h and second_eeprom.h
 */

#ifdef	RESET_ON_KEY_PRESSED

#define	board_key_erase_action	do { bkea_if; } while (0)

#if	INFO_FLASH
#define	bkea_if	if_erase (-1)
#else
#define	bkea_if	CNOP
#endif

#endif	/* RESET_ON_KEY_PRESSED */

#endif
