#ifndef __rfmod_dcf_cc__
#define __rfmod_dcf_cc__

#include "rfmod_dcf.h"

// Global variables, i.e., constants, of the DCF scheme
TIME	DCF_TDifs, DCF_TSifs, DCF_TEifs, DCF_NAV_delta, DCF_NAV_rts,
		DCF_NAV_cts, DCF_NAV_data, DCF_TIMEOUT_cts, DCF_TIMEOUT_ack;

double	DCF_Slot;

// A shortcut: assumes the same transmission rate for everybody
RATE	DCF_XRate = TIME_0;

Long	DCF_CW_min,	// Minimum backoff window size
	DCF_CW_max;	// Maximum backoff window size

void initDCF (
		double sf, double df, double ef,
		double nd,
		double ss,
		Long cwmin,
		Long cwmax
	     						) {

	DCF_TSifs = etuToItu (sf);
	DCF_TDifs = etuToItu (df);
	DCF_TEifs = etuToItu (ef);
	DCF_NAV_delta = etuToItu (nd);
	DCF_Slot = ss;
	DCF_CW_min = cwmin;
	DCF_CW_max = cwmax;
}

RFModule::RFModule (Transceiver *r, Long pqs, Long retr, PRETFUN prf) {

	RATE xr;

	S = TheStation;
	pretfun = prf;
	Xcv = r;
	PQ = create PQueue (pqs);
	RMax = retr;

	xr = r->getTRate ();
	if (DCF_XRate == TIME_0) {

		// Initialize globals

		DCF_XRate = xr;

		/*
		 * Calculate the fixed NAV components 
		 */

		// Needed after DATA to accommodate ACK
		DCF_NAV_data = Ether->RFC_xmt (DCF_XRate, PKT_LENGTH_ACK) 
			+ DCF_TSifs	// DATA - ACK
			+ DCF_NAV_delta;

		// After CTS on top of DATA
		DCF_NAV_cts = DCF_NAV_data
			+ DCF_TSifs;	// CTS - DATA

		// After RTS on top of DATA
		DCF_NAV_rts = DCF_NAV_cts + 
			Ether->RFC_xmt (DCF_XRate, PKT_LENGTH_RTS)
			+ DCF_TSifs;	// RTS - CTS

		// CTS timeout
		DCF_TIMEOUT_cts = DCF_TSifs + DCF_TSifs
			// Should we add here one Sifs just in case?
			+ Ether->RFC_xmt (DCF_XRate, PKT_LENGTH_CTS);

		// ACK timeout
		DCF_TIMEOUT_ack = DCF_TSifs + DCF_TSifs
			+ Ether->RFC_xmt (DCF_XRate, PKT_LENGTH_ACK);
	} else {
		Assert (DCF_XRate == xr,
			"RFModule init: transmission rates of all nodes "
				"must be the same");
	}

	TAccs = TIdle = TBusy = TEnav = TIME_0;

	create Xmitter (this);
	create Receiver (this);
}

Xmitter::perform {

    TIME t;

    state XM_LOOP:

	CP = RFM->getp (XM_LOOP);
	Retrans = 0;
	// Reset backoff parameters for a new packet
	clearFlag (CP->DCFP_Flags, DCFP_FLAG_RETRANS);
	trc_pkt (CP, "ACQUIRED");

    transient XM_TRY:

	// Obey backoff and diffs
	RFM->bckf (XM_TRY, XM_BACK);
	if (flagSet (CP->DCFP_Flags, DCFP_FLAG_BCST)) {
		// Broadcast packet, no handshake
		CP->NAV = TIME_0;
		trc_pkt (CP, "BROADCAST");
		RFM->xmit (CP, XM_EBCST);
		sleep;
	}
	// Send RTS: this also sets the packet's NAV
	RFM->xrts (CP, XM_RTS);

    state XM_BACK:

	// Backoff required, i.e., channel became busy or idle after busy
	RFM->genb (BKF_BUSY);
	// Note: genb honors old backoff in progress
	proceed XM_TRY;

    state XM_RTS:

	RFM->xstop ();
	// Wait for CTS
	RFM->wcts (XM_DATA, XM_NOCTS);

    state XM_DATA:

	// Wait for SIFS
	RFM->sifs (XM_XMT, XM_SBUSY);

    state XM_XMT:

	// Transmit data
	RFM->xmit (CP, XM_WAK);

    state XM_SBUSY:

	// Busy while waiting for SIFS before sending data; I am not sure
	// we have to monitor for this
	trc_col (CP, "INTERRUPTED CTS-DATA SIFS");
	RFM->genb (BKF_BUSY);
	proceed XM_TRY;

    state XM_WAK:

	RFM->xstop ();
	RFM->wack (XM_DONE, XM_COLL);

    state XM_DONE:

	RFM->done (CP, Retrans);
	RFM->rstb ();
	proceed XM_LOOP;

    state XM_EBCST:

	RFM->xstop ();
	RFM->done (CP, 0);
	RFM->rstb ();
	proceed XM_LOOP;

    state XM_NOCTS:

	// CTS timeout
	trc_col (CP, "CTS TIMEOUT");
	if (Retrans >= RFM->RMax) {
		// Drop the packet (FIXME: collect statistics)
		trc_col (CP, "DROPPING DATA PACKET");
		RFM->drop (CP);
		RFM->rstb ();
		proceed XM_LOOP;
	}

	trc_pkt (CP, "RETRYING");
	Retrans++;
	RFM->genb (BKF_COLL);
	proceed XM_TRY;

    state XM_COLL:

	trc_col (CP, "ACK TIMEOUT");
	if (Retrans >= RFM->RMax) {
		// Drop the packet (FIXME: collect statistics)
		trc_col (CP, "DROPPING DATA PACKET");
		RFM->drop (CP);
		RFM->rstb ();
		proceed XM_LOOP;
	}

	trc_pkt (CP, "RETRYING");
	Retrans++;
	RFM->genb (BKF_COLL);
	// The only difference w.r.t. NOCTS is the setting of this flag
	setFlag (CP->DCFP_Flags, DCFP_FLAG_RETRANS);
	proceed XM_TRY;
}

