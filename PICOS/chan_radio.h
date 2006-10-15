#ifndef	__chan_radio_h__
#define	__chan_radio_h__

typedef struct {
	// This structure represents a single entry mapping SIR (Signal to
	// Interference Ratio) to BER (Bit Error Rate). The user specifies
	// a bunch of discrete entries, and the actual values (for SIR in
	// between are interpolated. Does this make sense? Beats me!
	double	sir, ber;
} sir_to_ber_t;

rfchannel RadioChannel {
/*
 * This is the common part of all Radio channel used in our PicOS models
 *
 */
	const sir_to_ber_t	*STB;	// SIR to BER conversion table
	int			STBL;	// Table length

	double	BNoise;			// Background noise level (linear)

	Long	BitRate;		// Bits per second
	int	BitsPerByte,		// Symbol length
		PacketFrameLength;	// Excluding preamble and checksum!!!

	double ber (double);		// Converts SIR to BER

	void setup (
		Long nt,		// The number of transceivers
		double no,		// Background noise
		const sir_to_ber_t *st,	// SIR to BER conversion table
		int    sl,		// Length of the conversion table
		Long br,		// Bit rate
		int bpb,		// Bits per byte
		int frm			// Packet frame (excl. prmbl and chksum)
	);
};

extern RadioChannel *Ether;

#endif
