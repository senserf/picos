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

#ifndef __rf_h__
#define	__rf_h__

// #define	NEUTRINO
// #define	SIRDEBUG

class SIRtoBER {

	double	*Sir, *Ber;
	int NEntries;

	public:

	SIRtoBER () {
		int i;
		readIn (NEntries);
		Sir = new double [NEntries];
		Ber = new double [NEntries];
		for (i = 0; i < NEntries; i++) {
			readIn (Sir [i]);
			readIn (Ber [i]);
		}
	};

	double ber (double sir) const {
		int i;
		double res;
		for (i = 0; i < NEntries; i++)
			if (Sir [i] <= sir)
				break;
		// trace ("SIR %1d", i);
		if (i == 0)
			res = Ber [0];
		else if (i == NEntries)
			res = Ber [NEntries - 1];
		else
			res = Ber [i-1] +
		    ((Sir [i-1] - sir) / (Sir [i-1] - Sir [i])) *
			(Ber [i] - Ber [i-1]);

		// trace ("BER: %g -> %g", sir, res);
		return res;
	};
};

rfchannel ALOHARF {

	double BNoise, AThrs;
	int MinPr;

	const SIRtoBER *StB;

	double pathloss_db (double d) {
		if (d < 1.0)
			d = 1.0;
		return -27.0 * log10 ((4.0*3.14159265358979323846/0.75) * d);
	};
		
	TIME RFC_xmt (RATE r, Packet *p) {
		return (TIME) r * p->TLength;
	};

	double RFC_att (const SLEntry *xp, double d, Transceiver *src) {

#ifdef	SIRDEBUG
		double pl, sl;
		pl = pathloss_db (d);
		sl = xp->Level * dBToLin (pl);
		trace ("DIST %g, loss %g, xp %g, level %g", d, pl, xp->Level,
			sl);
		return sl;
#endif
		return xp->Level
#ifndef	NEUTRINO
			* dBToLin (pathloss_db (d))
#endif
		;
	};

	Boolean RFC_act (double sl, const SLEntry *sn) {
		// Not needed, if not used
#ifdef	NEUTRINO
		return sn->Level > 0.0;
#else
		return sl * sn->Level > AThrs;
#endif
	};

	Long RFC_erb (RATE r, const SLEntry *sl, const SLEntry *rs,

		double ir, Long nb) {
#ifdef	SIRDEBUG
		double sir;
		ir = ir * rs->Level + BNoise;
		sir = linTodB ((sl->Level*rs->Level)/ir);
		trace ("SIR %g, sen %g, sig %g, int %g", sir, rs->Level,
			sl->Level, ir);
		return (ir == 0.0) ? 0 : lRndBinomial (StB->ber (sir), nb);
#endif

#ifdef	NEUTRINO
		return (ir > 0.0) ? nb : 0;
#else
		ir = ir * rs->Level + BNoise;

		return (ir == 0.0) ? 0 :
		 lRndBinomial (StB->ber (linTodB ((sl->Level*rs->Level)/ir)),
		  nb);
#endif
	};

	Boolean RFC_bot (RATE r, const SLEntry *sl, const SLEntry *sn,
		const IHist *h) {

		return (h->bits (r) >= MinPr) &&
			!error (r, sl, sn, h, -1, MinPr);
	};

	Boolean RFC_eot (RATE r, const SLEntry *sl, const SLEntry *sn,
		const IHist *h) {

		return TheTransceiver->isFollowed (ThePacket) &&
			!error (r, sl, sn, h);
	};

	void setup (	Long nt,	// The number of transceivers
			double r,	// Rate in bps
			double xp,	// Default transmit power (dBm)
			double no,	// Background noise (dBm)
			double at,	// Activity threshold (dBm)
			int pr,		// Minimum receivable preamble length
			const SIRtoBER *sb) {

		RFChannel::setup (nt, (RATE)(Etu/r), PREAMBLE_LENGTH,
			dBToLin (xp), 1.0);
		BNoise = dBToLin (no);
		AThrs = dBToLin (at);
		StB = sb;
		MinPr = pr;
	};
};

#endif
