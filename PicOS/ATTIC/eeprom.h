#ifndef	__pg_eeprom_h
#define	__pg_eeprom_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "kernel.h"
#include "eeprom_sys.h"
#include "cc1100_sys.h"

#define	EE_WREN		0x06		// Write enable
#define	EE_WRDI		0x04		// Write disable
#define	EE_RDSR		0x05		// Read status register
#define	EE_WRSR		0x01		// Write status register
#define	EE_READ		0x03
#define	EE_WRITE	0x02

#define	STAT_WIP	0x01		// Write in progress
#define	STAT_INI	0x00

#define	EE_PAGES	(EE_SIZE / EE_PAGE_SIZE)

#if DIAG_MESSAGES > 2
// The number of EEPROM pages reserved for the log
#define	EE_DEBUG_PAGES	((DIAG_MESSAGES+EE_PAGE_SIZE-1)/EE_PAGE_SIZE)

#define	mkmk_eval	0
#if EE_DEBUG_PAGES > EE_PAGES
#error "DIAG_MESSAGES too large"
#endif
#undef	mkmk_eval

#else
#define	EE_DEBUG_PAGES	0
#endif	/* DIAG_MESSAGES */

#define	EE_APP_PAGES	(EE_PAGES - EE_DEBUG_PAGES)

#define	EE_APP_SIZE	(EE_APP_PAGES * EE_PAGE_SIZE)
#define	EE_DBG_START	EE_APP_SIZE

#if 0
#define	EEPROM_RAW_ERASE \
			for (zz_mintk = 0; zz_mintk < EE_PAGES; zz_mintk++) { \
				ee_read (zz_mintk * EE_PAGE_SIZE, \
				    (byte*)__PCB, EE_PAGE_SIZE); \
				for (zz_lostk = 0; zz_lostk < EE_PAGE_SIZE; \
				     zz_lostk++) \
					if (((byte*)__PCB) [zz_lostk] != 0xff) \
						break; \
				if (zz_lostk >= EE_PAGE_SIZE) \
					continue; \
				for (zz_lostk = 0; zz_lostk < EE_PAGE_SIZE; \
				    zz_lostk++) \
					((byte*)__PCB) [zz_lostk] = 0xff; \
				ee_write (zz_mintk * EE_PAGE_SIZE, \
				    (byte*)__PCB, EE_PAGE_SIZE); \
			}
#endif
#endif
