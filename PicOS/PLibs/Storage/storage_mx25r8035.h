#ifndef	__pg_storage_mx25r8035_h
#define	__pg_storage_mx25r8035_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2017                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "kernel.h"
#include "storage.h"
#include "pins.h"

// Commands
#define	CMD_READ		0x03	// Normal read
#define	CMD_FREAD		0x0b	// Fast read
#define	CMD_2READ		0xbb	// 2 x read
#define	CMD_DREAD		0x3b	// Dual read
#define	CMD_4READ		0xeb	// 4 x read
#define	CMD_QREAD		0x6b	// Quad read

#define	CMD_PP			0x02	// Page program
#define	CMD_4PP			0x38	// Quad page program
#define	CMD_SE			0x20	// Sector erase
#define	CMD_BE32		0x52	// Block erase 32K
#define	CMD_BE64		0xd8	// Block erase 64K
#define	CMD_CE			0x60	// Chip erase

#define	CMD_RDSFDP		0x5a	// Read SFDP
#define	CMD_WREN		0x06	// Write enable
#define	CMD_WRDI		0x04	// Write disable
#define	CMD_RDSR		0x05	// Read status register
#define	CMD_RDCR		0x15	// Read configuration register
#define	CMD_WRSR		0x01	// Write status register

#define	CMD_PERS		0x75	// Suspend program/erase
#define	CMD_PERR		0x7a	// Resume program/erase
#define	CMD_DP			0xb9	// Deep power down
#define	CMD_SBL			0xc0	// Set burst length

#define	CMD_RDID		0x9f	// Read ID
#define	CMD_RES			0xab	// Read electroni ID (obsolete)
#define	CMD_REMS		0x90	// Read manufacturer & device ID
#define	CMD_ENSO		0xb1	// Enter secured OTP
#define	CMD_EXSO		0xc1	// Exit secured OTP
#define	CMD_RDSCUR		0x2b	// Read security register
#define	CMD_WRSCUR		0x2f	// Write security register

#define	CMD_NOP			0x00
#define	CMD_RSTEN		0x66	// Reset enable
#define	CMD_RST			0x99	// Reset
#define	CMD_RRE			0xff	// Release read enhanced

// ===========================================================================

// Use WREN before every program/erase
// PP programs a previously-erased page exactly 256 bytes long
#define	EE_PAGE_SIZE		256

// Use sectors as official blocks (the minimum erasure unit)
#define	EE_BLOCK_SIZE		4096

// Size
#define	EE_SIZE			(1024 * 1024)
#define	EE_NBLOCKS		(EE_SIZE / EE_BLOCK_SIZE)

#ifndef	EE_NO_ERASE_BEFORE_WRITE
#define	EE_NO_ERASE_BEFORE_WRITE	1
#endif

#define	EE_ERASE_UNIT		EE_BLOCK_SIZE
// I know this is confusing; this constant indicates whether the storage MUST
// be erased by hand before write, whereas the previous one tells whether the
// driver should force erasure when flushing a block
#define	EE_ERASE_BEFORE_WRITE	EE_NO_ERASE_BEFORE_WRITE
#define	EE_RANDOM_WRITE		0

#endif
