#ifndef __rfmodule_h__
#define	__rfmodule_h__

#include "types.h"
#include "channel.h"

#define	BK_LBT	0		// Backoff modes
#define	BK_EOT	1
#define	BK_EOR	2

#define	EVENT_READY		0

process Receiver (Node) {

	states {
		RCV_GETIT,
		RCV_START,
		RCV_RECEIVE,
		RCV_ABORT,
		RCV_RESTART,
		RCV_GOTIT
	};

	perform;
};

process	ADC (Node) {

	// This process calculates the average combined received signal
	// level from the time it is started until it is stopped - to be
	// used for collision avoidance

	double		ATime,		// Accumulated sampling time
			Average,	// Average signal so far
			CLevel;		// Current (last) signal level
	TIME		Last;		// Last sample time

	double sigLevel () {

		// We call this one to collect the average signal level
		// from the process after LBT delay. We just complete the
		// most recent sample (see the perform method in rfomdule.c)
		// and return the average.

		double DT, NA, res;

		DT = (double)(Time - Last);
		NA = ATime + DT;
		res = ((Average * ATime) / NA) + (CLevel * DT) / NA;
		return res;
	};

	states { ADC_WAIT, ADC_RESUME, ADC_UPDATE, ADC_STOP };

	perform;
};

process Xmitter (Node) {

	TIME LBT_delay;
	double	LBT_threshold, MinBackoff, MaxBackoff;

	ADC *RSSI;

	states { XM_LOOP, XM_LBS, XM_TXDONE, XM_RBACK };

	TIME backoff (int);

	perform;

	void setup (double, double, double, double);
};

#endif
