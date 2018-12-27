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

#ifndef __channel_h__
#define	__channel_h__

rfchannel ShadowingChannel {

	double	BNoise,	// Background noise level (linear)
	        Beta,	// Loss exponent
	        Sigma,	// Gaussian (lognormal) loss component (std deviation)
	        RDist,	// Reference distance
		LORD,	// Loss at reference distance
	        AThrs,	// Channel activity signal threshold
	        COSL;	// Cut-off signal level

	Long	MinPr;			// Minimum received preamble bits

	int	BitsPerByte,		// Symbol length
		FrameLength;		// Extra bits (physical)

	double (*ber) (double);		// Converts SIR to BER

	void setup (
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
	);

	double RFC_att (const SLEntry*, double, Transceiver*);
	Boolean RFC_act (double, const SLEntry*);
	Boolean RFC_bot (RATE, const SLEntry*, const SLEntry*, const IHist*);
	Boolean RFC_eot (RATE, const SLEntry*, const SLEntry*, const IHist*);
	double RFC_cut (double, double);
	Long RFC_erb (RATE, const SLEntry*, const SLEntry*, double, Long);
	Long RFC_erd (RATE, const SLEntry*, const SLEntry*, double, Long);
	virtual TIME RFC_xmt (RATE, Packet*);
};

#endif
