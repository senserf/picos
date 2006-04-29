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
	if (j != IF_PAGE_SIZE) {
		cli;
		if_erase_sys ((int)(&(IFLASH [i])));
		sti;
	}
}

int if_write (word a, word w) {
/*
 * Write word w at location a (0-IFLASH_SIZE-1)
 */
	if (a >= IFLASH_SIZE)
		syserror (EFLASH, "if_write");

	// Check if the word is writeable
	if (IFLASH [a] != 0xffff)
		return ERROR;
	cli;
	if_write_sys (w, a);
	sti;

	return 0;
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

#if TARGET_BOARD == BOARD_VERSA2
	int i;

	if (VERSA2_RESET_KEY_PRESSED) {
		for (i = 0; i < 4; i++) {
			leds (0, 1); leds (2, 1); leds (3, 1);
			mdelay (256);
			leds (0, 0); leds (2, 0); leds (3, 0);
			mdelay (256);
		}
		if_erase (-1);
		while (VERSA2_RESET_KEY_PRESSED);
	}
#endif
}
