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
	Long br,		// Bit rate
	int bpb,		// Bits per byte
	int frm,		// Packet frame (extra bits)
	RSSICalc *rsc,		// RSSI calculator
	PowerSetter *ps
) {
	int i;
	sir_to_ber_t *stb;

	Assert (Ether == NULL, "RadioChannel->setup: only one Ether channel"
		" can exist in the present version");

	RFChannel::setup (nt);

	BNoise = dBToLin (no);
	STB = st;
	STBL = sl;
	BitRate = br;
	BitsPerByte = bpb;
	PacketFrameLength = frm;
	RSSIC = rsc;
	PS = ps;
	Ether = this;

	// Preprocess the BER table
	stb = (sir_to_ber_t*) STB;
	for (i = 0; i < STBL-1; i++)
		stb [i].fac = (stb [i+1].ber - stb [i].ber) /
			(stb [i].sir - stb [i+1].sir);
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
	int i;
//	double res;
#if 1
	// The bisection version

	i = STBL >> 1;

	do {
		if (sir > STB [i] . sir) {
			if (i == 0) {
				// Return the lowest ber in the table
// res = STB [0] . ber;
// break;
				return STB [0] . ber;
			}
			if (sir > STB [i-1] . sir) {
				// go to left
				i >>= 1;
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
		i = (i + 1 + STBL) >> 1;
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

RSSICalc::RSSICalc (unsigned short n, unsigned short m, unsigned short *wt,
								double *dt) {

	int i;

	NL = n;
	Mode = m;
	VLV = wt;
	SLV = dt;

	if (Mode >= 2) {
		// Interpolation
		FAC = new double [NL - 1];
		for (i = 0; i < NL-1; i++) {
			FAC [i] = (double) (VLV [i+1] - VLV [i]) /
				(SLV [i+1] - SLV [i]);
		}
	} else {
		FAC = NULL;
		SLV [2] = (double) (VLV [1] - VLV [0]);
		SLV [1] = SLV [2] / (SLV [1] - SLV [0]);
	}
}
	
unsigned short RSSICalc::calculate (double sl) {

	int ix;

	switch (Mode) {

	    case 0:

		sl = linTodB (sl);

	    case 1:

		// Transformation

		sl = ((sl - SLV [0]) * SLV [1]);

		if (sl < 0)
			return VLV [0];

		if (sl > SLV [2])
			return VLV [1];

		return (unsigned short) sl + VLV [0];

	    case 2:

		sl = linTodB (sl);

	    case 3:

		// Interpolation linear

		ix = NL >> 1;

		do {
			if (SLV [ix] <= sl) {
				if (ix+1 == NL)
					// At the end
					return VLV [ix];
				if (SLV [ix+1] <= sl) {
					// Go to right
					ix = (ix + NL + 1) >> 1;
					continue;
				}
				// Interpolate and return
				return (unsigned short) ((sl - SLV [ix]) *
					FAC [ix]) + VLV [ix];
			}

			if (ix == 0)
				// At the beginning
				return VLV [0];

			if (SLV [ix-1] > sl) {
				// Go to left
				ix >>= 1;
				continue;
			}

			// Interpolate and return
			return (unsigned short) ((sl - SLV [ix-1]) *
				FAC [ix-1]) + VLV [ix-1];
		} while (1);

	    default:

		excptn ("RSSICalc->calculate: illegal mode %d", Mode);
	
	}
}
	
PowerSetter::PowerSetter (unsigned short n, unsigned short m,
					unsigned short *wt, double *dt) {

	int i;

	NL = n;
	Mode = m;
	VLV = wt;
	SLV = dt;

	if (Mode >= 2) {
		// Interpolation
		FAC = new double [NL - 1];
		for (i = 0; i < NL-1; i++) {
			FAC [i] = (double) (SLV [i+1] - SLV [i]) /
				(VLV [i+1] - VLV [i]);
		}
	} else {
		FAC = NULL;
		SLV [2] = (SLV [1] - SLV [0]);
		SLV [3] = SLV [2] / (VLV [1] - VLV [0]);
		SLV [4] = (double) (VLV [1] - VLV [0]);
	}
}
	
double PowerSetter::setvalue (unsigned short w) {

	int ix;
	double d;

	switch (Mode) {

	    case 0:
	    case 1:

		d = (double)(w - VLV [0]) * SLV [3];

		if (d < 0.0) {
			d = SLV [0];
		} else if (d > SLV [2]) {
			d = SLV [1];
		} else {
			d += SLV [0];
		}

		if (Mode == 0)
			d = dBToLin (d);

		return d;

	    case 2:
	    case 3:

		ix = NL >> 1;

		do {
			if (VLV [ix] <= w) {
				if (ix+1 == NL) {
					// At the end
					d = SLV [ix];
					break;
				}
				if (VLV [ix+1] <= w) {
					// Go to right
					ix = (ix + NL + 1) >> 1;
					continue;
				}
				// Interpolate and return
				d = ((w - VLV [ix]) * FAC [ix]) + SLV [ix];
				break;
			}

			if (ix == 0) {
				// At the beginning
				d = SLV [0];
				break;
			}

			if (VLV [ix-1] > w) {
				// Go to left
				ix >>= 1;
				continue;
			}

			// Interpolate and return
			d = ((w - VLV [ix-1]) * FAC [ix-1]) + SLV [ix-1];
			break;

		} while (1);

		if (Mode == 2)
			d = dBToLin (d);

		return d;

	    default:

		excptn ("PowerSetter->setvalue: illegal mode %d", Mode);
	
	}
}

unsigned short PowerSetter::getvalue (double v) {

	int ix;
	double d;
	unsigned short w;

	switch (Mode) {

	    case 0:

		v = linTodB (v);
		
	    case 1:

		d = (double)(v - SLV [0]) / SLV [3];

		if (d < 0.0) {
			w = VLV [0];
		} else if (d > SLV [4]) {
			w = VLV [1];
		} else {
			w = (unsigned short) d + VLV [0];
		}

		return w;

	    case 2:

		v = linTodB (v);

	    case 3:

		ix = NL >> 1;

		do {
			if (SLV [ix] <= v) {
				if (ix+1 == NL) {
					// At the end
					w = VLV [ix];
					break;
				}
				if (SLV [ix+1] <= v) {
					// Go to right
					ix = (ix + NL + 1) >> 1;
					continue;
				}
				// Interpolate and return
				w = (unsigned short) ((v - SLV [ix]) /
					FAC [ix]) + VLV [ix];
				break;
			}

			if (ix == 0) {
				// At the beginning
				w = VLV [0];
				break;
			}

			if (SLV [ix-1] > v) {
				// Go to left
				ix >>= 1;
				continue;
			}

			// Interpolate and return
			w = (unsigned short) ((v - SLV [ix-1]) / FAC [ix-1]) +
				VLV [ix-1];
			break;

		} while (1);

		return w;

	    default:

		excptn ("PowerSetter->getvalue: illegal mode %d", Mode);
	}
}

#endif
