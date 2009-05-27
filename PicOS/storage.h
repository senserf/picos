#ifndef	__pg_storage_h
#define	__pg_storage_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "pins.h"

#ifdef	EEPROM_PRESENT

#include "board_storage.h"

word 	ee_read  (lword, byte*, word);
word 	ee_write (word, lword, const byte*, word);
word	ee_erase (word, lword, lword);
word	ee_sync (word);
lword	ee_size (Boolean*, lword*);
word	ee_open ();
void	ee_close ();
void	ee_panic ();

#ifdef	RESET_ON_KEY_PRESSED
void	ee_init_erase ();
#endif

#endif	/* EEPROM_PRESENT */

#ifdef	SDCARD_PRESENT

#include "board_storage.h"

word	sd_open ();
word	sd_read (lword, byte*, word);
word	sd_write (lword, const byte*, word);
word	sd_erase (lword, lword);
word	sd_sync ();
void	sd_close ();
void	sd_panic ();
lword	sd_size ();
void	sd_idle ();

#ifdef	RESET_ON_KEY_PRESSED
void	sd_init_erase ();
#endif

#endif	/* SDCARD_PRESENT */

#if	INFO_FLASH

#include "iflash.h"

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
