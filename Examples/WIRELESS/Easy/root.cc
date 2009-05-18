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
double MinBackoff, MaxBackoff;
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

	// Grid: the granularity of position data.
	readIn (g);
	print (g,		" Distance granularity:", 10, 26);

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
	print (NS,		" Number of nodes:", 10, 26);

	// Background noise in dBm
	readIn (BN);
	print (BN,		" Background noise:", 10, 26);

	// Parameters for the shadowing formula
	// RP(d)/XP [dB] = -10 x 3.0 x log(d/1.0m) + X(1.0) - 38.0

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

	print ("Parameters shared by all nodes:\n\n");

	readIn (n);		// Transmission rate
	// Number of ITUs in one second
	d = (double) etuToItu (1.0);
	// Number of ITUs per bit
	XmitRate = (RATE) round (d / n);
	print (n,		" Transmission rate:", 10, 26);

	readIn (Preamble);
	print (Preamble,	" Preamble length:", 10, 26);

	readIn (XmitPower);		// Transmission power
	print (XmitPower,	"  Transmission power:", 10, 26);

	readIn (MinBackoff);		// Minimum backoff
	readIn (MaxBackoff);		// Maximum backoff

	print (MinBackoff,	"  Minimum backoff:", 10, 26);
	print (MaxBackoff,  	"  Maximum backoff:", 10, 26);

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
