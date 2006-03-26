/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "iflash.h"

void if_write (word a, word w) {
/*
 * Write word w at location a (0-IFLASH_SIZE-1)
 */
	if (a >= IFLASH_SIZE)
		syserror (EFLASH, "if_write");

	cli;
	if_write_sys (w, a);
	sti;
}

void if_erase () {
/*
 * Erase info flash block
 */
	int i, j;

	for (i = 0; i < IFLASH_SIZE; i += IF_PAGE_SIZE) {
		// Check if the block needs to be erased
		for (j = 0; j < IF_PAGE_SIZE; j++)
			if (IFLASH [i + j] != 0xffff)
				break;
		if (j != IF_PAGE_SIZE) {
			cli;
			if_erase_sys ((int)(&(IFLASH [i])));
			sti;
		}
	}
}
