#ifndef	__wchan_c__
#define	__wchan_c__

#include "wchan.h"

#ifndef trc
#define	trc(a, ...)
#endif

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

	Assert (Ether == NULL, "RadioChannel->setup: only one Ether channel"
		" can exist in the present version");

	print ("RFChannel parameters:\n\n");

	RFChannel::setup (nt);

	BNoise = dBToLin (no);
	STB = st;
	STBL = sl;
	BitsPerByte = bpb;
	PacketFrameLength = frm;
	
	// This must match the order in which ivcc is filled in board.cc
	Rates = ivcc [0];
	RBoost = ivcc [1];
	RSSIC = ivcc [2];
	PS = ivcc [3];

	Channels = mxc;
	Ether = this;

	// Preprocess the BER table
	stb = (sir_to_ber_t*) STB;
	for (i = 0; i < STBL-1; i++)
		stb [i].fac = (stb [i+1].ber - stb [i].ber) /
			(stb [i].sir - stb [i+1].sir);

	print (nt, "  Number of xceivers:", 10, 26);
	print (no, "  Background noise (dBm):", 10, 26);
	print (BitsPerByte, "  Phys bits per byte:", 10, 26);
	print (PacketFrameLength, "  Phys header length:", 10, 26);
	print ("  -------SIR to -------BER ---table:\n");
	for (i = 0; i < STBL; i++)
		print (form ("  %10g    %10g\n", linTodB (stb [i].sir),
			stb [i].ber));
	print ("  ----------------------------------\n");
}

TIME RadioChannel::RFC_xmt (RATE r, Long tl) {

	assert ((tl & 0x7) == 0, "RFC_xmt: packet length %d not divisible by 8",
		tl);

	return (TIME) r * (LONG) ((tl >> 3) * BitsPerByte + PacketFrameLength);
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

IVMapper::IVMapper (unsigned short n, unsigned short *wt, double *dt,
								Boolean lg) {

	int i;

	NL = n;
	VLV = wt;

	Dec = (dt [1] < dt [0]);
	Log = lg;

	// Interpolation
	SLV = dt;
	if (NL > 1) {
		FAC = new double [NL - 1];
		for (i = 0; i < NL-1; i++)
			FAC [i] = (double) (SLV [i+1] - SLV [i]) /
				(VLV [i+1] - VLV [i]);
	} else {
		FAC = NULL;
	}
}
	
double IVMapper::setvalue (unsigned short w) {

	double d;
	int a, b, ix;

	a = 0; b = NL;

	do {
		ix = (a + b) >> 1;

		if (VLV [ix] <= w) {

			if (ix+1 == NL) {
				d = SLV [ix];
				goto Ret;
			}

			if (VLV [ix+1] <= w) {
				// Go to right
				a = ix+1;
				continue;
			}
			// Interpolate and return
			break;
		}

		if (ix == 0) {
			// At the beginning
			d = SLV [0];
			goto Ret;
		}

		if (VLV [ix-1] > w) {
			// Go to left
			b = ix;
			continue;
		}

		// Interpolate and return
		ix--;
		break;

	} while (1);

	// Interpolate
	d = ((w - VLV [ix]) * FAC [ix]) + SLV [ix];
Ret:
	return Log ? dBToLin (d) : d;
}

unsigned short IVMapper::getvalue (double v) {

	int a, b, ix;

	a = 0; b = NL;

	if (Log)
		v = linTodB (v);

	do {
		ix = (a + b) >> 1;

		if (vtor (SLV [ix], v)) {
			if (ix+1 == NL)
				// At the end
				return VLV [ix];

			if (vtor (SLV [ix+1], v)) {
				// Go to right
				a = ix+1;
				continue;
			}
			// Interpolate and return
			break;
		}

		if (ix == 0)
			// At the beginning
			return VLV [0];

		if (!vtor (SLV [ix-1], v)) {
			// Go to left
			b = ix;
			continue;
		}

		// Interpolate and return
		ix--;
		break;

	} while (1);

	return (unsigned short) ((v - SLV [ix]) / FAC [ix]) + VLV [ix];
}

Boolean IVMapper::exact (unsigned short w) {

	int a, b, ix;

	a = 0; b = NL;

	do {
		ix = (a + b) >> 1;

		if (VLV [ix] == w)
			return YES;

		if (VLV [ix] < w) {
			// Go to the right
			if (ix + 1 == NL || VLV [ix + 1] > w)
				return NO;

			a = ix + 1;
			continue;
		}

		// Go to the left
		if (ix == 0 || VLV [ix - 1] < w)
			return NO;

		b = ix;

	} while (1);
}

Boolean IVMapper::inrange (unsigned short w) {

	if (NL == 1)
		return (w == VLV [0]);

	return (w >= VLV [0] && w <= VLV [1]);
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

#endif
