/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "sysio.h"
#include "storage.h"

static void if_erase_block (address a, word bsize) {

	// Check if the block needs to be erased
	while (bsize--) {
		if (a [bsize] != 0xffff) {
			if_erase_sys ((int)a);
			return;
		}
	}
}

int if_write (word a, word w) {
/*
 * Write word w at location a (0-IFLASH_SIZE-1)
 */
	if (a >= IFLASH_SIZE)
		syserror (EFLASH, "if_write");
#if 0
	// Check if the word is writeable
	if (IFLASH [a] != 0xffff)
		return ERROR;
#endif
	__FLASH_FIM_UNLOCK__;
	if_write_sys (w, IFLASH + a);
	__FLASH_FIM_RELOCK__;

	return (IFLASH [a] == w) ? 0 : ERROR;
#if 0
	if (IFLASH [a] != w) {
		dbg_1 (a); dbg_1 (IFLASH [a]); dbg_1 (w);
		return ERROR;
	}

	return 0;
#endif
}

void if_erase (int a) {
/*
 * Erase info flash block containing the word with the specified address
 */
	__FLASH_FIM_UNLOCK__;
	if (a >= 0) {
		// Single block
		if (a >= IFLASH_SIZE)
			syserror (EFLASH, "if_erase");
		a &= ~(IFLASH_PAGESIZE - 1);
		if_erase_block (IFLASH + a, IFLASH_PAGESIZE);
	} else {
		for (a = 0; a < IFLASH_SIZE; a += IFLASH_PAGESIZE)
			if_erase_block (IFLASH + a, IFLASH_PAGESIZE);
	}
	__FLASH_FIM_RELOCK__;
}

#if INFO_FLASH > 1

// General access to code flash

void cf_write (address a, word w) { if_write_sys (w, a); }

void cf_erase (address a) {

	if_erase_block ((address)((word)a & ~((CFLASH_PAGESIZE << 1) - 1)),
		CFLASH_PAGESIZE);
}

#endif	/* INFO_FLASH > 1 */
