/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "kernel.h"
#include "storage.h"
#include "storage_mt29xxx.h"

#define	WS_DONE		0
#define	WS_ERASING	1
#define	WS_ERWAIT	2
#define	WS_WLOOP	3

#define	AS_IDLE		0
#define	AS_READ		1
#define	AS_WRITE	2
#define	AS_FLUSH	3

static lword 		current_page = LWNONE;
static word 		wleft;
static byte 		wstate = WS_DONE, 
			astate = 0;		// Memory state

#define	waitnb		while (!ee_ready)

#define	cblock	(((word*)&current_page) [0])
#define	lblock	(((word*)&current_page) [1])

#define	ee_reset do { \
		ee_cmd_mode; \
		ee_start; \
		ee_obuf = EE_CMD_RST; \
		ee_strobe_out; \
		ee_stop; \
		waitnb; \
	} while (0)

static void wwait (word st) {

	if (st == WNONE) {
		waitnb;
	} else if (!ee_ready) {
		delay (1, st);
		release;
	}
}

static byte get_status () {

	byte es;

	// Not needed: output is the homeostatic default
	// ee_set_output;
	ee_cmd_mode;
	ee_start;
	ee_obuf = EE_CMD_STA;
	ee_strobe_out;
	ee_set_input;
	ee_dat_mode;
	ee_strobe_in;
	es = ee_ibuf;
	ee_stop;
	ee_set_output;
	return es;
}

static void start_read (byte *buf, lword pn, word po, word len) {

	waitnb;

	// ee_set_output;
	ee_cmd_mode;
	ee_start;

	current_page = pn;

	ee_obuf = EE_CMD_PR0;
	ee_strobe_out;

	ee_adr_mode;

	ee_obuf = (byte) po;			// byte 0 [oooooooo]
	ee_strobe_out;

	ee_obuf = (byte)((po & 0x0700) >> 8);
	ee_strobe_out;				// byte 1 [00000ooo]

	ee_obuf = (byte) pn;			// byte 2 [pppppppp]
	ee_strobe_out;

	pn >>= 8;
	ee_obuf = (byte) pn;			// byte 3 [pppppppp]
	ee_strobe_out;

	pn >>= 8;
	ee_obuf = ((byte) pn) & 0x01;		// byte 4 [0000000p]
	ee_strobe_out;

	ee_cmd_mode;
	ee_obuf = EE_CMD_PR1;
	ee_strobe_out;

	ee_stop;

	udelay (10);
	waitnb;

	ee_set_input;
	ee_dat_mode;
	ee_start;
	
	while (len--) {
		ee_strobe_in;
		*buf++ = ee_ibuf;
	}

	ee_set_output;
	astate = AS_READ;

	// ... left open for reading
}

static void get_bytes (byte *buf, word off, word len) {

	// ee_set_output;
	ee_cmd_mode;

	ee_obuf = EE_CMD_RR0;
	ee_strobe_out;

	ee_adr_mode;

	ee_obuf = (byte) off;
	ee_strobe_out;

	ee_obuf = ((byte)(off >> 8)) & 0x07;
	ee_strobe_out;

	ee_cmd_mode;
	ee_obuf = EE_CMD_RR1;
	ee_strobe_out;

	ee_set_input;
	ee_dat_mode;

	while (len--) {
		ee_strobe_in;
		*buf++ = ee_ibuf;
	}

	ee_set_output;
	astate = AS_READ;

	// ... keep reading
}

static void start_write (const byte *buf, lword pn, word po, word len) {

	// ee_set_output;
	ee_cmd_mode;
	ee_start;

	current_page = pn;

	ee_obuf = EE_CMD_PPG;
	ee_strobe_out;

	ee_adr_mode;

	ee_obuf = (byte) po;			// byte 0 [oooooooo]
	ee_strobe_out;

	ee_obuf = (byte)((po & 0x0700) >> 8);
	ee_strobe_out;				// byte 1 [00000ooo]

	ee_obuf = (byte) pn;			// byte 2 [pppppppp]
	ee_strobe_out;

	pn >>= 8;
	ee_obuf = (byte) pn;			// byte 3 [pppppppp]
	ee_strobe_out;

	pn >>= 8;
	ee_obuf = ((byte) pn) & 0x01;		// byte 4 [0000000p]
	ee_strobe_out;

	ee_dat_mode;

	while (len--) {
		ee_obuf = *buf++;
		ee_strobe_out;
	}

	// This is where we stop; the line is open and ready for more stuff ...
	astate = AS_WRITE;
}

static void append_bytes (const byte *buf, word po, word len) {

	// ... which we provide in here ...

	ee_cmd_mode;
	ee_obuf = EE_CMD_PWR;
	ee_strobe_out;

	ee_adr_mode;

	ee_obuf = (byte) po;
	ee_strobe_out;

	ee_obuf = ((byte)(po >> 8)) & 0x07;
	ee_strobe_out;

	ee_dat_mode;

	while (len--) {
		ee_obuf = *buf++;
		ee_strobe_out;
	}

	// Again, the operation is left dangling ...
	astate = AS_WRITE;
}

