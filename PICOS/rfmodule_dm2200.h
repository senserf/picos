#ifndef __rfmodule_dm2200_h__
#define	__rfmodule_dm2200_h__

#include "board.h"

process Receiver (PicOSNode) {

	Transceiver *RFC;

	states { RCV_GETIT, RCV_START, RCV_RECEIVE, RCV_GOTIT };

	byte get_rssi (byte&);

	void setup () {
		RFC = S->RFInterface;
	};

	perform;
};

process	ADC (PicOSNode) {

	Transceiver *RFC;

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

	void setup () {
		RFC = S->RFInterface;
	};

	perform;
};

process Xmitter (PicOSNode) {

	Transceiver *RFC;

	address		buffer;
	int		buflen;
	ADC		*RSSI;

	states { XM_LOOP, XM_TXDONE, XM_LBS };

	perform;

	void setup () {
		buffer = NULL;
		RSSI = create ADC;
		RFC = S->RFInterface;
	};
};

#endif
