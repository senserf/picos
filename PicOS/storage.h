#ifndef	__pg_storage_h
#define	__pg_storage_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "board_pins.h"
#include "board_storage.h"
#include "iflash.h"

#ifdef	EEPROM_PRESENT

// These are actual functions

word 	ee_read  (lword, byte*, word);
word 	ee_write (word, lword, const byte*, word);
word	ee_erase (word, lword, lword);
word	ee_sync (word);
lword	ee_size (Boolean*, lword*);
word	ee_open ();
void	ee_close ();

#ifdef	RESET_ON_KEY_PRESSED
void	ee_init_erase ();
#endif

#endif	/* EEPROM_PRESENT */

#ifdef	SDCARD_PRESENT

word	sd_open ();
word	sd_read (lword, byte*, word);
word	sd_write (lword, const byte*, word);
word	sd_sync ();
void	sd_close ();
lword	sd_size ();
void	sd_idle ();

#ifdef	RESET_ON_KEY_PRESSED
void	sd_init_erase ();
#endif

#endif	/* SDCARD_PRESENT */

#if	INFO_FLASH

// Operations on the internal "information" flash (also dubbed FIM)
int	if_write (word, word);
void	if_erase (int);

#define	IFLASH		IFLASH_HARD_ADDRESS
#define	if_read(a)	(IFLASH [a])

#if	INFO_FLASH > 1
// Code flash access
void	cf_write (address, word);
void	cf_erase (address);
#endif

#endif	/* INFO_FLASH */

#endif
