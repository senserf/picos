/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
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

static	word buf_page [2];	// page in buffer
static	byte buf_flags [2];	// buffer state

static	byte wstate,		// current state of the write operation
	     cbsel;		// Alternating buffer selector

static  word wpageo,		// page offset for write
	     wpagen,		// page number
	     wrsize;		// residual length

static 	const byte *wbuffp;	// write buffer pointer

#define	EE_BiW(i)	((i) == 0 ? EE_B1W : EE_B2W)
#define	EE_BiR(i)	((i) == 0 ? EE_B1R : EE_B2R)
#define	EE_BiMMP(i)	((i) == 0 ? EE_B1MMP : EE_B2MMP)
#define	EE_BiMMPE(i)	((i) == 0 ? EE_B1MMPE : EE_B2MMPE)
#define	EE_MMPBiR(i)	((i) == 0 ? EE_MMPB1R : EE_MMPB2R)

#define	BF_DIRTY	0x01
#define	BF_ERASED	0x02

#define	WS_DONE		0
#define	WS_NEXT		1
#define	WS_GETPAGE	2
#define	WS_FIRST	3
#define	WS_PAGES	4
#define	WS_NPAGE	5
#define	WS_SYNC		6
#define	WS_LAST		7
#define	WS_GETLAST	8

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

static byte blook (word page) {

	if (buf_page [0] == page)
		return 0;
	if (buf_page [1] == page)
		return 1;
	return BNONE;
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

static void waitnb () {

// Wait while busy

	byte st;

	while (1) {
		ee_start;
		put_byte (EE_STAT);
		ee_clkh;
		st = ee_inp;
		ee_clkl;
		ee_stop;
		if (st)
			return;
		udelay (100);
	}
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

void zz_ee_init () {

	ee_ini_regs;

	buf_page [0] = buf_page [1] = WNONE;
	buf_flags [0] = buf_flags [1] = 0;
	wstate = WS_DONE;
	cbsel = 0;

	ee_postinit;
}

word ee_read (lword a, byte *s, word len) {

	word pn, nb;
	word bi, po;

	if (len == 0)
		return 0;

	if (a >= EE_SIZE || (a + len) > EE_SIZE)
		return 1;

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
			// Page number
			saddr (pn);
			// Page offset
			soffs (po);
			// Needed to initialize the transfer
			sdc (32);
		} else {
			ee_start;
			put_byte (EE_BiR (bi));
			sdc (EE_PADDR_BITS);
			// Page offset
			soffs (po);
			// 8 don't care bits
			sdc (8);
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
		delay (2, st);
		release;
	}
}

static byte bfree () {

	byte i, k;

	for (i = 0; i < 2; i++) {
		k = cbsel;
		cbsel = (byte) (1 - cbsel);
		if (buf_page [k] == WNONE) {
			return k;
		}
	}

	for (i = 0; i < 2; i++) {
		k = cbsel;
		cbsel = (byte) (1 - cbsel);
		if ((buf_flags [k] & BF_DIRTY) == 0) {
			buf_page [k] = WNONE;
			return k;
		}
	}

	return BNONE;
}

static byte bflush () {

	byte i, k;

	for (i = 0; i < 2; i++) {
		k = cbsel;
		cbsel = (byte) (1 - cbsel);
		if ((buf_flags [k] & BF_ERASED))
			break;
	}

	ee_start;
	put_byte (i < 2 ? EE_BiMMP (k) : EE_BiMMPE (k));
	saddr (buf_page [k]);
	sdc (EE_POFFS_BITS);
	ee_stop;

	buf_page [k] = WNONE;
	buf_flags [k] = 0;

	return k;
}

static void bfetch (word pn, byte bi) {

	ee_start;
	put_byte (EE_MMPBiR(bi));
	saddr (pn);
	sdc (EE_POFFS_BITS);
	ee_stop;

	buf_page [bi] = pn;
	buf_flags [bi] = 0;
	// FIXME: no erased flag is available yet
}

static void bwrite (byte bi, word po, const char *buf, word nb) {

	ee_start;
	put_byte (EE_BiW (bi));

	sdc (EE_PADDR_BITS);
	soffs (po);

	if (buf != NULL) {
		while (nb--)
			put_byte (*buf++);
	} else {
		while (nb--)
			put_byte (0xFF);
	}

	buf_flags [bi] |= BF_DIRTY;

	ee_stop;
}

static void sync (word st) {

	byte i;

	for (i = 0; i < 2; i++) {
		if (buf_page [i] != WNONE) {
			if ((buf_flags [i] & BF_DIRTY)) {
				wwait (st);
				ee_start;
				put_byte ((buf_flags [i] & BF_ERASED) ? 
					EE_BiMMP (i) : EE_BiMMPE (i));
				saddr (buf_page [i]);
				sdc (EE_POFFS_BITS);
				ee_stop;
			}
			buf_flags [i] &= ~BF_DIRTY;
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
			return 1;

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
			if (nb > len)
				nb = len;

			// Write to the block
			bwrite (bi, wpageo, wbuffp, nb);

			len -= nb;
			if (len == 0) {
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
}

word ee_erase (word st, lword from, lword upto) {

	word nb;
	byte bi;

	if (from >= EE_SIZE)
		return 1;

	if (upto >= EE_SIZE || upto == 0)
		upto = EE_SIZE - 1;
	else if (upto < from)
		return 1;

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
		buf_page [0] = buf_page [1] = WNONE;

		// We get here when we have been synced, i.e., the buffers
		// are clean; hopefully, sync is stateless and idempotent

		// bwrite (0, 0, NULL, EE_PAGE_SIZE);
		wstate = WS_NPAGE;

	    case WS_NPAGE:

		while (wrsize) {
			wwait (st);
			ee_start;
			put_byte (EE_ERASE);
			saddr (wpagen);
			sdc (EE_POFFS_BITS);
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
		buf_page [0] = buf_page [1] = WNONE;
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
}

word ee_sync (word st) { 

	sync (st);
	return 0;
}

#include "storage.c"
