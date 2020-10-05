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
#include "channel.h"

identify (Very Simple Wireless Network);

process Root {

	Long initChannel ();
	void initNodes (Long);
	void initTraffic ();

	states { Start, Stop };

	perform;
};

// Global parameters
double MinBackoff, MaxBackoff, PSpace;
RATE XmitRate;
Long Preamble;
double XmitPower;

typedef struct {
	// SIR to BER table entry
	double SIR, BER;
} STB_e;

static int NSTB;
static STB_e *STBT;

static double berfun (double sir) {

	int i;

	for (i = 0; i < NSTB-1; i++)
		if (sir >= STBT [i] . SIR)
			break;

	return STBT [i] . BER;
}

Long Root::initChannel () {

	Long NS, BPB, EFB, MPR;
	double g, d, BN, AL, Beta, RD, Sigma, LossRD, COFF;
	int i;

	print ("Parameters of shadowing channel:\n\n");

	// Grid: the granularity of position data in meters (because it is
	// used to divide SOL_VACUUM (which is in meters per second).
	readIn (g);
	print (g,		" Distance granularity:", 10, 26);

	// Set up the units of time and distance.
	// SOL_VACUUM is in m/s. So 1 ITU (the time grain) is set to the
	// fraction of a second needed to cross the grid distance (assuming the
	// latter is in meters). This implies that 1 ETU = 1 second.
	setItu (g/SOL_VACUUM);

	// The distance unit expressed as propagation time, i.e., how many ITUs
	// are needed to cover 1 m (so we express distance in meters).
	setDu (1.0/g);

	// Clock tolerance (0.01%)
	setTolerance (0.0001, 2);

	// The number of nodes
	readIn (NS);
	print (NS,		" Number of nodes:", 10, 26);

	// Background noise in dBm for the channel model
	readIn (BN);
	print (BN,		" Background noise:", 10, 26);

	// Parameters for the shadowing formula
	// RP(d)/XP [dB] = -10 x Beta x log(d/RD) + X(Sigma) - LossRD

	// This is supposed to be 10 and will be ignored
	readIn (Beta);

	// The loss exponent
	readIn (Beta);
	print (Beta,		" Loss exponent:", 10, 26);

	// Reference distance
	readIn (RD);
	print (RD,		" Reference distance:", 10, 26);

	// Sigma
	readIn (Sigma);
	print (Sigma,		" Gaussian sigma:", 10, 26);

	// Loss at the reference distance in dB
	readIn (LossRD);
	print (LossRD,		" Loss base:", 10, 26);

	// Bits per physical byte
	readIn (BPB);
	print (BPB,		" Bits per byte:", 10, 26);

	// Extra framing bits
	readIn (EFB);
	print (EFB,		" Extra framing bits:", 10, 26);

	// Minimum received preamble length
	readIn (MPR);
	print (MPR,		" Minimum preamble:", 10, 26);

	// BER table length
	readIn (NSTB);
	print (NSTB,		" Ber table length:", 10, 26);

	STBT = new STB_e [NSTB];

	for (i = 0; i < NSTB; i++) {
		readIn (d);
		// Specified in dB, stored as a regular fraction
		STBT [i] . SIR = dBToLin (d);
		readIn (STBT [i] . BER);
		print (form ("     %8.3fdB -> %8g\n", d, STBT [i]. BER));
	}
	print ("\n");

	// Cutoff threshold in dBm
	readIn (COFF);
	print (COFF,		" Cut-off threshold:", 10, 26);

	// Activity level at receiver gain 0dB - to tell the channel is busy
	readIn (AL);
	print (AL,		" Activity threshold:", 10, 26);

	create ShadowingChannel (NS, RD, LossRD, Beta, Sigma, BN, berfun, AL,
		COFF, BPB, EFB, MPR);

	return NS;
}

void Root::initNodes (Long N) {

	double d, x, y;
	Long n;

	print ("\nParameters shared by all nodes:\n\n");

	// Transmission rate in bps
	readIn (n);

	// Convert to ITUs per bit (Etu = number of ITUs in 1 second)
	XmitRate = (RATE) round (Etu / n);

	print (n,		" Transmission rate:", 10, 26);
	print (XmitRate,	" ITUs per bit:", 10, 26);

	readIn (Preamble);
	print (Preamble,	" Preamble length:", 10, 26);

	readIn (XmitPower);		// Transmission power
	print (XmitPower,	"  Transmission power:", 10, 26);
	// Convert to linear (assuming log [dBm] on input)
	XmitPower = dBToLin (XmitPower);
	print (XmitPower,	"  As absolute value:", 10, 26);

	readIn (MinBackoff);		// Minimum backoff
	readIn (MaxBackoff);		// Maximum backoff
	readIn (PSpace);		// Packet space after xmit

	print (MinBackoff,	"  Minimum backoff:", 10, 26);
	print (MaxBackoff,  	"  Maximum backoff:", 10, 26);
	print (PSpace,  	"  Packet space:", 10, 26);

	for (n = 0; n < N; n++) {
		readIn (x);
		readIn (y);
		print (form ("  Node %4d at <%7.3f,%7.3f>\n", n, x, y));
		create WirelessNode (x, y);
		create (n) Transmitter;
		create (n) Receiver;
	}

	print ("\n");
}

void Root::initTraffic () {

	double MinML, MaxML, MINT;
	Traffic *U;

	// This is a very simple uniform traffic pattern with uniformly
	// distributed message (packet) length and exponential inter-arrival
	// time.

	readIn (MinML);		// Minumum message length in bytes
	readIn (MaxML);		// Maximum message length in bytes
	readIn (MINT);		// Mean interarrival time in seconds

	print ("Traffic parameters:\n\n");
	print (MinML, "  Minimum message length:", 10, 26);
	print (MaxML, "  Maximum message length:", 10, 26);
	print (MINT,  "  Mean interarrival time:", 10, 26);
	print ("\n");

	//                                          ... must be in bits ... 
	U = create Traffic (MIT_exp + MLE_unf, MINT, MinML * 8, MaxML * 8);

	// All node are senders
	U->addSender (ALL);
	// ... and receivers
	U->addReceiver (ALL);
}

Root::perform {

	state Start:

		double TimeLimit;

		settraceFlags (
			TRACE_OPTION_TIME +
			TRACE_OPTION_ETIME +
			TRACE_OPTION_STATID +
			TRACE_OPTION_STATE
		);

		initNodes (initChannel ());
		initTraffic ();
			
		// Reset after inits
		TheStation = System;

		readIn (TimeLimit);
		setLimit (0, etuToItu (TimeLimit));
		Kernel->wait (DEATH, Stop);

	state Stop:

		Client->printPfm ();
		terminate;
}
