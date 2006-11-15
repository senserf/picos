/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "iflash.h"

static void if_erase_block (int i) {

	int j;

	// Check if the block needs to be erased
	for (j = 0; j < IF_PAGE_SIZE; j++)
		if (IFLASH [i + j] != 0xffff)
				break;
	if (j != IF_PAGE_SIZE)
		if_erase_sys ((int)(&(IFLASH [i])));
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
	if_write_sys (w, a);

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
	if (a >= 0) {
		// Single block
		if (a >= IFLASH_SIZE)
			syserror (EFLASH, "if_erase");
		a &= ~(IF_PAGE_SIZE - 1);
		if_erase_block (a);
		return;
	}

	for (a = 0; a < IFLASH_SIZE; a += IF_PAGE_SIZE)
		if_erase_block (a);
}

void zz_if_init () {

	if_start_up;
}
