/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "kernel.h"
#include "storage.h"
#include "storage_at45xxx.h"

// All this stuff is only needed for writing, quite a bit (for a micro).
// Still, writing is not going to be extremely efficient, mostly because
// we insist on using the simple (standard) set of API. For true flash,
// we may need a special provision for sequential writing with erase.

// Restriction: only one write operation can be pending at a time. Things
// will go berserk if the praxis starts a second write before the first one
// completes.

#define	WS_DONE		0
#define	WS_NEXT		1
#define	WS_GETPAGE	2
#define	WS_FIRST	3
#define	WS_PAGES	4
#define	WS_NPAGE	5
#define	WS_SYNC		6
#define	WS_LAST		7
#define	WS_GETLAST	8

#define	WS_CLOSED	BNONE

#define	EE_BiW(i)	((i) == 0 ? EE_B1W : EE_B2W)
#define	EE_BiR(i)	((i) == 0 ? EE_B1R : EE_B2R)
#define	EE_BiMMP(i)	((i) == 0 ? EE_B1MMP : EE_B2MMP)
#define	EE_BiMMPE(i)	((i) == 0 ? EE_B1MMPE : EE_B2MMPE)
#define	EE_MMPBiR(i)	((i) == 0 ? EE_MMPB1R : EE_MMPB2R)

typedef struct {
		word 	page;
		byte	dirty,
			wcmd;
} at45_bstat;

#if	EE_NO_ERASE_BEFORE_WRITE

// Assume that EEPROM is pre-erased, and writes never overlap
// ==========================================================

#define	EE_B1WCMD	EE_B1MMP
#define	EE_B2WCMD	EE_B2MMP

#else

// The default is to play it safe
// ==============================

#define	EE_B1WCMD	EE_B1MMPE
#define	EE_B2WCMD	EE_B2MMPE

#endif

static at45_bstat buf_stat [2] = {
					{ WNONE, 0, EE_B1WCMD },
					{ WNONE, 0, EE_B2WCMD }
				 };

static byte wstate = WS_CLOSED,	// Automaton state
	    cbsel = 0;		// Buffer toggle

static  word wpageo,		// page offset for write
	     wpagen,		// page number
	     wrsize;		// residual length

static 	const byte *wbuffp;	// write buffer pointer

#if	EE_USE_UART
// ===========================================================================
// SPI mode ==================================================================
// ===========================================================================

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

static Boolean busy () {

// Check if busy

	byte c;

	ee_start;
	put_byte (EE_STAT);
	c = get_byte ();
	ee_stop;

	return (c & 0x80) == 0;
}

static void saddr (word pn, word po) {
//
// Issue the address part of a command
//
	// Assumes EE_PADDR_BITS >= 8 and 24 bits complete address size, i.e.,
	// EE_PADDR_BITS + EE_POFFS_BITS == 24
	put_byte ((byte) (pn >> (EE_PADDR_BITS - 8)));
	put_byte ((byte) ((pn << (16 - EE_PADDR_BITS)) | (po >> 8)));
	put_byte ((byte) po);
}

static void dummies (word n) {

	while (n--)
		put_byte (0);
}

#else 	/* EE_USE_UART */

// ===========================================================================
// Direct mode ===============================================================
// ===========================================================================

