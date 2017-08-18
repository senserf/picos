/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2017                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "kernel.h"
#include "storage.h"
#include "storage_mx25r8035.h"

#define	DOPEN_PERM	1
#define	DOPEN_TEMP	2

// ============================================================================

static lword		curr, last;
static const byte*	cptr;

static byte	dopen,		// How opened
		dstat;		// Operation

static word	left;

// ============================================================================

static byte get_byte () {

	int i;
	byte b;

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

	int i;

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

// ============================================================================

static void cmd_0 (byte cmd) {

	ee_start;
	put_byte (cmd);
	ee_stop;
}

static void cmd_3 (byte cmd, lword a) {

	ee_start;
	put_byte (cmd);
	put_byte ((byte)(a >> 16));
	put_byte ((byte)(a >>  8));
	put_byte ((byte)(a      ));
	ee_stop;
}

static byte rdsr () {
//
// Read the status register
//
	byte r;

	ee_start;
	put_byte (CMD_RDSR);
	r = get_byte ();
	ee_stop;
	return r;
}

static void wwait (word st, word del) {
//
// Wait for write to complete
//
	while (dopen && (rdsr () & 1)) {
		if (st == WNONE)
			mdelay (1);
		else {
			delay (del, st);
			release;
		}
	}
}

static void pdown () {
//
// Deep power down
//
	cmd_0 (CMD_DP);
}

static void copen () {

	// This is mostly void
	ee_bring_up;
	// Release from power down
	ee_start;
	udelay (1);
	ee_stop;
	udelay (1);
}

static void cclose () {

	if (dopen == DOPEN_TEMP) {
		wwait (WNONE, 1);
		pdown ();
		dopen = 0;
		ee_bring_down;
	}
}

static void ready () {
//
// Called before a new operation to make sure the chip is ready
//
	if (dopen) {
		// The device is open
		wwait (WNONE, 1);
	} else {
		copen ();
		dopen = DOPEN_TEMP;
	}

	dstat = 0;
}

// ============================================================================

word ee_open () {
//
// Open (for more than one operation)
//
	ready ();
	dopen = DOPEN_PERM;
#if 0
	{
		byte r [3];
		ee_start;
		put_byte (CMD_RDID);
		r [0] = get_byte ();
		r [1] = get_byte ();
		r [2] = get_byte ();
		ee_stop;
		diag ("ID: %x %x %x", r [0], r [1], r [2]);
	}
#endif
	return 0;
}

void ee_close () {

	if (dopen) {
		dopen = DOPEN_TEMP;
		cclose ();
	}
}

word ee_read (lword a, byte *s, word len) {

	if (len == 0)
		return 0;

	if (a > EE_SIZE || (a + len) > EE_SIZE)
		return 1;

	ready ();

	ee_start;

	put_byte (CMD_READ);
	put_byte ((byte)(a >> 16));
	put_byte ((byte)(a >>  8));
	put_byte ((byte)(a      ));

	while (len--)
		*s++ = get_byte ();

	ee_stop;

	cclose ();

	return 0;
}

word ee_erase (word st, lword from, lword upto) {

	if (dstat == 0) {

		// Starting up

		if (upto >= EE_SIZE || upto == 0)
			upto = EE_SIZE - 1;
		if (from >= EE_SIZE || upto < from)
			// Error
			return 1;

		ready ();

		if (from == 0 && upto == EE_SIZE) {

			// Special case: the entire chip

			cmd_0 (CMD_WREN);
			cmd_0 (dstat = CMD_CE);

		} else {

			// Initialize at 4K segment boundary
			curr = from & 0xfffff000;
			// Last + 1'st segment address
			last = (upto & 0xfffff000) + 0x1000;
ECont:
			// Continue
	
			if (curr >= last) {
EDone:
				// Done
				dstat = 0;
				cclose ();
				return 0;
			}

			if ((curr & 0xffff) == 0 && last - curr >= 0x10000)
				// 64K block
				dstat = CMD_BE64;
			else if ((curr & 0x7fff) == 0 && last - curr >= 0x8000)
				// 32K block
				dstat = CMD_BE32;
			else
				// 4K segment
				dstat = CMD_SE;

			cmd_0 (CMD_WREN);
			cmd_3 (dstat, curr);
		}
	}

	switch (dstat) {

		case CMD_CE:

			// This takes up to 75 seconds
			wwait (st, 1024);
			goto EDone;

		case CMD_BE64:

			wwait (st, 300);
			curr += 0x10000;
			// Continue
			goto ECont;

		case CMD_BE32:

			wwait (st, 200);
			curr += 0x8000;
			// Continue
			goto ECont;

		case CMD_SE:

			wwait (st, 50);
			curr += 0x1000;
			goto ECont;
	}

	// Something wrong
	return 1;
}

word ee_write (word st, lword a, const byte *s, word len) {

	word bl;

	if (dstat == 0) {

		// Starting up

		if (a >= EE_SIZE || (a + len) > EE_SIZE)
			return 1;

		curr = a;
		left = len;
		cptr = s;

		ready ();
		dstat = CMD_PP;
WCont:
		if (left == 0) {
			// Done
			dstat = 0;
			cclose ();
			return 0;
		}

		// The number of bytes left on this page
		bl = 256 - (a & 0xff);

		if (bl > len)
			bl = (word) len;

		// Write the bytes
		cmd_0 (CMD_WREN);

		ee_start;

		put_byte (CMD_PP);
		put_byte ((byte)(curr >> 16));
		put_byte ((byte)(curr >>  8));
		put_byte ((byte)(curr      ));

		while (bl--) {
			put_byte (*cptr++);
			left--;
			curr++;
		}

		ee_stop;
	}

	if (dstat != CMD_PP)
		// Something wrong
		return 1;

	wwait (st, 1);
	goto WCont;
}

void ee_panic () {

	// Not needed
}

word ee_sync (word st) {

	// Not needed
	return 0;
}

// ============================================================================

#include "storage_eeprom.h"
