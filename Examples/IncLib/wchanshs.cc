/*
	Copyright 1995-2018, 2019 Pawel Gburzynski

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

#ifndef	__wchansh_c__
#define	__wchansh_c__

// Shadowing channel model: single channel, single rate (does not use Tags)

#include "wchansh.h"

#include "wchan.cc"

//#define	trc(a, ...)	trace (a, ## __VA_ARGS__)
#ifndef trc
#define	trc(a, ...)
#endif

double RFShadow::RFC_add (int n, int followed, const SLEntry **sl,
	const SLEntry *xmt) {

/* ============================ */
/* Simple additive interference */
/* ============================ */

	double tsl;

	tsl = xmt->Level;

	while (n--)
		if (n != followed)
			tsl += sl [n] -> Level;
	return tsl;
}

double RFShadow::RFC_att (const SLEntry *xp, double d, Transceiver *src) {
/*
 * Attenuation formula according to the shadowing model:
 *
 * rp = ((xp * d0 ^ Beta) / l0) * lin (Xsigma) * d ^ -Beta;
 *
 * where l0 is (linear) loss at the reference distance d0.
 */
	double res;

	if (d < RDist)
		d = RDist;

	res = xp->Level * LFac * dBToLin (dRndGauss (0.0, Sigma)) *
		pow (d, NBeta);

	if (gain)
		res *= gain (src, TheTransceiver);

	trc ("RFC_att (sl) = [%g,%g,%d,%d] -> %g [%g dBm]",
		xp->Level, d, src->getSID (), TheTransceiver->getSID (), res,
			linTodB (res));
	return res;
}

Boolean RFShadow::RFC_act (double sl, const SLEntry *sn) {

/*
 * Note: sn (receiver sensitivity) is a linear multiplier for the
 * received signal. I.e., 0dB means "standard sensitivity" of 1.
 */
	trc ("RFC_act (st) = %g [%g %g] -> %c", sl, sn->Level, BThrs,
		sl * sn->Level > BThrs ? 'Y' : 'N');
	return sl * sn->Level > BThrs;
}

double RFShadow::RFC_cut (double xp, double rp) {

/*
 * The cut-off distance d is the minimum distance at which the attenuated
 * signal (without the Gaussian component) is deemed irrelevant.
 */
	double d;

	d = pow (COSL / (xp * rp * LFac), OBeta);
	trc ("RFC_cut (ds) = %g [%g %g]", d, xp, rp);
	return d;
}

Long RFShadow::RFC_erb (RATE tr, const SLEntry *sl, const SLEntry *rs,
							double ir, Long nb) {
/*
 * Number of error bits in the run of nb bits.
 */
	Long res;

	trc ("RFC_erb (ir) = %g %g %g", ir, rs->Level, BNoise);
	ir = ir * rs->Level + BNoise;

	trc ("RFC_erb (sl) = %g", sl->Level);
	res = (ir == 0.0) ? 0 :
		lRndBinomial (ber ((sl->Level * rs->Level) / ir), nb);

	trc ("RFC_erb (nb) = %1d/%1d", res, nb);
	return res;
}

Long RFShadow::RFC_erd (RATE tr, const SLEntry *sl, const SLEntry *rs,
	double ir, Long nb) {
/*
 * This determines the expected number of bits until the first error bit. We
 * assume (somewhat simplistically) that this event will trigger an illegal
 * symbol and thus abort the reception. Note: if this isn't accurate, you can
 * always adjust the BER to match the real-life behavior.
 */
	double er;
	Long res;

	trc ("RFC_erd (ir) = %g %g %g", ir, rs->Level, BNoise);
	ir = ir * rs->Level + BNoise;

	if (ir == 0.0)
		return MAX_Long;

	er = ber ((sl->Level * rs->Level) / ir);
	trc ("RFC_erd (be) = %g", er);

	er = dRndPoisson (1.0/er);

	res = (er > (double) MAX_Long) ? MAX_Long : (Long) er;
	trc ("RFC_erd (nb) = %1d/%1d", res, nb);
	return res;
}

Boolean RFShadow::RFC_bot (RATE r, const SLEntry *sl, const SLEntry *sn,
	const IHist *h) {
/*
 * BOT assessment, i.e., deciding whether we have tuned to the beginning of
 * packet. We assume that we are fine, if the received preamble has at least
 * so many correctly received bits at the end.
 */

	if (h->bits (r) < MinPr) {
		trc ("RFC_bot (st) = [%f] -> (short)", sl->Level);
		return NO;
	}

	if (error (r, sl, sn, h, -1, MinPr)) {
		trc ("RFC_bot (st) = [%f] -> perr", sl->Level);
		return NO;
	}

	trc ("RFC_bot (st) = [%f] -> OK", sl->Level);
	return YES;
}

Boolean RFShadow::RFC_eot (RATE r, const SLEntry *sl, const SLEntry *sn,
	const IHist *h) {
/*
 * EOT assessment. In this model, EOT is always fine. This is because we are
 * going to abort a reception in progress on the first error bit. Thus, if
 * EOT is ever reached, it is bound to be OK.
 */
	trc ("RFC_eot (st) = %1d", TheTransceiver->isFollowed (ThePacket));
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
	int bpb,		// Bits per byte
	int frm,		// Packet frame (extra physical bits)
	IVMapper **ivcc,	// Value converters
	MXChannels *mxc,
	double (*g) (Transceiver*, Transceiver*)	// Gain function
) {
	RadioChannel::setup (nt, no, st, sl, bpb, frm, ivcc, mxc);

	RDist = rd;
	COSL = (co == -HUGE) ? 0.0 : dBToLin (co);
	Sigma = sg;

	setMinDistance (RDist);

	LFac = pow (RDist, be) / dBToLin (lo);
	NBeta = -be;		// The exponent is negative
	OBeta = 1.0 / -be;
	MinPr = mp;
	BThrs = dBToLin (bu);
	gain = g;

	print (MinPr, "  Minimum preamble:", 10, 26);
	print (bu, "  Activity threshold:", 10, 26);
	print (co, "  Cutoff threshold:", 10, 26);

	print ("  Fading formula:\n");
	print (form ("    RP(d)/XP = -10 x %3.1f x log(d/%3.1fm) + X(%3.1f) - "
		"%4.1fdB\n\n", be, rd, sg, lo));
}

#endif