static byte get_byte () {

	register int i;
	register byte b;

	for (b = 0, i = 0; i < 8; i++) {
		b <<= 1;
		ee_clkh;
		if (ee_inp)
			b |= 1;
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

static Boolean busy () {

// Check if busy

	byte c;

	ee_start;
	put_byte (EE_STAT);

	ee_clkh;
	c = ee_inp;
	ee_clkl;
	ee_stop;

	return (c == 0);
}

static void saddr (word a) {

// Send page address

	register int i;

	a <<= (16 - EE_PADDR_BITS);
	for (i = 0; i < EE_PADDR_BITS; i++) {
		if (a & 0x8000)
			ee_outh;
		else
			ee_outl;
		ee_clkh;
		ee_clkl;
		a <<= 1;
	}
}

static void soffs (word a) {

// Send page offset

	register int i;

	a <<= (16 - EE_POFFS_BITS);
	
	for (i = 0; i < EE_POFFS_BITS; i++) {
		if (a & 0x8000)
			ee_outh;
		else
			ee_outl;
		ee_clkh;
		ee_clkl;
		a <<= 1;
	}
}

static void sdc (byte nb) {

// Send nb zero bits

	ee_outl;
	while (nb--) {
		ee_clkh;
		ee_clkl;
	}
}

#define	dummies(n)	sdc ((byte)((n) << 3))

// ============================================================================
// ============================================================================
// ============================================================================

#endif 	/* EE_USE_UART */

static byte blook (word page) {

	if (buf_stat [0] . page == page)
		return 0;
	if (buf_stat [1] . page == page)
		return 1;
	return BNONE;
}

static void waitnb () {

// Wait while busy

	while (busy ())
		udelay (50);
}

word ee_read (lword a, byte *s, word len) {

	word pn, nb;
	word bi, po;

	if (wstate != WS_DONE || a >= EE_SIZE || (a + len) > EE_SIZE)
		return 1;

	if (len == 0)
		return 0;

	// Page offset
	po = (word)(a & (EE_PAGE_SIZE - 1));
	// Page number
	pn = (word)(a >> EE_PAGE_SHIFT);

	// No need to do this for a buffer read, but let us do it anyway once
	// upfront. Normally, the flash will be idle, so this is just a 
	// precaution in case somebody was writing to it a short while ago.
	waitnb ();

	while (1) {
		// Check if the page is present in one of the internal buffers
		if ((bi = blook (pn)) == BNONE) {
			ee_start;
			put_byte (EE_MMPR);
#if EE_USE_UART
			saddr (pn, po);
#else
			// Page number
			saddr (pn);
			// Page offset
			soffs (po);
#endif
			// Needed to initialize the transfer
			dummies (4);
		} else {
			ee_start;
			put_byte (EE_BiR (bi));
#if EE_USE_UART
			saddr (0, po);
#else
			sdc (EE_PADDR_BITS);
			// Page offset
			soffs (po);
#endif
			// 8 don't care bits
			dummies (1);
		}

		// Now we are going to extract the data - until the end of
		// page

		nb = (word) EE_PAGE_SIZE - po;
		if (nb > len)
			nb = len;

		while (nb--) {
			*s++ = get_byte ();
			len--;
		}
		ee_stop;

		if (len == 0)
			return 0;

		// more to read
		pn++;
		po = 0;
	}
}

static void wwait (word st) {

	if (st == WNONE) {
		waitnb ();
	} else if (busy ()) {
		delay (1, st);
		release;
	}
}

static byte bfree () {

	byte i, k;

	for (i = 0; i < 2; i++) {
		k = cbsel;
		cbsel = (byte) (1 - cbsel);
		if (buf_stat [k] . page == WNONE || !(buf_stat [k] . dirty))
			return k;
	}

	return BNONE;
}

static byte bflush () {

	byte k;

	k = cbsel;
	cbsel = (byte) (1 - cbsel);

	ee_start;

	put_byte (buf_stat [k] . wcmd);
#if EE_USE_UART
	saddr (buf_stat [k] . page, 0);
#else
	saddr (buf_stat [k] . page);
	sdc (EE_POFFS_BITS);
#endif
	ee_stop;

	buf_stat [k] . page = WNONE;
	return k;
}

static void bfetch (word pn, byte bi) {

	ee_start;

	put_byte (EE_MMPBiR(bi));
#if EE_USE_UART
	saddr (pn, 0);
#else
	saddr (pn);
	sdc (EE_POFFS_BITS);
#endif
	ee_stop;

	buf_stat [bi] . page = pn;
	buf_stat [bi] . dirty = 0;
}

static void bwrite (byte bi, word po, const char *buf, word nb) {

	ee_start;
	put_byte (EE_BiW (bi));

#if EE_USE_UART
	saddr (0, po);
#else
	sdc (EE_PADDR_BITS);
	soffs (po);
#endif

	if (buf != NULL) {
		while (nb--)
			put_byte (*buf++);
	} else {
		while (nb--)
			put_byte (0xFF);
	}

	buf_stat [bi] . dirty = YES;

	ee_stop;
}

static void sync (word st) {

	byte i;

	for (i = 0; i < 2; i++) {
		if (buf_stat [i] . page != WNONE) {
			if ((buf_stat [i] . dirty)) {
				wwait (st);
				ee_start;
				put_byte (buf_stat [i] . wcmd);
#if EE_USE_UART
				saddr (buf_stat [i] . page, 0);
#else
				saddr (buf_stat [i] . page);
				sdc (EE_POFFS_BITS);
#endif
				ee_stop;
				buf_stat [i] . dirty = NO;
			}
		}
	}

	wwait (st);
}

word ee_write (word st, lword a, const byte *s, word len) {

	word nb;
	byte bi;

	switch (wstate) {

	    case WS_DONE:

		if (a >= EE_SIZE || (a + len) > EE_SIZE)
			// return 1
			break;

		if (len == 0)
			// Basic sanity check
			return 0;

		// Start a new write
		wstate = WS_NEXT;
		wbuffp = s;
		wpagen = (word) (a >> EE_PAGE_SHIFT);
		wpageo = (word) (a & (EE_PAGE_SIZE - 1));
		wrsize = len;

		// Fall through

	    case WS_NEXT:

		while ((bi = blook (wpagen)) != BNONE) {
Found:
			wwait (st);
			nb = (word) EE_PAGE_SIZE - wpageo;
			if (nb > wrsize)
				nb = wrsize;

			// Write to the block
			bwrite (bi, wpageo, wbuffp, nb);

			wrsize -= nb;
			if (wrsize == 0) {
				wstate = WS_DONE;
				return 0;
			}

			// More, we are necessarily at a page boundary
			wbuffp += nb;
			wpageo = 0;
			wpagen++;
		}

		// Have to fetch the page
		wstate = WS_GETPAGE;
		// Fall through

	    case WS_GETPAGE:

		if ((bi = bfree ()) == BNONE) {
			// We are about to start a buffer transfer;
			// make sure we are idle
			wwait (st);
			// Select and flush the victim
			bi = bflush ();
			// This makes us busy
		}

		// Fetch the page
		wwait (st);
		bfetch (wpagen, bi);
		wstate = WS_NEXT;
		goto Found;
	}

	return 1;
}

word ee_erase (word st, lword from, lword upto) {

	word nb;
	byte bi;

	if (from >= EE_SIZE)
		goto ERet;

	if (upto >= EE_SIZE || upto == 0)
		upto = EE_SIZE - 1;
	else if (upto < from)
		goto ERet;

	// Make it LWA+1
	upto++;

	switch (wstate) {

	    case WS_DONE:

		wpagen = (word) (from >> EE_PAGE_SHIFT);
		wpageo = (word) (from & (EE_PAGE_SIZE - 1));

		if (wpageo) {
			// Partial first page
			wstate = WS_FIRST;
			goto WS_first;
		}
WS_pcheck:
		// From is at a page boundary
		wrsize = (word)((upto - from) >> EE_PAGE_SHIFT);
		// The number of complete pages
		if (wrsize == 0)
			goto WS_left;

		// A number of full pages
		wstate = WS_PAGES;

	    case WS_PAGES:

		// Flush wrsize entire pages from wpagen 
		sync (st);
		buf_stat [0] . page = buf_stat [1] . page = WNONE;

		// We get here when we have been synced, i.e., the buffers
		// are clean; note that sync is stateless and idempotent

		// bwrite (0, 0, NULL, EE_PAGE_SIZE);
		wstate = WS_NPAGE;

	    case WS_NPAGE:

		while (wrsize) {
			wwait (st);
			ee_start;
			put_byte (EE_ERASE);
#if EE_USE_UART
			saddr (wpagen, 0);
#else
			saddr (wpagen);
			sdc (EE_POFFS_BITS);
#endif
			ee_stop;
			wpagen++;
			wrsize--;
		}

		// Take care of the leftover
WS_left:
		wpageo = upto & (EE_PAGE_SIZE - 1);
		if (wpageo == 0) {
			// No bytes on the last page
			wstate = WS_SYNC;
			// Done: make sure we are in a decent clean state
			goto WS_sync;
		}

		wstate = WS_LAST;

	    case WS_LAST:
WS_last:
		if ((bi = blook (wpagen)) != BNONE) {
			wwait (st);
			bwrite (bi, 0, NULL, wpageo);
			wstate = WS_SYNC;
			goto WS_sync;
		}

		wstate = WS_GETLAST;

		// Fall through

	    case WS_GETLAST:

		if ((bi = bfree ()) == BNONE) {
			wwait (st);
			bi = bflush ();
		}

		// Fetch the page
		wwait (st);
		bfetch (wpagen, bi);
		wstate = WS_LAST;
		goto WS_last;

	    case WS_SYNC:
WS_sync:
		sync (st);
		wstate = WS_DONE;
		return 0;

	    case WS_FIRST:
WS_first:
		// Write a single portion of the first page
		if ((bi = blook (wpagen)) != BNONE) {
			wwait (st);
			nb = (word) EE_PAGE_SIZE - wpageo;
			if (nb > upto - from)
				nb = (word) (upto - from);

			// Write to the block
			bwrite (bi, wpageo, NULL, nb);

			from += nb;
			if (from >= upto) {
				// We are done
				wstate = WS_SYNC;
				goto WS_sync;
			}

			// Done with the first page
			wpagen++;
			// We are on a page boundary now, note that from is
			// set
			goto WS_pcheck;
		}

		wstate = WS_GETPAGE;

		// Fall through

	    case WS_GETPAGE:

		if ((bi = bfree ()) == BNONE) {
			wwait (st);
			bi = bflush ();
		}

		// Fetch the page
		wwait (st);
		bfetch (wpagen, bi);
		wstate = WS_FIRST;
		goto WS_first;
	}
ERet:
	return 1;
}

word ee_sync (word st) { 

	if (wstate == WS_DONE)
		sync (st);
	return 0;
}

word ee_open () {
//
// Open the EEPROM
//
	word cnt;

	if (wstate != WS_CLOSED)
		return 0;

	ee_bring_up;

#ifdef	EEPROM_PDMODE_AVAILABLE
	ee_start;
	put_byte (EE_PUP);
	ee_stop;
#endif
	for (cnt = 1000; busy () && cnt; cnt--);

	if (cnt == 0) {
		ee_bring_down;
		return 1;
	}

	wstate = WS_DONE;
	return 0;
}

void ee_close () {

	word cnt;

	if (wstate == WS_CLOSED)
		return;

	sync (WNONE);

	for (cnt = 1000; busy () && cnt; cnt--);

#ifdef	EEPROM_PDMODE_AVAILABLE
	ee_start;
	put_byte (EE_PDN);
	ee_stop;
#endif
	ee_bring_down;
	wstate = WS_CLOSED;
}

#include "storage_eeprom.h"
