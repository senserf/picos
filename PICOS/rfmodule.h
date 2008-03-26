#ifndef __rfmodule_dm2200_h__
#define	__rfmodule_dm2200_h__

#include "board.h"
#include "rfmattr.h"

process Receiver (PicOSNode) {

	int RBS;			// Receive buffer size

	states { RCV_GETIT, RCV_START, RCV_RECEIVE, RCV_GOTIT };

	byte get_rssi (byte&);

	perform;

	void setup (int s) {
		RBS = s;
	};
};

process	ADC (PicOSNode) {

	double		ATime,		// Accumulated sampling time
			Average,	// Average signal so far
			CLevel;		// Current (last) signal level
	TIME		Last;		// Last sample time

	double sigLevel () {

		double DT, NA, res;

		DT = (double)(Time - Last);
		NA = ATime + DT;
		res = ((Average * ATime) / NA) + (CLevel * DT) / NA;
// diag ("ADC: %d / %g / %g", S->getId (), res, S->lbt_threshold);
		return res;
	};

	states { ADC_WAIT, ADC_RESUME, ADC_UPDATE, ADC_STOP };

	perform;
};

process Xmitter (PicOSNode) {

	int		buflen;
	ADC		*RSSI;

	states { XM_LOOP, XM_TXDONE, XM_LBS };

	perform;

	void setup () {
		zzx_buffer = NULL;
		RSSI = create ADC;
	};
};

#endif
