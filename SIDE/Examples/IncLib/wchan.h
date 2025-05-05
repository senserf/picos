/*
	Copyright 1995-2020 Pawel Gburzynski

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

#ifndef	__wchan_h__
#define	__wchan_h__

// =========================================================================
// General framework for a wireless channel model (version V - used in VUEE)
// =========================================================================

#ifdef ZZ_RF_TRACING
#define	trc(a, ...)	trace (a, ## __VA_ARGS__)
#endif

#ifndef trc
#define	trc(a, ...)
#endif

// For switchng off randomness
#define	RND_OFF_ATTENUATION	0
#define	RND_OFF_ERRORS		1
#define	RND_OFF_CHOICE		2

// Standard indexes into the XVMapper table; we keep here the tally of all
// entries over all channel types
#define	XVMAP_RATES		0
#define	XVMAP_RBOOST	1
#define	XVMAP_RSSI		2
#define	XVMAP_PS		3
#define	XVMAP_ATT		4
#define	XVMAP_SIGMA		5
#define	XVMAP_MODES		6	// Not used yet
#define	XVMAP_SIZE		7

// ===========================================================================
// PG 250214
// The partitioning of the Tag values:
//		bits  0- 7:	channel
//		bits  8-15:	rate index
//		bits 16-24:	mode (position)

#define RF_MAX_N_CHANNELS	256
#define	RF_MAX_N_RATES		256
#define	RF_MAX_N_MODES		256
// ===========================================================================

typedef struct {
	// This structure represents a single entry mapping SIR (Signal to
	// Interference Ratio) to BER (Bit Error Rate). The user specifies
	// a bunch of discrete entries, and the actual values (for SIR in
	// between are interpolated. Does this make sense?
	double	sir, ber, fac;
	// Note: the third value is a precalculated factor to save on
	// multiplication/division
} sir_to_ber_t;

template <class RType> class XVNdexer {

	// This is an indexer, i.e., basically, an array. Added it on 250215
	// to account for "modes" implementing my idea of orientation-dependent
	// propagation features.

	unsigned short NL;	// Number of elements
	RType *VLV;			// The values

	public:

	XVNdexer (unsigned short n, RType *wt) {
		VLV = wt;
		NL = n;
	};

	inline unsigned short size () { return NL; };

	// These stupid and doubly confusing names are for compatypility with
	// XVMapper
	inline int getvalue (RType w) {
	//
	// Returns the index of the value
	//
		int ix;
		for (ix = 0; ix < NL; ix++)
			if (VLV [ix] == w)
				// Must be found exactly to succeed
				return ix;
		return -1;
	};

	inline RType setvalue (int ix) {
	//
	// Returns the value given an index
	//
		assert (ix >= 0 && ix < NL, "XVNdexer: index out of range");
		return VLV [ix];
	};
};
		
template <class RType> class XVMapper {

	// This is a generic converter between model parameters (like signal
	// levels or bit rates) and their representations for the praxis.
	// For example, there may be 8 discrete power level setting
	// represented by numbers from 0 to 7 that should be mapped into
	// FP multipliers. The converter is able to do it both ways, i.e.,
	// convert discrete representations to the actual values, as well
	// as convert the values to their discrete representations.
	// As the converter uses interpolation, the values must be monotonic.

	Boolean Dec,		// Decreasing order of values
		Log;		// Logarithmic
	unsigned short NL;	// Number of levels
	RType *VLV;		// Representations
	double *SLV;		// Values
	double *FAC;		// To speed up interpolation

	inline Boolean vtor (double a, double b) {
		// Returns YES if b's representation is larger than a's, i.e.,
		// b is to the "right" of a 
		return (Dec && (a >= b)) || (!Dec && (a <= b));
	}

	public:

	XVMapper (unsigned short n, RType *wt, double *dt, Boolean lg = NO) {

		int i;

		NL = n;
		VLV = wt;

		if (n < 2)
			Dec = 0;
		else
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
	};

	unsigned short size () { return NL; };

	RType row (int n, double &v) {
		v = SLV [n];
		return VLV [n];
	};

	double setvalue (RType w, Boolean lin = YES) {
	//
	// Converts representation to value
	//
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
		return (Log && lin) ? dBToLin (d) : d;
	};

	RType getvalue (double v, Boolean db = YES) {
	//
	// Converts value to representation
	//
		int a, b, ix;

		a = 0; b = NL;

		if (Log && db)
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

		return (RType) ((v - SLV [ix]) / FAC [ix]) + VLV [ix];
	};

	Boolean exact (RType w) {
	//
	// Checks if the representation occurs exactly as an argument in the
	// table
	//
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
	};

	Boolean inrange (RType w) {
	//
	// Checks if the representation is in range, i.e., between min and
	// max
	//
		if (NL == 1)
			return (w == VLV [0]);

		return (w >= lower () && w <= upper ());
	};

	// Return the limits on the index
	RType lower () { return VLV [0]; };
	RType upper () { return VLV [NL-1]; };
};

typedef	XVMapper<unsigned short>	IVMapper;
typedef	XVMapper<double>			DVMapper;
typedef	XVNdexer<unsigned short>	IVNdexer;
typedef	XVNdexer<double>			DVNdexer;

class MXChannels {

	// Represents the set of available channels together with their
	// separation

	unsigned short NC;	// The number of channels
	double *SEP;
	int NSEP;

	public:

	MXChannels (unsigned short, int, double*);

	inline double ifactor (unsigned short c1, unsigned short c2) {
		trc ("IFACTOR: %1d %1d", c1, c2);

		if (c1 > c2)
			c1 -= c2;
		else
			c1 = c2 - c1;
		if (c1 == 0)
			return 1.0;

		assert (this != NULL,
			"MXChannels: attempt to use different channels while"
			" no channels are defined");

		if (--c1 >= NSEP)
			return 0.0;

		return SEP [c1];
	};

	unsigned short max () { return NC-1; };

	void print ();
};

rfchannel RadioChannel {
/*
 * This is the generic part of a radio channel. It doesn't work by itself.
 *
 */
	const sir_to_ber_t	*STB;	// SIR to BER conversion table
	int					STBL;	// Table length

	double	BNoise;			// Background noise level (linear)

	int	BitsPerByte,		// Symbol length
		PacketFrameLength;	// Extra bits (physical)

	IVMapper	*Rates,		// Selection of xmit rates
				*RBoost,	// Rate-related receiver sensitivities
				*PS,		// Power setter
				*RSSIC;		// RSSI converter

	MXChannels	*Channels;	// Available channels
	DVNdexer	*Modes;		// Available modes (this is optional)

	// 0 - normal, 1 - attenuation, 2 - errors
	FLAGS	RndOff;

	virtual TIME RFC_xmt (RATE, Packet*);

	double ber (double);		// Converts SIR to BER

	// Access to Tag components ===============================================
	inline unsigned short get_x_channel (Transceiver *t) {
		return t->getXTag () & 0xff;
	};
	inline unsigned short get_r_channel (Transceiver *t) {
		return t->getRTag () & 0xff;
	};
	inline unsigned short get_channel (RF_TAG_TYPE t) {
		return t & 0xff;
	};
	inline void set_x_channel (Transceiver *t, unsigned short ch) {
		assert (ch < RF_MAX_N_CHANNELS,
			"set_x_channel: channel number too big");
		t->setXTag ((t->getXTag () & ~0xff) | (RF_TAG_TYPE) ch);
	};
	inline void set_r_channel (Transceiver *t, unsigned short ch) {
		assert (ch < RF_MAX_N_CHANNELS,
			"set_r_channel: channel number too big");
		t->setRTag ((t->getRTag () & ~0xff) | (RF_TAG_TYPE) ch);
	};
	// ==
	inline unsigned short get_x_rindex (Transceiver *t) {
		return (t->getXTag () >> 8) & 0xff;
	};
	inline unsigned short get_r_rindex (Transceiver *t) {
		return (t->getRTag () >> 8) & 0xff;
	};
	inline unsigned short get_rindex (RF_TAG_TYPE t) {
		return (t >> 8) & 0xff;
	};
	inline void set_x_rindex (Transceiver *t, unsigned short ri) {
		assert (ri < RF_MAX_N_RATES, "set_x_rindex: rate index too big");
		t->setXTag ((t->getXTag () & ~0xff00) | (((RF_TAG_TYPE) ri) << 8));
	};
	inline void set_r_rindex (Transceiver *t, unsigned short ri) {
		assert (ri < RF_MAX_N_RATES, "set_r_rindex: rate index too big");
		t->setRTag ((t->getRTag () & ~0xff00) | (((RF_TAG_TYPE) ri) << 8));
	};
	// ==
	inline unsigned short get_cnr (RF_TAG_TYPE t) {
		// Extract channel and rate index; they determine compatibility for
		// reception; the modes are allowed to differ
		return (t & 0x0ffff);
	};
	// ==
	inline unsigned short get_x_mode (Transceiver *t) {
		return (t->getXTag () >> 16) & 0xff;
	};
	inline unsigned short get_r_mode (Transceiver *t) {
		return (t->getRTag () >> 16) & 0xff;
	};
	inline unsigned short get_mode (RF_TAG_TYPE t) {
		return (t >> 16) & 0xff;
	};
	inline void set_x_mode (Transceiver *t, unsigned short mo) {
		assert (mo < RF_MAX_N_MODES, "set_x_mode: mode too big");
		t->setXTag ((t->getXTag () & ~0xff0000) | (((RF_TAG_TYPE) mo) << 16));
	};
	inline void set_r_mode (Transceiver *t, unsigned short mo) {
		assert (mo < RF_MAX_N_MODES, "set_r_mode: mode too big");
		t->setRTag ((t->getRTag () & ~0xff0000) | (((RF_TAG_TYPE) mo) << 16));
	};
	// ========================================================================

	inline double rateBoost (RF_TAG_TYPE tag) {
		// Rate-index-specific boost for BER assessment
		return RBoost == NULL ? 1.0 :
			RBoost->setvalue (get_rindex (tag));
	};

	void setup (
		Long nt,		// The number of transceivers
		double no,		// Background noise
		const sir_to_ber_t *st,	// SIR to BER conversion table
		int    sl,		// Length of the conversion table
		int bpb,		// Bits per byte
		int frm,		// Packet frame (excl. prmbl and chksum)
		IVMapper **ivcc,	// RSSI calculator, power setter, ...
		MXChannels *mxc		// Description of settable channels
	);

	void setBN (double);
	double getBN ();

	FLAGS rndoff (FLAGS f) {
		FLAGS prev = RndOff;
		RndOff = f;
		return prev;
	};

	inline double dgauss (double sm) {
		if (flagSet (RndOff, RND_OFF_ATTENUATION))
			return 0.0;
		return dRndGauss (0.0, sm);
	};

	inline Long lbinomial (double er, Long cn) {
		if (flagSet (RndOff, RND_OFF_ERRORS))
			return 0;
		return lRndBinomial (er, cn);
	};

	inline double dpoisson (double d) {
		if (flagSet (RndOff, RND_OFF_ERRORS))
			return (double) MAX_Long;
		return dRndPoisson (d);
	};

	inline unsigned short stoss (Long n) {
		if (flagSet (RndOff, RND_OFF_CHOICE))
			return 0;
		return (unsigned short) toss (n);
	};

};

extern RadioChannel *Ether;

#endif
