/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __oep_ee_c__
#define	__oep_ee_c__

#include "sysio.h"
#include "oep.h"
#include "oep_ee.h"
#include "storage.h"

#include "oep_ee_types.h"


//
// Functions for sending/receiving blocks of EEPROM via OEP
//


static ee_rxdata_t *oep_ee_rxdata __sinit (NULL);

static byte rxdinit (lword fwa, lword len) {
//
// Initialize the data, calculate the number of chunks (-1)
//
	lword es, lw;

	es = ee_size (NULL, NULL);
	lw = fwa + len;
	if (fwa >= es || lw > es || lw < fwa) {
		return OEP_STATUS_PARAM;
	}

	if ((oep_ee_rxdata = (ee_rxdata_t*)umalloc (sizeof (ee_rxdata_t))) == NULL)
		return OEP_STATUS_NOMEM;

	oep_ee_rxdata->FWA = fwa;
	oep_ee_rxdata->LEN = len;
	// The number of chunks - 1
	oep_ee_rxdata->NC = (len - 1) / OEP_CHUNKLEN;

	return 0;
}

static void oep_ee_chunk_out (word state, address pay, word chn) {
//
// Chunk handler for the sender: fills the payload with the specified chunk
//
	lword off;

	// Offset
	off = ((lword)chn) * OEP_CHUNKLEN;

	if (chn < oep_ee_rxdata->NC) {
		// Full length, possibly except the last chunk
		chn = OEP_CHUNKLEN;
	} else {
		if (chn > oep_ee_rxdata->NC)
			// This will not happen but be prepared
			return;
		// Last chunk
		chn = (word) (oep_ee_rxdata->LEN - off);
	}
	// state argument not needed: we never block
	ee_read (oep_ee_rxdata->FWA + off, (byte*)pay, chn);
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
	if ((st = oep_snd (ylid, yrqn, oep_ee_rxdata->NC + 1,
						      oep_ee_chunk_out)) != 0) {
		ufree (oep_ee_rxdata);
		return st;
	}
	return 0;
}

static void oep_ee_chunk_in (word state, address pay, word chn) {
//
// Chunk handler for the receiver
//
	lword off;

	off = ((lword)chn) * OEP_CHUNKLEN;

	if (chn < oep_ee_rxdata->NC) {
		// Length 
		chn = OEP_CHUNKLEN;
	} else {
		if (chn > oep_ee_rxdata->NC)
			// Just in case
			return;
		// Last chunk
		chn = (word) (oep_ee_rxdata->LEN - off);
	}

	// This time state becomes handy
	ee_write (state, oep_ee_rxdata->FWA + off, (byte*)pay, chn);
}

byte oep_ee_rcv (lword fwa, lword len) {
//
// Sender side init
//
	byte st;

	if ((st = rxdinit (fwa, len)) != 0)
		return st;

	// Start the protocol
	if ((st = oep_rcv (oep_ee_rxdata->NC + 1, oep_ee_chunk_in)) != 0) {
		ufree (oep_ee_rxdata);
		return st;
	}
	return 0;
}

void oep_ee_cleanup () {
//
// Deallocate data (to be called by the praxis when the transaction is over -
// one way or the other)
//
	ufree (oep_ee_rxdata);
	ee_sync (WNONE);
}

#endif
