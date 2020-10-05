/*
	Copyright 1995-2020 Pawel Gburzynski

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

#include "types.h"
#include "wchanshs.cc"

Long BitRate;

static double 	*XGT = NULL,		// Xmit antenna gain table
		*RGT = NULL;		// Receive antenna gain table

double		NXGT, NRGT;		// size / PI

// Sector size as a fraction of 2 PI
double		SXSC, SRSC;

static inline double gain_calc (double ant, double ang, double *gt, double N) {
//
// Interpolates gain from the table
//
	if (gt == NULL)
		// No directional gain for this part
		return 1.0;

#if TRACE_ANGLES
	trace ("GAIN CALC [%s]: ang = %f, ant = %f, dif = %f, gain = %fdB",
	gt == XGT ? "XMT" : "RCV", ang, ant, adiff (ang, ant),
		linTodB (gt [(int)(adiff (ang, ant) * N)]));
#endif
	return gt [ (int) (adiff (ang, ant) * N) ];
}

static double dir_gain (Transceiver *src, Transceiver *dst) {
//
// This is the gain calculation function; we assume that the gain is determined
// by interpolation from a discrete table
//
	double RA, XA, XS, YS, XD, YD, ang, gai;

	// Source antenna setting
	XA = ((MY_RFModule*) (src->getXTag ())) -> getXAnt ();
	// Destination antenna setting
	RA = ((MY_RFModule*) (dst->getXTag ())) -> getRAnt ();

	if (XA < 0.0 && RA < 0.0) {
		// Get the trivial case out of the way
#if TRACE_ANGLES
		trace ("DIR GAIN: %s -> %s [both omni]: 0dB",
			src->getSName (), dst->getSName ());
#endif
		return 1.0;
	}

	// Calculate the transmission angle: this is the angle "from" src "to"
	// dest
	src->getLocation (XS, YS);
	dst->getLocation (XD, YD);

	ang = angle (XS, YS, XD, YD);
	gai = (XA < 0.0) ? 1.0 : gain_calc (XA,  ang, XGT, NXGT);
	if (RA >= 0.0) {
		if ((ang += M_PI) > M_PI + M_PI)
			ang -= M_PI + M_PI;
		gai *= gain_calc (RA, ang, RGT, NRGT);
	}

#if TRACE_ANGLES
	trace ("DIR GAIN %s [%f,%f] -> %s [%f,%f]: %fdB", src->getSName (),
		XS, YS, dst->getSName (), XD, YD, linTodB (gai));
#endif
	return gai;
}

static void initGainTable (double &SSC, double *&gt, double &N,
							     const char *hdr) {
	double F, T, D;
	int NL, n, i;

	// Number of discrete setting
	readIn (NL);
	Assert (NL >= 0, "initGainTable [%s]: negative number of setting %1d",
		hdr, NL);

	if (NL == 0) {
		// Look no further
		gt = NULL;
		N = 0.0;
		return;
	}

	// Sector size
	SSC = (M_PI + M_PI) / NL;

	// The size
	readIn (n);

	Assert (n > 1 && n <= 4096, "initGainTable [%s]: illegal size %1d, "
		"must be 2 <= size <= 4096", hdr, n);

	print (form ("Antenna gain table [%s], %1d settings, GT size %1d:\n\n",
		hdr, NL, n));

	gt = new double [n + 1];	// Include a sentinel
	N = (double) n / M_PI;

	T = 0.0;

	for (i = 0; i < n; i++) {
		readIn (D);
		F = T;
		T = (i + 1) / N;
		print (form ("  [%5.3f, %5.3f] : %6.1fdB\n", F, T, D));
		gt [i] = dBToLin (D);
	}
	print ("\n");

	// The sentinel
	gt [n] = gt [n - 1];
}

static void initAGain () {

// Initializes the gain tables

	initGainTable (SXSC, XGT, NXGT, "transmit");
	initGainTable (SRSC, RGT, NRGT, "receive");
}

void MY_RFModule::setXAntOmni () {

	X_Ant = -1.0;
#if TRACE_ANGLES
	trace ("SET X ANT %s OMNI", MyNode->getSName ());
#endif
}

void MY_RFModule::setRAntOmni () {

	R_Ant = -1.0;
#if TRACE_ANGLES
	trace ("SET R ANT %s OMNI", MyNode->getSName ());
#endif
}

void MY_RFModule::setXAnt (Long nd) {

	double mx, my, x, y, ang;
	int sec;

	// Destination location
	if (MyNode->Neighbors->getCoords (nd, x, y) == NO) {
		// Neighbor not found, use omni
#if TRACE_ANGLES
		trace ("SET X ANT, %1d neighbor not found, using omni");
#endif
		setXAntOmni ();
	}
	// Your own location
	MyNode->RFI->getLocation (mx, my);
	// Angle towards <x,y>
	ang = angle (mx, my, x, y);

	// Calculate the sector; Note: we may want to randomize the
	// sector origin on a per-node basis, as now we assume that
	// sector zero is aligned exactly at East
	sec = (int) (ang / SXSC);

	// Point it to the center of the sector
	X_Ant = ((double) sec + 0.5) * SXSC;

#if TRACE_ANGLES
	trace ("SET X ANT %s [%f,%f] -> %d [%f,%f] (%f) -> %1d (%f)",
		MyNode->getSName (), mx, my, nd, x, y, ang, sec, X_Ant);
#endif
}

void MY_RFModule::setRAnt (Long nd) {

	double mx, my, x, y, ang;
	int sec;

	// Sender location
	if (MyNode->Neighbors->getCoords (nd, x, y) == NO) {
		// Neighbor not found, use omni
#if TRACE_ANGLES
		trace ("SET R ANT, %1d neighbor not found, using omni");
#endif
		setRAntOmni ();
	}
	// Your own location
	MyNode->RFI->getLocation (mx, my);
	// Angle towards <x,y>
	ang = angle (mx, my, x, y);

	// Calculate the sector; Note: we may want to randomize the
	// sector origin on a per-node basis, as now we assume that
	// sector zero is aligned exactly at East
	sec = (int) (ang / SRSC);

	// Point it to the center of the sector
	R_Ant = ((double) sec + 0.5) * SRSC;

#if TRACE_ANGLES
	trace ("SET R ANT %s [%f,%f] -> %d [%f,%f] (%f) -> %1d (%f)",
		MyNode->getSName (), mx, my, nd, x, y, ang, sec, R_Ant);
#endif
}

void initChannel (Long &NS, Long &PRE) {

// Read channel parameters from the input data

	Long SDT, BPB, EFB, MPR, STBL;
	double g, psir, pber, BN, AL, Beta, RD, Sigma, LossRD, COFF;
	sir_to_ber_t *STB;
	int i;
	IVMapper *ivc [4];
	unsigned short rix;

	// Relate the numbers read here to the input data set

	// Grid: the granularity of position data.
	readIn (g);

	// Set up the units of time and distance. The external time unit is
	// second, the internal time unit (ITU) is the crossing time of grid
	// unit.
	setEtu (SOL_VACUUM / g);

	// The distance unit (1m), i.e., propagation time across 1m
	setDu (1.0/g);

	// Clock tolerance (0.01%)
	setTolerance (0.0001, 2);

	// The number of nodes
	readIn (NS);

	// Background noise in dBm
	readIn (BN);

	// Parameters for the shadowing formula
	// RP(d)/XP [dB] = -10 x 3.0 x log(d/1.0m) + X(1.0) - 38.0

	// This is supposed to be 10 and will be ignored
	readIn (BitRate);

	// The loss exponent
	readIn (Beta);

	// Reference distance
	readIn (RD);

	// Sigma
	readIn (Sigma);

	// Loss at the reference distance in dB
	readIn (LossRD);

	// Transmission rate
	readIn (BitRate);

	// Bits per physical byte
	readIn (BPB);

	// Physical preamble length
	readIn (PRE);

	// Extra framing bits
	readIn (EFB);

	// Minimum received preamble length
	readIn (MPR);

	// BER table length
	readIn (STBL);

	STB = new sir_to_ber_t [STBL];

	for (psir = HUGE, pber = -1.0, i = 0; i < STBL; i++) {
		readIn (g);			// SIR in dB
		STB [i] . sir = dBToLin (g);	// Convert to linear
		readIn (STB [i] . ber);		// BER
		// Validate
		if (STB [i] . sir >= psir)
			excptn ("SIR entries in STB must be "
				"monotonically decreasing, %f and %f aren't",
					psir, STB [i] . sir);
		psir = STB [i] . sir;
		if (STB [i] . ber < 0)
			excptn ("BER entries in STB must not be "
				"negative, %f is", STB [i] . ber);
		if (STB [i] . ber <= pber)
			excptn ("BER entries in STB must be "
				"monotonically increasing, %f and %f aren't",
					pber, STB [i] . ber);
		pber = STB [i] . ber;
	}

	// Cutoff threshold in dBm
	readIn (COFF);

	// Activity level at receiver gain 0dB - to tell the channel is busy
	readIn (AL);

	rix = 0;
	g = (double) BitRate;
	ivc [0] = new IVMapper (1, &rix, &g);
	ivc [1] = ivc [2] = ivc [3] = NULL;

	create RFShadow (NS, STB, STBL, RD, LossRD, Beta, Sigma, BN, AL, COFF,
		MPR, BPB, EFB, ivc, NULL, dir_gain);

	// Directionality
	initAGain ();

	// Initialize global parameters of the DCF scheme

	readIn (g);	// SIFS
	readIn (psir);	// SLOT
	readIn (SDT);	// Short data threshold
	readIn (BPB);	// Short retransmission limit
	readIn (EFB);	// Long retransmission limit
	readIn (MPR);	// CW min
	readIn (STBL);	// CW max

	initDCF (g, psir, SDT, (int)BPB, (int)EFB, (int)MPR, (int)STBL);

}
