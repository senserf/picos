#ifndef	__wchan_h__
#define	__wchan_h__

// =========================================================================
// General framework for a wireless channel model (version V - used in VUEE)
// =========================================================================

// #define	trc(a, ...)	trace (a, ## __VA_ARGS__)

#ifndef trc
#define	trc(a, ...)
#endif

// Standard indexes into the XVMapper table
#define	XVMAP_RATES	0
#define	XVMAP_RBOOST	1
#define	XVMAP_RSSI	2
#define	XVMAP_PS	3

typedef struct {
	// This structure represents a single entry mapping SIR (Signal to
	// Interference Ratio) to BER (Bit Error Rate). The user specifies
	// a bunch of discrete entries, and the actual values (for SIR in
	// between are interpolated. Does this make sense?
	double	sir, ber, fac;
	// Note: the third value is a precalculated factor to save on
	// multiplication/division
} sir_to_ber_t;

template <class RType> class XVMapper {

	// This is a generic converter between model parameters (like signal
	// levels or bit rates) and their representations for the praxis.
	// For example, there may be 8 discrete power level setting
	// represented by numbers from 0 to 7 that should be mapped into
	// FP multipliers. The converter is able to do it both ways, i.e.,
	// convert discrete representations to the actual values, as well
	// as convert the values to their discrete representations.

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
typedef	XVMapper<double>		DVMapper;

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
	int			STBL;	// Table length

	double	BNoise;			// Background noise level (linear)

	int	BitsPerByte,		// Symbol length
		PacketFrameLength;	// Extra bits (physical)

	IVMapper	*Rates,		// Selection of xmit rates
			*RBoost,	// Rate-related receiver sensitivities
			*PS,		// Power setter
			*RSSIC;		// RSSI converter

	MXChannels	*Channels;	// Available channels

	virtual TIME RFC_xmt (RATE, Packet*);

	double ber (double);		// Converts SIR to BER

	inline unsigned short tagToCh (IPointer tag) {
		// Tag to channel number
		return (unsigned short) (tag & 0xffff);
	};

	inline unsigned short tagToRI (IPointer tag) {
		// Tag to rate index
		return (unsigned short) ((tag >> 16) & 0xffff);
	};

	inline double rateBoost (IPointer tag) {
		// Rate-index-specific boost for BER assessment
		return RBoost == NULL ? 1.0 :
			RBoost->setvalue ((unsigned short) (tag >> 16));
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
};

extern RadioChannel *Ether;

#endif
