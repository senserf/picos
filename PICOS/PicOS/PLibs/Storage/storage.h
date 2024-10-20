/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_storage_h
#define	__pg_storage_h	1

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

void	ee_init_erase ();

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

void	sd_init_erase ();

#endif	/* SDCARD_PRESENT */

#if	INFO_FLASH

#include "iflash.h"

// Operations on the internal "information" flash (also dubbed FIM)
int	if_write (word, word);
void	if_erase (int);

#define	IFLASH		IFLASH_START
#define	if_read(a)	(IFLASH [a])

#if	INFO_FLASH > 1
// Code flash access
void	cf_write (address, word);
void	cf_erase (address);
#endif

#endif	/* INFO_FLASH */

#endif
