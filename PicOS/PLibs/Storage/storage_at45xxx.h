/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_storage_at45xxx_h
#define	__pg_storage_at45xxx_h	1

#include "kernel.h"
#include "storage.h"
#include "pins.h"

#ifndef	STORAGE_AT45_TYPE
#define	STORAGE_AT45_TYPE	41	// This is what we started from
#endif

// ============================================================================
#if STORAGE_AT45_TYPE == 41

#define	EE_MMPR		0x52		// Main memory page read (direct)
#define	EE_B1R		0x54		// Buffer 1 read
#define	EE_B2R		0x56		// Buffer 2 read
#define	EE_STAT		0x57		// Status

#define	EE_PADDR_BITS	15		// Page address bits
#define	EE_POFFS_BITS	9		// Including extra zero bit

#define	EE_PAGE_SIZE	256			// bytes used by the praxis
#define	EE_PAGE_SIZE_T	(EE_PAGE_SIZE + 8)	// including the extra 8 bytes
#define	EE_PAGE_SHIFT	8

#define	EE_NBLOCKS	2048

#endif	/* 41 */

// ============================================================================
#if STORAGE_AT45_TYPE >= 410
//
// This is the 'D' variant of 41
//
#define	EE_MMPR		0xD2		// Main memory page read (direct)

// Use lower frequency reads (it is still up to 33 MHz, i.e., more than we
// can handle), while giving us a slightly better power budget

#define	EE_B1R		0xD4		// Buffer 1 read
#define	EE_B2R		0xD6		// Buffer 2 read

#define	EE_STAT		0xD7		// Status

#define	EE_PADDR_BITS	15		// Page address bits
#define	EE_POFFS_BITS	9		// Including extra zero bit

#define	EE_PAGE_SIZE	256			// bytes used by the praxis
#define	EE_PAGE_SIZE_T	(EE_PAGE_SIZE + 8)	// including the extra 8 bytes
#define	EE_PAGE_SHIFT	8

#define	EE_NBLOCKS	2048

#if STORAGE_AT45_TYPE == 413
// This is the "E" version with ultra-super-duper-deep power-down mode
#define	EE_PDN		0x79		// Enter ultra-deep PD mode
#else
#define	EE_PDN		0xB9		// Enter PD mode
#endif

#define	EE_PUP		0xAB		// Return to standby

#endif	/* 41 D */

// ============================================================================
#if STORAGE_AT45_TYPE == 321

#define	EE_MMPR		0xD2		// Main memory page read (direct)
#define	EE_B1R		0xD4		// Buffer 1 read
#define	EE_B2R		0xD6		// Buffer 2 read
#define	EE_STAT		0xD7		// Status

#define	EE_PADDR_BITS	14		// Page address bits
#define	EE_POFFS_BITS	10		// Bits in offset, including one dummy

#define	EE_PAGE_SIZE	512			// bytes used by the praxis
#define	EE_PAGE_SIZE_T	(EE_PAGE_SIZE + 16)	// including the extra 8 bytes
#define	EE_PAGE_SHIFT	9

#define	EE_NBLOCKS	8192

#endif	/* 321 */

#ifndef	EE_NBLOCKS
#error "S: illegal STORAGE_AT45_TYPE, not supported"
#endif

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
#define	EE_ERASE	0x81		// Page erase


#define	EE_BLOCK_SIZE	EE_PAGE_SIZE
#define	EE_NPAGES	EE_NBLOCKS

#define	EE_SIZE		(((lword)EE_NPAGES) * EE_PAGE_SIZE)

#ifndef	EE_NO_ERASE_BEFORE_WRITE
#define	EE_NO_ERASE_BEFORE_WRITE	0
#endif

#define	EE_ERASE_UNIT		1
// I know this is confusing; this constant indicates whether the storage MUST
// be erased by hand before write, whereas the previous one tells whether the
// driver should force erasure when flushing a block
#define	EE_ERASE_BEFORE_WRITE	EE_NO_ERASE_BEFORE_WRITE
#define	EE_RANDOM_WRITE		1

#endif
