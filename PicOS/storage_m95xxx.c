/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "kernel.h"
#include "storage.h"
#include "storage_m95xxx.h"

#if EE_USE_UART

static byte get_byte () {
	// Send a dummy byte of zeros; we are the master so we have to
	// keep the clock ticking
	ee_put (0);
	while (!ee_tx_ready);
	while (!ee_rx_ready);
	return ee_get;
}

static void put_byte (byte b) {
	byte s;
	ee_put (b);
	while (!ee_tx_ready);
	while (!ee_rx_ready);
	s = ee_get;
}

#else	/* EE_USE_UART */

static byte get_byte () {

	register int i;
	register byte b;

	for (b = 0, i = 0; i < 8; i++) {
		b <<= 1;
		if (ee_inp)
			b |= 1;
		ee_clkh;
		ee_clkl;
	}

	return b;
}

static void put_byte (byte b) {

	register int i;

	for (i = 0; i < 8; i++) {
		if (b & 0x80)
			ee_outh;
		else
			ee_outl;
		ee_clkh;
		ee_clkl;
		b <<= 1;
	}
}

#endif

word ee_open () {

	ee_bring_up;
	ee_start;
	put_byte (EE_WRSR);
	put_byte (STAT_INI);
	ee_stop;
	udelay (10);
	return 0;
}

void ee_close () {

	ee_bring_down;
}

word ee_read (lword a, byte *s, word len) {

	byte c;

	if (len == 0)
		return 0;

	if (a >= EE_SIZE || (a + len) > EE_SIZE)
		return 1;

	ee_start;
	put_byte (EE_RDSR);
	do {
		c = get_byte ();
	} while (c & STAT_WIP);
	ee_stop;

	a &= (EE_SIZE - 1);

	ee_start;
	put_byte (EE_READ);
	put_byte ((byte)((a) >> 8));
	put_byte ((byte)((a)     ));

	while (len--)
		*s++ = get_byte ();
	ee_stop;
	return 0;
}

word ee_erase (word st, lword from, lword upto) {

	word a, b;
	byte c, d;
	byte cnt;

	// Note: the state argument is ignored in this version. Erase blocks
	// the CPU.

	if (from >= EE_SIZE)
		return 1;

	if (upto >= EE_SIZE || upto == 0)
		upto = EE_SIZE - 1;
	else if (upto < from)
		return 1;

	a = (word)from;
	b = (word)upto;

	while (a <= b) {

		// how many bytes on this page
		cnt = EE_PAGE_SIZE - (a & (EE_PAGE_SIZE - 1));
		if (a + cnt > b)
			cnt = b + 1 - a;

		ee_start;
		put_byte (EE_RDSR);
		do {
			c = get_byte ();
		} while (c & STAT_WIP);
		ee_stop;
		
		ee_start;
		put_byte (EE_READ);
		put_byte ((byte)((a) >> 8));
		put_byte ((byte)((a)     ));

		d = cnt;
		while (d--) {
			if (get_byte () != 0xff)
				break;
		}
		ee_stop;

		if (d) {
			// We have to erase this page
			ee_start;
			put_byte (EE_RDSR);
			do {
				c = get_byte ();
			} while (c & STAT_WIP);
			ee_stop;

			// Set WEL
			ee_start;
			put_byte (EE_WREN);
			ee_stop;

			ee_start;
			put_byte (EE_WRITE);
			put_byte ((byte)((a) >> 8));
			put_byte ((byte)((a)     ));

			d = cnt;
			while (d--)
				put_byte (0xff);
			ee_stop;
		}

		a += cnt;
	}
	return 0;
}

word ee_write (word st, lword a, const byte *s, word len) {

	byte c, ne;

	if (a >= EE_SIZE || (a + len) > EE_SIZE)
		return 1;

	while (len) {
		// How far to the end of page
		ne = EE_PAGE_SIZE - (a & (EE_PAGE_SIZE - 1));
		if (ne > len)
			ne = len;

		ee_start;
		put_byte (EE_RDSR);
		do {
			// Wait for the end of previous write
			c = get_byte ();
		} while (c & STAT_WIP);
		ee_stop;

		// Set WEL
		ee_start;
		put_byte (EE_WREN);
		ee_stop;

		ee_start;
		put_byte (EE_WRITE);
		put_byte ((byte)((a) >> 8));
		put_byte ((byte)((a)     ));

		len -= ne;
		a += ne;

		while (ne--)
			put_byte (*s++);
		ee_stop;

	}
	return 0;
}

word ee_sync (word st) { 
// This one is void
	return 0;
}

void ee_panic () { }

#include "storage_eeprom.h"
