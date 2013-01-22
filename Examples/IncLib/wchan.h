#ifndef	__wchan_h__
#define	__wchan_h__

// General framework for a wireless channel model (version V - used in VUEE)

typedef struct {
	// This structure represents a single entry mapping SIR (Signal to
	// Interference Ratio) to BER (Bit Error Rate). The user specifies
	// a bunch of discrete entries, and the actual values (for SIR in
	// between are interpolated. Does this make sense?
	double	sir, ber, fac;
	// Note: the third value is a precalculated factor to save on
	// multiplication/division
} sir_to_ber_t;

class IVMapper {

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
	unsigned short *VLV;	// Representations
	double *SLV;		// Values
	double *FAC;		// To speed up interpolation

	inline Boolean vtor (double a, double b) {
		// Returns YES if b's representation is larger than a's, i.e.,
		// b is to the "right" of a 
		return (Dec && (a >= b)) || (!Dec && (a <= b));
	}

	public:

	IVMapper (unsigned short, unsigned short*, double*, Boolean l = NO);

	// Converts representation to value
	double setvalue (unsigned short);
	// Converts value to representation
	unsigned short getvalue (double);
	// Checks is the representation is "exact", i.e., it actually is one
	// of the stored discrete representations
	Boolean exact (unsigned short);
	// Checks if the representation is in range, i.e., between min and
	// max
	Boolean inrange (unsigned short);
	// Return the limits on the index
	unsigned short lower () { return VLV [0]; };
	unsigned short upper () { return VLV [NL-1]; };
};

class MXChannels {

	// Represents the set of available channels together with their
	// separation

	unsigned short NC;	// The number of channels
	double *SEP;
	int NSEP;

	public:

	MXChannels (unsigned short, int, double*);

	inline double ifactor (unsigned short c1, unsigned short c2) {

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

	virtual TIME RFC_xmt (RATE, Long);

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
