#ifndef	__chan_shadow_h__
#define	__chan_shadow_h__

#include "chan_radio.h"

rfchannel RFShadow : RadioChannel {
/*
 * The attenuation formula for Shadowing model:
 *
 *   Pd/Pd0 [dB] = -10 Beta log (d/d0) + Xsigma, where
 *
 *   Pd     is the received power at distance d
 *   Pd0    is the received power at reference distance d0
 *   Beta   is the loss exponent
 *   Xsigma is the lognormal random component
 */

	double NBeta,	// Negative loss exponent (-Beta from the formula)
	       Sigma,	// Gaussian (lognormal) loss component (std deviation)
	       LFac,	// Ref distance ** Beta / loss at ref distance (lin)
	       RDist,	// Reference distance
	       CDist;	// Cut off distance

	Long   MinPr;	// Minimum number of preamble bits

	// Assessment methods
	double RFC_att (double, double, Transceiver*, Transceiver*);
	Boolean RFC_act (double, double);
	Boolean RFC_bot (RATE, double, double, const IHist*);
	Boolean RFC_eot (RATE, double, double, const IHist*);
	double RFC_cut (double, double);
	Long RFC_erb (RATE, double, double, double, Long);
	Long RFC_erd (RATE, double, double, double, Long);

	void setup (

		Long nt,		// The number of transceivers
		const sir_to_ber_t *st,	// SIR to BER conversion table
		int    sl,		// Length of the conversion table
		double rd,		// Reference distance
		double lo,		// Loss at reference distance (dB)
		double be,		// Path loss exponent (Beta)
		double sg,		// Gaussian lognormal component Sigma
		double no,		// Background noise (dBm)
		double co,		// Cut off distance
		Long mp,		// Minimum received preamble length
		Long br,		// Bit rate
		int bpb,		// Bits per byte
		int frm			// Packet frame (excl. prmbl and chksum)
	) {
		RadioChannel::setup (nt, no, st, sl, br, bpb, frm);

		RDist = rd;
		CDist = co;
		Sigma = sg;
		LFac = pow (RDist, be) / dBToLin (lo);
		NBeta = -be;		// The exponent is negative
		MinPr = mp;
	};
};

#define	SEther ((RFShadow*) Ether)

#endif
