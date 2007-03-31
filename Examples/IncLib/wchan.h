#ifndef	__wchan_h__
#define	__wchan_h__

// General framework for a wireless channel model (version V - used in VUEE)

typedef struct {
	// This structure represents a single entry mapping SIR (Signal to
	// Interference Ratio) to BER (Bit Error Rate). The user specifies
	// a bunch of discrete entries, and the actual values (for SIR in
	// between are interpolated. Does this make sense? Beats me!
	double	sir, ber, fac;
	// Note: the third value is a precalculated factor to save on
	// multiplication/division
} sir_to_ber_t;

class RSSICalc {

	// This is the RSSI calculator - in case the RF driver wants to return
	// RSSI indication to the application

	unsigned short NL;	// Number of levels
	unsigned short Mode;	// How to calculate
	unsigned short *VLV;	// Value levels
	double *SLV;		// Signal levels
	double *FAC;

	// For transformation:
	//
	// VLV [0] minimum returned value
	// VLV [1] maximum returned value
	// SLV [0] minimum signal level Smin
	// SLV [1] (Vmax - Vmin) / (Smax - Smin)
	// SLV [2] Vmax - Vmin
	//
	// For interpolation:
	// 
	// VLV - values
	// SLV - signal levels (strictly increasing)
	// FAC [i] = (VLV [i+1] - VLV [i]) / (SLV [i+1] - SLV [i])
	// 
	//

	public:

	RSSICalc (unsigned short, unsigned short, unsigned short*, double*);

	unsigned short calculate (double);
};

class PowerSetter {

	// This is the vehicle for setting the transmission power, e.g.,
	// according to a number of discretized levels implanted into the
	// module. This, as well as RSSICalc, are primarily useful for
	// PICOS (VUEE) models, but they are not unlikely to be of a more
	// general interest.

	unsigned short NL;	// Number of levels
	unsigned short Mode;	// How to calculate
	unsigned short *VLV;	// Value levels
	double *SLV;		// Signal levels
	double *FAC;

	// For transformation:
	//
	// VLV [0] minimum argument value
	// VLV [1] maximum argument value
	// SLV [0] minimum signal level
	// SLV [1] maximum signal level
	// SLV [2] SLV [1] - SLV [2]
	// SLV [3] (SLV [1] - SLV [0]) / (VLV [1] - VLV [0])
	// SLV [4] VLV [1] - VLV [0]

	public:

	PowerSetter (unsigned short, unsigned short, unsigned short*, double*);

	double setvalue (unsigned short);
	unsigned short getvalue (double);
};

rfchannel RadioChannel {
/*
 * This is the generic part of a radio channel. It doesn't work by itself.
 *
 */
	const sir_to_ber_t	*STB;	// SIR to BER conversion table
	int			STBL;	// Table length

	double	BNoise;			// Background noise level (linear)

	Long	BitRate;		// Bits per second
	int	BitsPerByte,		// Symbol length
		PacketFrameLength;	// Extra bits (physical)

	RSSICalc *RSSIC;
	PowerSetter *PS;

	virtual TIME RFC_xmt (RATE, Long);

	double ber (double);		// Converts SIR to BER

	void setup (
		Long nt,		// The number of transceivers
		double no,		// Background noise
		const sir_to_ber_t *st,	// SIR to BER conversion table
		int    sl,		// Length of the conversion table
		Long br,		// Bit rate
		int bpb,		// Bits per byte
		int frm,		// Packet frame (excl. prmbl and chksum)
		RSSICalc *rssic,	// RSSI calculator
		PowerSetter *ps		// Power setting converter (SETPOWER)
	);
};

extern RadioChannel *Ether;

#endif
