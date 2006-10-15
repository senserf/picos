#ifndef	__chan_radio_c__
#define	__chan_radio_c__

#include "chan_radio.h"

#ifndef trc
#define	trc(a, ...)
#endif

RadioChannel *Ether = NULL;

void RadioChannel::setup (

	Long nt,		// The number of transceivers
	double no,		// Background noise
	const sir_to_ber_t *st,	// SIR to BER conversion table
	int    sl,		// Length of the conversion table
	Long br,		// Bit rate
	int bpb,		// Bits per byte
	int frm			// Packet frame (excl. prmbl and chksum)
) {

	Assert (Ether == NULL, "RadioChannel->setup: only one Ether channel"
		" can exist in the present version");

	RFChannel::setup (nt);

	BNoise = dBToLin (no);
	STB = st;
	STBL = sl;
	BitRate = br;
	BitsPerByte = bpb;
	PacketFrameLength = frm;

	Ether = this;
}

double RadioChannel::ber (double sir) {
/*
 * Converts (linear) SIR to BER by interpolating entries in
 * STB.
 */
	int i;
	double res;

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
	res = ((STB [i-1].sir - sir) / (STB [i-1].sir - STB [i].sir)) *
	       (STB [i].ber - STB [i-1].ber) + STB [i-1].ber;

	trc ("ber: [%f] -> %f", sir, res);

	return res;
}

#endif
