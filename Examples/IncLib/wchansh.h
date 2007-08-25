#ifndef	__wchansh_h__
#define	__wchansh_h__

// Shadowing channel model

#include "wchan.h"

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
	       OBeta,	// 1.0/NBeta
	       Sigma,	// Gaussian (lognormal) loss component (std deviation)
	       LFac,	// Ref distance ** Beta / loss at ref distance (lin)
	       RDist,	// Reference distance
	       BThrs,	// Channel busy signal threshold
	       COSL;	// Cut-off signal level

	Long   MinPr;	// Minimum number of preamble bits

	double (*gain) (Transceiver*, Transceiver*);

	// Assessment methods
	double RFC_att (double, double, Transceiver*, Transceiver*);
	Boolean RFC_act (double, double);
	Boolean RFC_bot (RATE, double, double, const IHist*);
	Boolean RFC_eot (RATE, double, double, const IHist*);
	double RFC_cut (double, double);
	Long RFC_erb (RATE, double, double, double, Long);
	Long RFC_erd (RATE, double, double, double, Long);

	void setup (
		Long,			// The number of transceivers
		const sir_to_ber_t*,	// SIR to BER conversion table
		int,			// Length of the conversion table
		double,			// Reference distance
		double,			// Loss at reference distance (dB)
		double,			// Path loss exponent (Beta)
		double,			// Gaussian lognormal component Sigma
		double,			// Background noise (dBm)
		double,			// Channel busy signal threshold dBm
		double,			// Cut off signal level
		Long,			// Minimum received preamble length
		Long,			// Bit rate
		int,			// Bits per byte
		int,			// Packet frame (extra physical bits)
		double (*g) (Transceiver*, Transceiver*) = NULL,
		RSSICalc *rsc = NULL,	// RSSI calculator
		PowerSetter *ps = NULL	// Power setting converter
	);
};

#define	SEther ((RFShadow*) Ether)

#endif
