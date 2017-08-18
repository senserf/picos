#ifndef	__pg_storage_mt29xxx_h
#define	__pg_storage_mt29xxx_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// EE_NBLOCKS defined in board_storage.h

#define	EE_PAGE_SIZE		2048			// used by the praxis
#define	EE_PAGE_SIZE_T		(EE_PAGE_SIZE + 64)	// + extra 64 bytes
#define	EE_PAGE_SHIFT		11

#define	EE_PAGES_PER_BLOCK	64
#define	EE_BLOCK_SIZE		((lword)EE_PAGE_SIZE * EE_PAGES_PER_BLOCK)
#define	EE_BLOCK_SHIFT		(EE_PAGE_SHIFT + 6)

#define	EE_NPAGES		((lword)EE_NBLOCKS * EE_PAGES_PER_BLOCK)
#define	EE_SIZE			(EE_NPAGES * EE_PAGE_SIZE)

#define	EE_ERASE_UNIT		EE_BLOCK_SIZE
#define	EE_ERASE_BEFORE_WRITE	1
#define	EE_RANDOM_WRITE		0

#define	EE_CMD_PR0		0x00		// Page read stage 0
#define	EE_CMD_PR1		0x30		// Page read stage 1
#define	EE_CMD_RR0		0x05		// Random read stage 0
#define	EE_CMD_RR1		0xE0		// Random read stage 1
#define	EE_CMD_PPG		0x80		// Page program
#define	EE_CMD_PWR		0x85		// Page write
#define EE_CMD_FLU		0x10		// Page flush
#define	EE_CMD_BE0		0x60		// Block erase stage 0
#define	EE_CMD_BE1		0xD0		// Block erase stage 1
#define	EE_CMD_STA		0x70		// Read status
#define	EE_CMD_RST		0xFF		// Reset

#endif
