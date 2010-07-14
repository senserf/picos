#ifndef __rfmodule_dm2200_h__
#define	__rfmodule_dm2200_h__

#include "board.h"

#define	rfi	(rf->RFInterface)
#define	xbf	(rf->__pi_x_buffer)
#define	rbf	(rf->__pi_r_buffer)
#define	obf	(rf->OBuffer)
#define	bkf	(rf->backoff)
#define	txe	(rf->tx_event)
#define	rxe	rf
#define	txoff	(rf->TXOFF)
#define	rxoff	(rf->RXOFF)
#define	sid	(rf->statid)
#define	minbkf	(rf->min_backoff)
#define	maxbkf	(rf->max_backoff)
#define	lbtth	(rf->lbt_threshold)
#define	lbtdl	(rf->lbt_delay)
#define	xmtg	(rf->Xmitting)
#define	rcvg	(rf->Receiving)
#define	defxp	(rf->DefXPower)
#define	defrt	(rf->DefRate)
#define	defch	(rf->DefChannel)
#define	physid	(rf->phys_id)

strandhdr (Receiver, PicOSNode) {

	int RBS;			// Receive buffer size
	rfm_intd_t *rf;

	states { RCV_GETIT, RCV_START, RCV_RECEIVE, RCV_GOTIT };

	byte get_rssi (byte&);

	perform;

	void setup (int s) {
		RBS = s;
		rf = TheNode->RFInt;
	};
};

threadhdr (ADC, PicOSNode) {

	double		ATime,		// Accumulated sampling time
			Average,	// Average signal so far
			CLevel;		// Current (last) signal level
	TIME		Last;		// Last sample time
	rfm_intd_t 	*rf;

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

	void setup () {
		rf = TheNode->RFInt;
	};

};

threadhdr (Xmitter, PicOSNode) {

	int		buflen;
	ADC		*RSSI;
	rfm_intd_t	*rf;

	states { XM_LOOP, XM_TXDONE, XM_LBS };

	perform;

	void setup () {
		rf = TheNode->RFInt;
		xbf = NULL;
		RSSI = create ADC;
	};

	inline void gbackoff () {
		bkf = minbkf + toss (maxbkf);
	};

	inline void pwr_on () {
		TheNode->pwrt_change (PWRT_RADIO, 
			rxoff ? PWRT_RADIO_XMT : PWRT_RADIO_XCV);
	};

	inline void pwr_off () {
		TheNode->pwrt_change (PWRT_RADIO, 
			rxoff ? PWRT_RADIO_OFF : PWRT_RADIO_RCV);
	};
};

#endif
