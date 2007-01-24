#ifndef	__pg_storage_m95xxx_h
#define	__pg_mstorage_95xxx_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "kernel.h"
#include "storage_sys.h"
#include "pins.h"

#define	EE_WREN		0x06		// Write enable
#define	EE_WRDI		0x04		// Write disable
#define	EE_RDSR		0x05		// Read status register
#define	EE_WRSR		0x01		// Write status register
#define	EE_READ		0x03
#define	EE_WRITE	0x02

#define	STAT_WIP	0x01		// Write in progress
#define	STAT_INI	0x00

#define	EE_PAGE_SIZE	32	// bytes
#define	EE_SIZE		8192	// 8 K

#define	EE_PAGES	(EE_SIZE / EE_PAGE_SIZE)

#endif
