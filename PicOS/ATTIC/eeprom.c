/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "eeprom.h"

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

void zz_ee_init () {

	int i;

	ee_ini_regs;
	ee_ini_spi;

	ee_start;
	put_byte (EE_WRSR);
	put_byte (STAT_INI);
	ee_stop;
	udelay (10);
}

static void start_cmd (byte cmd, address a) {

	byte s;

	ee_start;
	put_byte (EE_RDSR);
	do {
		s = get_byte ();
	} while (s & STAT_WIP);
	ee_stop;

	if (cmd == EE_WRITE) {
		// Set WEL
		ee_start;
		put_byte (EE_WREN);
		ee_stop;
	}

	ee_start;
	put_byte (cmd);
	put_byte ((byte)(((word)(a)) >> 8));
	put_byte ((byte)(((word)(a))     ));
}

word ee_readw (address a) {

	word w;

	start_cmd (EE_READ, a);
	// We go little endian
	w =   get_byte ();
	w |= (get_byte () << 8);
	ee_stop;
	return w;
}

lword ee_readl (address a) {

	lword lw;

	start_cmd (EE_READ, a);
	lw  = ((lword)get_byte ()      );
	lw |= ((lword)get_byte () <<  8);
	lw |= ((lword)get_byte () << 16);
	lw |= ((lword)get_byte () << 24);
	ee_stop;
	return lw;
}

void ee_reads (address a, byte *s, word len) {

	start_cmd (EE_READ, a);
	while (len--)
		*s++ = get_byte ();
	ee_stop;
}

void ee_writew (address a, word w) {

	start_cmd (EE_WRITE, a);
	put_byte ((byte)(w     ));
	put_byte ((byte)(w >> 8));
	ee_stop;
}

void ee_writel (address a, lword lw) {

	start_cmd (EE_WRITE, a);
	put_byte ((byte)(lw      ));
	put_byte ((byte)(lw >>  8));
	put_byte ((byte)(lw >> 16));
	put_byte ((byte)(lw >> 24));
	ee_stop;
}

void ee_writes (address a, const byte *s, word len) {

	start_cmd (EE_WRITE, a);
	while (len--)
		put_byte (*s++);
	ee_stop;
}
