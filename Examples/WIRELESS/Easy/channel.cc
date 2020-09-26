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

#ifndef __channel_c__
#define __channel_c__

#include "channel.h"

void ShadowingChannel::setup (
		Long nt,		// The number of transceivers
		double rd,		// Reference distance
		double lo,		// Loss at reference distance
		double ex,		// Loss exponent
		double si,		// Sigma
		double no,		// Background noise
		double (*stb) (double),	// SNR to BER converter
		double bu,		// Activity signal threshold
		double co,		// Cut-off signal threshold
		int bpb,		// Bits per byte
		int frm,		// Packet frame length
		Long mpr		// Minimum received preamble length
	) {
		RFChannel::setup (nt);
		BNoise = dBToLin (no);
		Beta = ex;
		RDist = rd;
		Sigma = si;
		LORD = dBToLin (lo);
		AThrs = dBToLin (bu);
		COSL = dBToLin (co);

		BitsPerByte = bpb;
		FrameLength = frm;
		MinPr = mpr;

		ber = stb;
}

double ShadowingChannel::RFC_att (const SLEntry *xp, double d,
					Transceiver *src) {
	// The attenuation formula

	if (d < RDist)
		// Anything less than reference distance is treated as
		// reference distance
		d = RDist;

	// For illustration, this formula is not optimized
	return (xp->Level * dBToLin (dRndGauss (0.0, Sigma))) /
		(LORD * pow (d / RDist, Beta));
}

Boolean ShadowingChannel::RFC_act (double sl, const SLEntry *rs) {

	// Activity sense: received signal multiplied by receiver gain is
	// greater than the threshold

	return sl * rs->Level >= AThrs;
}

TIME ShadowingChannel::RFC_xmt (RATE r, Packet *p) {

	// Transmit time for a packet

	assert ((p->TLength & 0x7) == 0,
		"RFC_xmt: packet length %d not divisible by 8", p->TLength);

	return (TIME) r * (LONG) (((p->TLength) >> 3) * BitsPerByte +
		FrameLength);
}

double ShadowingChannel::RFC_cut (double xp, double rp) {

	// Cutoff threshold: distance over which signal tranmsitted at level
	// xp will be detected at level less than COSL, assuming receiver
	// gain rp

	return pow ((rp * xp)/(COSL * LORD), 1.0 / Beta) * RDist;
}

Boolean ShadowingChannel::RFC_bot (RATE r, const SLEntry *sl, const SLEntry *rs,
							const IHist *h) {

	// BOT trigger: received at least MinPr trailing bits of preamble
	// without a bit error

	return (h->bits (r) >= MinPr) && !error (r, sl, rs, h, -1, MinPr);
}

Boolean ShadowingChannel::RFC_eot (RATE r, const SLEntry *sl, const SLEntry *rs,
							const IHist *h) {

	// EOT trigger: the packet has been followed. If there's been a bit
	// error before EOT, the receiver has detected it via another event
	return TheTransceiver->isFollowed (ThePacket);
}

Long ShadowingChannel::RFC_erb (RATE tr, const SLEntry *sl, const SLEntry *rs,
	double ir, Long nb) {

	// Bit error distribution: independent, based on SIR/BER

	return lRndBinomial (ber ((sl->Level * rs->Level)/(ir + BNoise)), nb);
}

Long ShadowingChannel::RFC_erd (RATE tr, const SLEntry *sl, const SLEntry *rs,
	double ir, Long nb) {

	// Expected waiting time for a bit error (Poisson)

	double er;

	er = dRndPoisson (1.0 / ber ((sl->Level * rs->Level) / (ir + BNoise)));
	return (er > (double) MAX_Long) ? MAX_Long : (Long) er;
}

#endif
