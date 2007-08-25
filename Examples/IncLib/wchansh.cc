#ifndef	__wchansh_c__
#define	__wchansh_c__

// Shadowing channel model

#include "wchansh.h"

#include "wchan.cc"

#ifndef trc
#define	trc(a, ...)
#endif

// These are the general assessment method that should be OK for most of our
// models

double RFShadow::RFC_att (double xp, double d,
					   Transceiver *src, Transceiver *des) {
/*
 * Attenuation formula according to the shadowing model:
 *
 * rp = ((xp * d0 ^ Beta) / l0) * lin (Xsigma) * d ^ -Beta;
 *
 * where l0 is (linear) loss at the reference distance d0.
 */
	double res = xp;

	if (d < RDist)
		d = RDist;

	res *= LFac * dBToLin (dRndGauss (0.0, Sigma)) * pow (d, NBeta);

	if (gain)
		res *= gain (src, des);

	trc ("RFC_att: [%f,%f,%d,%d] -> %f [%f dBm]",
		xp, d, src->getSID (), des->getSID (), res,
			linTodB (res));
	return res;
}

Boolean RFShadow::RFC_act (double sl, double sn) {

/*
 * Note: sn (receiver sensitivity) is a linear multiplier for the
 * received signal. I.e., 0dB means "standard sensitivity" of 1.
 */
	return sl * sn > BThrs;
}

double RFShadow::RFC_cut (double xp, double rp) {

/*
 * The cut-off distance d is the minimum distance at which the attenuated
 * signal (without the Gaussian component) bumped by CUTOFF_MARGIN dB is
 * below the background noise level.
 */

	double res = pow (COSL / (xp * rp * LFac), OBeta);

	return res;
}

Long RFShadow::RFC_erb (RATE tr, double sl, double rs, double ir, Long nb) {
/*
 * Number of error bits in the run of nb bits.
 */
	Long res = lRndBinomial (ber (sl/(ir + BNoise)), nb);
	trc ("RFC_erb: [%f,%f,%1d] -> %1d", sl, ir, nb, res);
	return res;
}

Long RFShadow::RFC_erd (RATE tr, double sl, double rs, double ir, Long nb) {
/*
 * This determines the expected number of bits until the first error bit. We
 * assume (somewhat simplistically) that this event will trigger an illegal
 * symbol and thus abort the reception. Note: if this isn't accurate, you can
 * always adjust the BER to match the real-life behavior.
 */
	double er;
	Long res;

	er = ber ((sl * rs) / (ir + BNoise));
	er = dRndPoisson (1.0/er);
	res = (er > (double) MAX_Long) ? MAX_Long : (Long) er;
	trc ("RFC_erd: [%g,%g,%1d] -> %1d", sl, ir, nb, res);
	return res;
}

Boolean RFShadow::RFC_bot (RATE r, double sl, double sn, const IHist *h) {
/*
 * BOT assessment, i.e., deciding whether we have tuned to the beginning of
 * packet. We assume that we are fine, if the received preamble has at least
 * so many correctly received bits at the end.
 */

	if (h->bits (r) < MinPr) {
		trc ("RFC_bot: [%f] -> 0 (short)", sl);
		return NO;
	}

	if (error (r, sl, sn, h, -1, MinPr)) {
		trc ("RFC_bot: [%f] -> 0", sl);
		return NO;
	}

	trc ("RFC_bot: [%f] -> 1", sl);
	return YES;
}

Boolean RFShadow::RFC_eot (RATE r, double sl, double sn, const IHist *h) {
/*
 * EOT assessment. In this model, EOT is always fine. This is because we are
 * going to abort a reception in progress on the first error bit. Thus, if
 * EOT is ever reached, it is bound to be OK.
 */
	return TheTransceiver->isFollowed (ThePacket);
}

void RFShadow::setup (

	Long nt,		// The number of transceivers
	const sir_to_ber_t *st,	// SIR to BER conversion table
	int    sl,		// Length of the conversion table
	double rd,		// Reference distance
	double lo,		// Loss at reference distance (dB)
	double be,		// Path loss exponent (Beta)
	double sg,		// Gaussian lognormal component Sigma
	double no,		// Background noise (dBm)
	double bu,		// Channel busy signal threshold dBm
	double co,		// Cut off signal level
	Long mp,		// Minimum received preamble length
	Long br,		// Bit rate
	int bpb,		// Bits per byte
	int frm,		// Packet frame (extra physical bits)
	double (*g) (Transceiver*, Transceiver*),	// Gain function
	RSSICalc *rsc,		// RSSI calculator
	PowerSetter *ps		// Power setting converter
) {
	RadioChannel::setup (nt, no, st, sl, br, bpb, frm, rsc, ps);

	RDist = rd;
	COSL = (co == -HUGE) ? 0.0 : dBToLin (co);
	Sigma = sg;

	LFac = pow (RDist, be) / dBToLin (lo);
	NBeta = -be;		// The exponent is negative
	OBeta = 1.0 / -be;
	MinPr = mp;
	BThrs = dBToLin (bu);
	gain = g;

	print (MinPr, "  Minimum preamble:", 10, 26);
	print (bu, "  Activity threshold:", 10, 26);
	print (co, "  Cutoff threshold:", 10, 26);

	print ("  Fading fromula:\n");
	print (form ("    RP(d)/XP = -10 x %3.1f x log(d/%3.1fm) + X(%3.1f) - "
		"%4.1fdB\n\n", be, rd, sg, lo));
}

#endif
