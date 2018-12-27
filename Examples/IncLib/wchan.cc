/*
	Copyright 1995-2018, 2019 Pawel Gburzynski

	This file is part of SMURPH/SIDE.

	SMURPH/SIDE is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	SMURPH/SIDE is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SMURPH/SIDE. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef	__wchan_c__
#define	__wchan_c__

#include "wchan.h"

RadioChannel *Ether = NULL;

void RadioChannel::setup (

	Long nt,		// The number of transceivers
	double no,		// Background noise
	const sir_to_ber_t *st,	// SIR to BER conversion table
	int sl,			// Length of the conversion table
	int bpb,		// Bits per byte
	int frm,		// Packet frame (extra bits)
	IVMapper **ivcc,	// Value converters
	MXChannels *mxc		// Channels
) {
	int i;
	sir_to_ber_t *stb;
	double v, w;
	unsigned short r;

	Assert (Ether == NULL, "RadioChannel->setup: only one Ether channel"
		" can exist in the present version");

	print ("RFChannel:\n\n");

	RFChannel::setup (nt);

	BNoise = (no == -HUGE) ? 0.0 : dBToLin (no);
	STB = st;
	STBL = sl;
	BitsPerByte = bpb;
	PacketFrameLength = frm;
	
	// This must match the order in which ivcc is filled
	Rates = ivcc [XVMAP_RATES];
	RBoost = ivcc [XVMAP_RBOOST];
	RSSIC = ivcc [XVMAP_RSSI];
	PS = ivcc [XVMAP_PS];

	Channels = mxc;
	Ether = this;

	setAevMode (NO);

	// Preprocess the BER table
	stb = (sir_to_ber_t*) STB;
	for (i = 0; i < STBL; i++)
		stb [i].sir = dBToLin (stb [i].sir);
	for (i = 0; i < STBL-1; i++)
		stb [i].fac = (stb [i+1].ber - stb [i].ber) /
			(stb [i].sir - stb [i+1].sir);

	print (nt, "  Number of nodes:", 10, 26);
	if (no != -HUGE)
		print (no, "  Background noise (dBm):", 10, 26);
	print (BitsPerByte, "  Phys bits per byte:", 10, 26);
	print (PacketFrameLength, "  Phys header length:", 10, 26);
	if (STBL) {
		print ("\n   SIR(dB)           BER\n");
		for (i = 0; i < STBL; i++)
			print (form ("  %8g    %10g\n", linTodB (stb [i].sir),
				stb [i].ber));
	}

	if (Rates) {
		print ("\n      Rate         BPS  Boost(dB)\n");
		for (i = 0; i < Rates->size (); i++) {
			r = Rates->row (i, v);
			if (RBoost)
				RBoost->row (i, w);
			else
				w = 0.0;
			print (form ("%10d %11g %10g\n", r, v, w));
		}
	}

	if (RSSIC) {
		print ("\n      RSSI  Signal(dBm)\n");
		for (i = 0; i < RSSIC->size (); i++) {
			r = RSSIC->row (i, v);
			print (form ("%10d     %8g\n", r, v));
		}
	}

	if (PS) {
		print ("\n   Setting  Power(dBm)\n");
		for (i = 0; i < PS->size (); i++) {
			r = PS->row (i, v);
			print (form ("%10d    %8g\n", r, v));
		}
	}

	if (Channels)
		Channels->print ();
}

TIME RadioChannel::RFC_xmt (RATE r, Packet *p) {

	assert ((p->TLength & 0x7) == 0,
		"RFC_xmt: packet length %d not divisible by 8", p->TLength);

	return (TIME) r * (LONG) (((p->TLength) >> 3) * BitsPerByte +
		PacketFrameLength);
}

double RadioChannel::ber (double sir) {
/*
 * Converts (linear) SIR to BER by interpolating entries in
 * STB.
 */
//	double res;
	int i;
#if 1
	int a, b;
	// The bisection version

	a = 0; b = STBL;

	do {
		i = (a + b) >> 1;

		if (sir > STB [i] . sir) {
			if (i == 0) {
				// Return the lowest ber in the table
// res = STB [0] . ber;
// break;
				return STB [0] . ber;
			}
			if (sir > STB [i-1] . sir) {
				// go to left
				b = i;
				continue;
			}
			// Interpolate
// res = (STB [i-1].sir - sir) * STB [i-1].fac + STB [i-1].ber;
// break;
			return (STB [i-1].sir - sir) * STB [i-1].fac +
				STB [i-1].ber;
		}

		if (i == STBL - 1) {
			// The error rate is 1.0
// res = 1.0;
// break;
			return 1.0;
		}

		if (sir > STB [i+1] . sir) {
			// Interpolate
// res = (STB [i].sir - sir) * STB [i].fac + STB [i].ber;
// break;
			return (STB [i].sir - sir) * STB [i].fac + STB [i].ber;
		}

		// Go to right
		a = i + 1;
	} while (1);

// trace ("SIR: %f, BER %g", linTodB (sir), res);
// return res;

#else
	// The linear version
		
	for (i = 0; i < STBL; i++) {
		// The table is ordered from high SIR to low SIR
		if (sir > STB [i] . sir)
			// First one less than the argument
			break;
	}

	if (i == 0) {
		// The top, return the lowest BER in the table
		trc ("ber: [%f] -> %f (low)", sir, STB[0].ber);
		return STB [0] . ber;
	}

	if (i == STBL) {
		// We hit the bottom, the error rate is 1.0
		trc ("ber: [%f] -> 1.0 (high)", sir);
		return 1.0;
	}

	// Interpolate
	return (STB [i-1].sir - sir) * STB [i-1].fac + STB [i-1].ber;

#endif	/* Bisection or Linear */

}

MXChannels::MXChannels (unsigned short nc, int nsep, double *sep) {

	int i;

	NC = nc;

	if ((NSEP = nsep) > 0) {
		SEP = sep;
		// Turn them into reverse linear factors
		for (i = 0; i < NSEP; i++)
			SEP [i] = dBToLin (-SEP [i]);
	}
}

void MXChannels::print () {

	int i;

	::print (form ("\n  %1d Channels", NC));
	if (NSEP) {
		::print (", sep(dB): ");
		for (i = 0; i < NSEP; i++)
			::print (form (" %g", -linTodB (SEP [i])));
	}
	::print ("\n");
}
	
#endif
