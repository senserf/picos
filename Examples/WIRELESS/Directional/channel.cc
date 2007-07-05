#include "types.h"
#include "channel.h"

// The library channel module
#include "wchansh.cc"

#define	EPSILON	0.00000001	// Almost zero

double angle (double XS, double YS, double XD, double YD) {

// This function calculates the angle from (XS,YS) [source] to
// (XD,YD) [destination], which is between -PI and +PI. [+-]PI means
// West, 0 means East, PI/2 means North, and -PI/2 means South.

	double DX, DY, a;

	DX = XD - XS;
	DY = YD - YS;

	if (fabs (DX) < EPSILON)
		return DY < 0.0 ? -M_PI_2 : M_PI_2;
	else if (fabs (DY) < EPSILON)
		return DX < 0.0 ? M_PI : 0.0;
	else {
		// This is between -M_PI_2 and M_PI_2
		a = atan (DY / DX);
		if (DX < 0.0) {
			if (DY < 0.0)
				return a - M_PI;
			else
				return a + M_PI;
		}
	}
}

static double dir_gain (Transceiver *src, Transceiver *dst) {

// This function is plugged into the shadowing channel model (see create
// RFShadow below) and its value is used as a signal level multiplier in
// RFC_att (the assessment method that describes signal attenuation).
// This simple variant illustrates how to implement directional antennas.

// We assume that the Tag attribute of the sending Transceiver (srs) stores
// the angle of its antenna w.r.t. East (i.e., vector <1, 0>) between PI and
// -PI. The special setting of anything > 5.0 means no directional gain, i.e.,
// the transmission is omnidirectional.

	double XS, YS, XD, YD, Angle, Delta, DA, Dir;

	// Direction at which our antenna is pointing
	Dir = tag_to_angle (src->getTag ());

	if (Dir > 5.0) {
		// Omnidirectional transmission, the gain is 1.0
#if 0
		trace ("GAIN: <%f> = 0 dB (BROADCAST)", Dir);
#endif
		return 1.0;
	}

	// Our coordinates
	src->getLocation (XS, YS);

	// Destination coordinates
	dst->getLocation (XD, YD);

	// Angle to the destination
	Angle = angle (XS, YS, XD, YD);

	// Absolute angle difference between the antenna setting and the
	// destination (modulo PI)
	Delta = adiff (Dir, Angle);

	// Now we need a formula to convert the absolute angular difference
	// into gain. Hopefully, you will find somewhere such formulas (or
	// create your favorite approximation). For now, I am putting here
	// something stupid.

	DA = 20 * ((M_PI - Delta) / M_PI) - 10;

	// Note that for Delta == 0, it gives +10 and for Delta == PI, it gives
	// -10. Delta == 0 means perfect agreement of the angles, and
	// Delta == PI means perfect disagreement. Now we use this value as
	// the gain in dB.

#if TRACE_ANGLES
	trace ("GAIN: F <%f,%f> T <%f,%f>, A %f (%f deg), D %f (%f deg), DELTA "
		"%f (%f deg) = %f dB",
			XS, YS, XD, YD, Angle, (Angle / M_PI) * 180.0,
				Dir, (Dir / M_PI) * 180.0, 
					Delta, (Delta / M_PI) * 180.0,
						DA);
#endif
	// The model needs a linear factor, so we turn dB to linear

	return dBToLin (DA);

}

Long initChannel () {

// Read channel parameters from the input data

	Long NS, BR, BPB, EFB, MPR, STBL;
	double g, psir, pber, BN, Beta, RD, Sigma, LossRD, COFF;
	sir_to_ber_t *STB;
	int i;

	// Relate the numbers read here to the input data set

	// Grid: the granularity of position data.
	readIn (g);

	// Set up the units of time and distance. The external time unit is
	// second, the internal time unit (ITU) is the crossing time of grid
	// unit.
	setEtu (SOL_VACUUM / g);

	// The distance unit (1m), i.e., propagation time across 1m
	setDu (1.0/g);

	// Clock tolerance (keep it at 0.01%)
	setTolerance (0.0001, 2);

	// The number of nodes
	readIn (NS);

	// Background noise in dBm
	readIn (BN);

	// Parameters for the shadowing formula
	// RP(d)/XP [dB] = -10 x 3.0 x log(d/1.0m) + X(1.0) - 38.0

	// This is supposed to be 10 and will be ignored
	readIn (BR);

	// The loss exponent
	readIn (Beta);

	// Reference distance
	readIn (RD);

	// Sigma
	readIn (Sigma);

	// Loss at the reference distance in dB
	readIn (LossRD);

	// Transmission rate
	readIn (BR);

	// Bits per physical byte
	readIn (BPB);

	// Extra framing bits
	readIn (EFB);

	// Minimum received preamble length
	readIn (MPR);

	// BER table length
	readIn (STBL);

	STB = new sir_to_ber_t [STBL];

	for (psir = HUGE, i = 0; i < STBL; i++) {
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

	// This sets SEther
	create RFShadow (NS, STB, STBL, RD, LossRD, Beta, Sigma, BN, BN, COFF,
		MPR, BR, BPB, EFB, dir_gain, NULL, NULL);

	return NS;
}
