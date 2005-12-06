#ifndef	__pg_eeprom_h
#define	__pg_eeprom_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "eeprom_sys.h"

#define	EE_WREN		0x06		// Write enable
#define	EE_WRDI		0x04		// Write disable
#define	EE_RDSR		0x05		// Read status register
#define	EE_WRSR		0x01		// Write status register
#define	EE_READ		0x03
#define	EE_WRITE	0x02

#define	STAT_WIP	0x01		// Write in progress
#define	STAT_INI	0x00

#endif
