#ifndef	__pg_storage_at45xxx_h
#define	__pg_storage_at45xxx_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "kernel.h"
#include "storage.h"
#include "pins.h"

#define	EE_MMPR		0x52		// Main memory page read (direct)
#define	EE_B1R		0x54		// Buffer 1 read
#define	EE_B2R		0x56		// Buffer 2 read
#define	EE_MMPB1R	0x53		// Main memory page read to buffer 1
#define	EE_MMPB2R	0x55		// Main memory page read to buffer 2
#define	EE_MMPB1C	0x60		// Main memory page compare buffer 1
#define	EE_MMPB2C	0x61		// Main memory page compare buffer 2
#define	EE_B1W		0x84		// Buffer 1 write
#define	EE_B2W		0x87		// Buffer 2 write
#define	EE_B1MMPE	0x83		// Buffer 1 to MM program with erase
#define	EE_B2MMPE	0x86		// Buffer 2 to MM program with erase
#define	EE_B1MMP	0x88		// Buffer 1 to MM program no erase
#define	EE_B2MMP	0x89		// Buffer 2 to MM program no erase
#define	EE_MMPB1	0x82		// MM program through buffer 1
#define	EE_MMPB2	0x85		// MM program through buffer 2
#define	EE_APRB1	0x58		// Auto page rewrite through buffer 1
#define	EE_APRB2	0x59		// Auto page rewrite through buffer 2
#define	EE_STAT		0x57		// Status
#define	EE_ERASE	0x81		// Page erase

#define	EE_PAGE_SIZE	256		// bytes used by the praxis
#define	EE_PAGE_SIZE_T	(EE_PAGE_SIZE + 8)	// including the extra 8 bytes
#define	EE_PAGE_SHIFT	8
#define	EE_BLOCK_SIZE	EE_PAGE_SIZE
#define	EE_NPAGES	EE_NBLOCKS
#define	EE_SIZE		(((lword)EE_NPAGES) * EE_PAGE_SIZE)

#define	EE_ERASE_UNIT		1
#define	EE_ERASE_BEFORE_WRITE	0
#define	EE_RANDOM_WRITE		1



#endif