word ee_sync (word st) {

	// ... until now

	word es;

	es = 0;

	switch (astate) {

	    case AS_IDLE:

		break;

	    case AS_READ:

		ee_stop;
		// ee_set_output;
		break;

	    case AS_WRITE:

		// Flush
		ee_cmd_mode;
		ee_obuf = EE_CMD_FLU;
		ee_strobe_out;
		ee_stop;
		udelay (10);

		astate = AS_FLUSH;

	    case AS_FLUSH:

		// Wait until done
		wwait (st);
		es = get_status () & 0x01;
		astate = AS_IDLE;
		current_page = LWNONE;
		break;
	}
		
	current_page = LWNONE;
	astate = AS_IDLE;
	ee_wp_low;
	ee_reset;
	return es;
}

word ee_open () {

	ee_bring_up;
	ee_reset;
	return 0;
}

void ee_close () {

	ee_sync (WNONE);
	ee_bring_down;
}

word ee_read (lword a, byte *s, word len) {

	lword pn;
	word po, nb;

	if (len == 0)
		return 0;

	if ((astate != AS_IDLE && astate != AS_READ) || wstate != WS_DONE)
		// write/erase in progress
		syserror (ENOTNOW, "ee_read");

	if (a >= EE_SIZE || (a + len) > EE_SIZE)
		return 1;

	// Page offset
	po = (word)(a & (EE_PAGE_SIZE - 1));
	// Page number
	pn = (a >> EE_PAGE_SHIFT);

	while (1) {

		nb = (word) EE_PAGE_SIZE - po;
		if (nb > len)
			nb = len;

		if (pn != current_page) {
			// Fetch the page
			start_read (s, pn, po, nb);
		} else {
			get_bytes (s, po, nb);
		}

		len -= nb;
		if (len == 0)
			return 0;
		pn++;
		po = 0;
	}
}

word ee_erase (word st, lword from, lword upto) {
/*
 * Note: this necessarily erases entire blocks. from and upto are addresses,
 * but whatever blocks the fall into, those blocks will be erased completely.
 */
	byte es;

	if (wstate == WS_DONE) {

		if (from >= EE_SIZE)
			return 1;

		if (upto >= EE_SIZE || upto == 0)
			upto = EE_SIZE - 1;
		else if (upto < from)
			return 1;

		// Count errors
		wleft = ee_sync (st);

		cblock = (word)((from >> EE_BLOCK_SHIFT) & 0x0FFFF);
		lblock = (word)((upto >> EE_BLOCK_SHIFT) & 0x0FFFF);

		wstate = WS_ERASING;
		ee_wp_high;
	}

	switch (wstate) {

	    case WS_ERASING:
ELoop:
		// Start erase for a new block
		// ee_set_output;
		ee_cmd_mode;
		ee_start;
		
		ee_obuf = EE_CMD_BE0;
		ee_strobe_out;

		ee_adr_mode;

		ee_obuf = (byte) (cblock << 6);
		ee_strobe_out;

		ee_obuf = (byte) (cblock >> 2);
		ee_strobe_out;

		ee_obuf = (byte) ((cblock >> 10) & 0x01);
		ee_strobe_out;

		ee_cmd_mode;
		ee_obuf = EE_CMD_BE1;
		ee_strobe_out;

		ee_stop;

		wstate = WS_ERWAIT;
		udelay (10);

		// Fall through

	    case WS_ERWAIT:

		// Wait for erase to complete
		wwait (st);

		// Count errors
		if (wleft != MAX_WORD)
			wleft += (get_status () & 0x01);

		if (cblock <= lblock) {
			cblock++;
			wstate = WS_ERASING;
			goto ELoop;
		}

		// All done
		wstate = WS_DONE;
		current_page = LWNONE;
		ee_wp_low;
		return wleft;

	}

	syserror (ENOTNOW, "ee_erase");
}

word ee_write (word st, lword a, const byte *s, word len) {

	lword	pn;
	word	po, nb;

	if (wstate == WS_DONE) {

		wwait (st);

		if (astate != AS_WRITE && astate != AS_IDLE) {
			if (astate == AS_READ)
				ee_sync (st);
			else
				syserror (ENOTNOW, "ee_write");
		}

		if (len == 0)
			return 0;

		if (a >= EE_SIZE || (a + len) > EE_SIZE)
			return 1;

		wleft = len;
		wstate = WS_WLOOP;
		ee_wp_high;

	} else if (wstate != WS_WLOOP) {

		syserror (ENOTNOW, "ee_write");

	}

	while (1) {

		if (wleft == 0) {
			wstate = WS_DONE;
			return 0;
		}

		// The progress
		a += (len - wleft);
		s += (len - wleft);
		po = (word)(a & (EE_PAGE_SIZE - 1));
		// Page number
		pn = (a >> EE_PAGE_SHIFT);

		// How much to write in this turn
		nb = (word) EE_PAGE_SIZE - po;
		if (nb > wleft)
			nb = wleft;

		if (astate == AS_WRITE || astate == AS_FLUSH) {
			// CE is down, we are enabled and in the middle of a
			// rather tricky command sequence
			if (current_page == pn) {
				// Continue writing to the same page
				append_bytes (s, po, nb);
				wleft -= nb;
				continue;
			}
			// FIXME: check for continuity???
			// Switch to a new page
			if (ee_sync (st)) {
				// Error
				wstate = WS_DONE;
				return wleft;
			}
			// sync brings it down
			ee_wp_high;
		}

		// Starting up a new page
		start_write (s, pn, po, nb);
		// udelay (10);
		wleft -= nb;
	}
}

#include "storage_eeprom.h"