Receiver::perform {
//
// Note: in addition to reception, this process is responsible for monitoring
// the channel status, like in IDLE, BUSY, and conveing those changes to the
// transmitter.
//

    state RC_LOOP:

	// In this state we are idle - waiting for any activity
	RFM->wact (RC_ACT);

    state RC_ACT:

	// Activity perceived
SACT:
	RFM->sact ();
WBOT:
	RFM->wbot (RC_BOT, RC_SIL);

    state RC_SIL:

SSIL:
	if (RFM->ssil (RC_ACT, RC_SIL))
		// Still busy
		goto SACT;

    state RC_BOT:

	RFM->strc (RC_RCV);

    state RC_RCV:

	RFM->wrec (RC_RCVD, RC_RABT);

    state RC_RCVD:

	// Check for special packets
	if (flagSet (TheDCFP->DCFP_Flags, DCFP_FLAG_BCST)) {
		assert (TheDCFP->DCFP_Type == DCFP_TYPE_DATA, 
			"rfmodule rcv: broadcast packet not DATA");
		RFM->recv ();
		// Broadcast packets have no NAV
		if (RFM->busy ()) goto WBOT; else goto SSIL;
	}

	// A non-broadcast packet must be addressed to us
	if (!RFM->ismy (TheDCFP)) {
		// No, set NAV and ignore the packet
		RFM->snav (TheDCFP->NAV);
		if (RFM->busy ()) goto WBOT; else goto SSIL;
	}

	switch (TheDCFP->DCFP_Type) {

	    case DCFP_TYPE_DATA:

		// Data packet (handshake)
		RFM->recv ();
		// Delay before ACK
		Timer->wait (DCF_TSifs, RC_SACK);
		sleep;

	    case DCFP_TYPE_RTS:

		// Set NAV
		RFM->snav (TheDCFP->NAV);
		// Preserve the sender identity for CTS
		CS = TheDCFP->DCFP_S;
		if ((NV = TheDCFP->NAV) != TIME_0)
			// This is the DATA component of RTS's NAV
			NV -= DCF_NAV_rts;
		// Delay for SIFS while keeping the channel's appearance as
		// busy for the transmitter
		Timer->wait (DCF_TSifs, RC_SCTS);
		sleep;

	    case DCFP_TYPE_CTS:

		// Signal the transmitter
		RFM->scts ();
		if (RFM->busy ()) goto WBOT; else goto SSIL;

	    case DCFP_TYPE_ACK:

		// Signal the transmitter
		RFM->sack ();
		if (RFM->busy ()) goto WBOT; else goto SSIL;

	    default:

		excptn ("rfmodule rcv: illegal packet type %1d",
			TheDCFP->DCFP_Type);
	}

    state RC_SACK:

	RFM->xack (CS, RC_SACKD);

    state RC_SACKD:

	RFM->xstop ();
	if (RFM->busy ()) goto WBOT; else goto SSIL;

    state RC_RABT:

	// This is an aborted something that at some point looked like a
	// packet. If I understand the standard correctly, we should now
	// wait for EIFS.

	RFM->snav (DCF_TEifs);
	if (RFM->busy ()) goto WBOT; else goto SSIL;

    state RC_SCTS:

	// I understand that we should send it blindly - at least to enforce a
	// collision. Or should be monitor the channel while waiting for SIFS
	// and yield?

	RFM->xcts (CS, NV, RC_SCTSD);

    state RC_SCTSD:

	RFM->xstop ();
	if (RFM->busy ()) goto WBOT; else goto SSIL;
}

#endif
