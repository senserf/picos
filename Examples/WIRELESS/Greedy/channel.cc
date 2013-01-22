#include "types.h"
#include "wchansh.cc"

Long BitRate;

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
		MPR, BPB, EFB, ivc, NULL, NULL /* dir_gain*/);

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
