#include "sysio.h"
#include "oep.h"
#include "oep_ee.h"
#include "storage.h"

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//
// Functions for sending/receiving blocks of EEPROM via OEP
//

typedef struct {
//
// This is the data structure that must be maintained while a transaction is
// in progress: it stores the origin and length of the EEPROM block, as well
// as the number of chunks (-1)
//
	lword FWA, LEN;
	word NC;

} ee_rxdata_t;

//
// Data hook: the pointer to our data. Remember to deallocate it when the
// transaction is over (the praxis should call oep_ee_cleanup
//
static ee_rxdata_t *EE_RXDATA = NULL;

static byte rxdinit (lword fwa, lword len) {
//
// Initialize the data, calculate the number of chunks (-1)
//
	lword es, lw;

	es = ee_size (NULL, NULL);
	lw = fwa + len;
	if (fwa >= es || lw > es || lw < fwa)
		return OEP_STATUS_PARAM;

	if ((EE_RXDATA = (ee_rxdata_t*)umalloc (sizeof (ee_rxdata_t))) == NULL)
		return OEP_STATUS_NOMEM;

	EE_RXDATA->FWA = fwa;
	EE_RXDATA->LEN = len;
	// The number of chunks - 1
	EE_RXDATA->NC = (len - 1) / OEP_CHUNKLEN;

	return 0;
}

static void chunk_out (word state, address pay, word chn) {
//
// Chunk handler for the sender: fills the payload with the specified chunk
//
	lword off;

	// Offset
	off = ((lword)chn) * OEP_CHUNKLEN;

	if (chn < EE_RXDATA->NC) {
		// Full length, possibly except the last chunk
		chn = OEP_CHUNKLEN;
	} else {
		if (chn > EE_RXDATA->NC)
			// This will not happen but be prepared
			return;
		// Last chunk
		chn = (word) (EE_RXDATA->LEN - off);
	}
	// state argument not needed: we never block
	ee_read (EE_RXDATA->FWA + off, (byte*)pay, chn);
}

byte oep_ee_snd (word ylid, byte yrqn, lword fwa, lword len) {
//
// Sender side initializer (you call this instead of oep_snd, which is
// called below); fwa/len are the origin and length of the EEPROM block
//
	byte st;

	if ((st = rxdinit (fwa, len)) != 0)
		return st;

	// Start the protocol
	if ((st = oep_snd (ylid, yrqn, EE_RXDATA->NC + 1, chunk_out)) != 0) {
		ufree (EE_RXDATA);
		return st;
	}
	return 0;
}

static void chunk_in (word state, address pay, word chn) {
//
// Chunk handler for the receiver
//
	lword off;

	off = ((lword)chn) * OEP_CHUNKLEN;

	if (chn < EE_RXDATA->NC) {
		// Length 
		chn = OEP_CHUNKLEN;
	} else {
		if (chn > EE_RXDATA->NC)
			// Just in case
			return;
		// Last chunk
		chn = (word) (EE_RXDATA->LEN - off);
	}

	// This time state becomes handy
	ee_write (state, EE_RXDATA->FWA + off, (byte*)pay, chn);
}

byte oep_ee_rcv (lword fwa, lword len) {
//
// Sender side init
//
	byte st;

	if ((st = rxdinit (fwa, len)) != 0)
		return st;

	// Start the protocol
	if ((st = oep_rcv (EE_RXDATA->NC + 1, chunk_in)) != 0) {
		ufree (EE_RXDATA);
		return st;
	}
	return 0;
}

void oep_ee_cleanup () {
//
// Deallocate data (to be called by the praxis when the transaction is over -
// one way or the other)
//
	ufree (EE_RXDATA);
	ee_sync (WNONE);
}
