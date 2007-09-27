#ifndef __rfmod_dcf_cc__
#define __rfmod_dcf_cc__

#include "rfmod_dcf.h"

// Global variables, i.e., constants, of the DCF scheme
TIME	DCF_TSifs, DCF_TDifs, DCF_TEifs, DCF_NAV_rts,
		DCF_NAV_rts_auto, DCF_NAV_cts, DCF_NAV_data, DCF_TIMEOUT_bot,
			DCF_TIMEOUT_cts, DCF_TIMEOUT_ack;

double	DCF_DSlot;

// A shortcut: assumes the same transmission rate for everybody
RATE	DCF_XRate = TIME_0;

int	DCF_RTR_short,	// Short retransmission threshold
	DCF_RTR_long;	// Long retransmission threshold

int	DCF_CW_min,	// Minimum backoff window size
	DCF_CW_max;	// Maximum backoff window size

Long	DCF_RTS_ths;	// Short data threshold

void initDCF (
		double sf,
		double sl,

		Long rth,

		int srl,
		int lrl,

		int cwmin,
		int cwmax
	     						) {

	// SIFS in ITU
	DCF_TSifs = etuToItu (sf);

	// This is the slot length stored as double - for ease of calculations
	DCF_DSlot = Etu * sl;

	// Calculate DIFS
	DCF_TDifs = DCF_TSifs + (TIME) (DCF_DSlot * 2.0);

	// Short data threshold
	DCF_RTS_ths = rth;

	// Retransmission
	DCF_RTR_short = srl;
	DCF_RTR_long = lrl;

	// Thresholds
	DCF_CW_min = cwmin;
	DCF_CW_max = cwmax;

	print ("DCF parameters:\n\n");
	print (ituToEtu (DCF_TSifs), "  SIFS:", 			10, 26);
	print (DCF_DSlot * Itu,      "  SLOT: ", 			10, 26);
	print (ituToEtu (DCF_TDifs), "  DIFS:", 			10, 26);
	print (DCF_RTS_ths,          "  RTS ths:", 			10, 26);
	print (DCF_RTR_short,        "  Max retr count (short):",	10, 26);
	print (DCF_RTR_long,         "  Max retr count (long):", 	10, 26);
	print (DCF_CW_min,           "  CW min:", 			10, 26);
	print (DCF_CW_max,           "  CW max:", 			10, 26);
	print ("\n");
}

RFModule::RFModule (Transceiver *r, Long pqs) {

	TIME pr;
	RATE xr;

	S = TheStation;
	Xcv = r;
	PQ = create PQueue (pqs);

	backoff_reset ();
	backoff_clear ();

	xr = r->getTRate ();

	if (DCF_XRate == TIME_0) {

		TIME xt_ack, xt_cts;

		// First time around: initialize global "constants"

		DCF_XRate = xr;		// Transmission rate

		// This tacitly assumes that everybody is using the same
		// preamble length. If not, we will have to make it the
		// maximum (and wait with the initialization of globals
		// until the last RFModule has been created).
		pr = Xcv->getPreambleTime();

		xt_ack = Ether->RFC_xmt (DCF_XRate, PKT_LENGTH_ACK);
		xt_cts = Ether->RFC_xmt (DCF_XRate, PKT_LENGTH_CTS);

		print (ituToEtu(pr),	 "  Preamble time:",		10, 26);
		print (PKT_LENGTH_RTS,   "  RTS length:",		10, 26);
		print (ituToEtu(Ether->RFC_xmt (DCF_XRate, PKT_LENGTH_RTS)),
					 "  RTS time:",			10, 26);
		print (PKT_LENGTH_CTS,   "  CTS length:",		10, 26);
		print (ituToEtu(xt_cts), "  CTS time:", 		10, 26);
		print (PKT_LENGTH_ACK,   "  ACK length:",		10, 26);
		print (ituToEtu(xt_ack), "  ACK time:", 		10, 26);

		DCF_TEifs = DCF_TSifs + DCF_TDifs + pr + xt_ack;
		print (ituToEtu (DCF_TEifs), "  EIFS:", 		10, 26);

		// Pre-calculate the fixed NAV components: just add to this
		// the data transmission time.

		DCF_NAV_data = 
			// Time to transmit the ACK packet sans the preamble
			xt_ack
			// Time to send the preamble
			+ pr
			// SIFS before the ACK
			+ DCF_TSifs;
		print (ituToEtu (DCF_NAV_data), "  NAV data:",		10, 26);

		DCF_NAV_cts =
			DCF_NAV_data
			// Data preamble
			+ pr
			// Plus the SIFS separating CTS and DATA
			+ DCF_TSifs;
		print (ituToEtu (DCF_NAV_cts), "  NAV CTS:",		10, 26);

		DCF_NAV_rts =
			// We need all that is needed for a CTS
			DCF_NAV_cts
			// Plus the pure transmission time of the CTS packet
			+ xt_cts
			// Plus the CTS preamble
			+ pr
			// Plus the gap separating RTS from CTS
			+ DCF_TSifs;
		print (ituToEtu (DCF_NAV_rts), "  NAV RTS:",		10, 26);

		// This is the delay after which a node that has set its
		// NAV based on RTS can cancel the NAV if it doesn't perceive
		// a data packet earlier. Well, in plain English, this is the
		// NAV setting for a node that hears an RTS. In our 
		// implementation, data packets carry NAV, which will reset
		// the short NAV after RTS, if the data packet makes it.
		// We can do this because we do not have multisegment
		// handshakes.
		//
		// Note: this is optional, so perhaps we should make it
		// optional as well
		DCF_NAV_rts_auto =
			DCF_TSifs + DCF_TSifs
			+ xt_cts
			+ pr
			+ (TIME) (DCF_DSlot * 2.0);
		print (ituToEtu (DCF_NAV_rts_auto), "  NAV RTS (auto):",
									10, 26);
		// Timeout to a BOT after RTS or CTS
		DCF_TIMEOUT_bot =
			DCF_TSifs
			+ pr 
			+ (TIME) (DCF_DSlot * 2.0);
		print (ituToEtu (DCF_TIMEOUT_bot),  "  BOT timeout:", 	10, 26);

		// CTS timeout counted from perceived BOT, i.e., the length
		// of CTS + something small
		DCF_TIMEOUT_cts =
			xt_cts
			+ DCF_TSifs;
		// ACK timeout, counted as for CTS
		print (ituToEtu (DCF_TIMEOUT_cts),  "  CTS timeout:", 	10, 26);

		DCF_TIMEOUT_ack = xt_ack + DCF_TSifs;
		print (ituToEtu (DCF_TIMEOUT_ack),  "  ACK timeout:", 	10, 26);
		print ("\n");
			
	}

	create Xmitter (this);
	create Receiver (this);

	TEnav = TIME_0;
	FLG_vbusy = NO;
}

Xmitter::perform {

    state XM_READY:

	// We get here every time the backoff happily expires; so first, mark
	// there is no backoff condition; we need this to tell (when we sense
	// an activity), that the backoff should be set anew
	RFM->backoff_clear ();

	// See if there is a packet to transmit
	if (CP == NULL) {
		// Need a brand new packet
		if ((CP = RFM->get_packet (XM_READY)) != NULL) {
			// Just in case, zero out the NAV; note that, as we
			// do not implement fragments, the packet's NAV can
			// (in fact should) be zero.
			CP->NAV = TIME_0;
			// Also, reset the failure counters
			RetrS = RetrL = 0;
			// Mark it first transmission
			clearFlag (CP->DCFP_Flags, DCFP_FLAG_RETRANS);
			_trc ("NEW %s", _dpk (CP));
		} else {
			// No packet; get_packet has issued the wait request,
			// so just take care of ACTIVITY; note that even if
			// we have nothing to transmit we should be keeping
			// track of what is going on
			_trc ("NO PACKET");
			if (RFM->wait_activity_event (XM_BUSY))
				sameas XM_BUSY;
			sleep;
		}
	} else
		_trc ("RETR [%1d,%1d] %s", RetrS, RetrL, _dpk (CP));

	// A packet is available: it may be a brand new packet, or a
	// retransmission; first two special cases

	if (flagSet (CP->DCFP_Flags, DCFP_FLAG_BCST)) {
		// This is a broadcast packet: no handshake, no ACK
		_trc ("DATA BCST");
		RFM->xmit_data (CP, XM_EBCST);
		sleep;
	}

	if (CP->TLength <= DCF_RTS_ths) {
		// Short packet: send without a CTS, but expect an ACK
		_trc ("DATA SHRT");
		RFM->xmit_data (CP, XM_ESHRT);
		sleep;
	}

	// Need the handshake, start with RTS; note that xrts sets the NAV of
	// the RTS packet based on the packet length
	RFM->xmit_rts (CP, XM_RTSD);

    state XM_RTSD:

	_trc ("RTS SENT");
	// RTS done: stop it and wait for CTS
	RFM->xstop ();
	// Notify upper layers
	RFM->UPPER_snt_rts (TheDCFP->DCFP_R);
	// Wait for CTS: the maximum time before BOT
	RFM->TDExp = Time + DCF_TIMEOUT_bot;

    transient XM_WCTB:

	RFM->wait_bot_event (XM_WCTS, XM_COLC);

    state XM_WCTS:

	_trc ("CTS BOT");
	RFM->wait_cts_event (XM_CTSD, XM_CTSN, XM_COLC);

    state XM_CTSN:

	// Another BOT
	_trc ("CTS BOT SUP");
	if (RFM->TDExp > Time)
		// Keep waiting
		sameas XM_WCTB;

    transient XM_COLC:

	_trc ("NO CTS");
	// CTS-level collision, i.e., missing CTS
	RFM->UPPER_col_cts (RetrL, RetrS);
	if (++RetrS >= DCF_RTR_short) {
		// Limit reached, drop the packet
		_trc ("DROPPING C [%1d,%1d] %s", RetrS, RetrL, _dpk (CP));
		RFM->UPPER_fai_data (CP, RetrS, RetrL);
		// Mark done with the packet
		CP = NULL;
		// Reset backoff window
		RFM->backoff_reset ();
	} else {
		// We shall retransmit: push the backoff window
		RFM->backoff_push ();
	}
	// Generate the backoff
	RFM->backoff_generate ();
	sameas XM_BACK;

    state XM_CTSD:

	_trc ("CTS RECEIVED");
	// Reset the short counter after every successful CTS reception
	RetrS = 0;
	// Notify the upper layers that the CTS has been received
	RFM->UPPER_rcv_cts (CP->DCFP_R, RFM->RSSI);
	// Wait for SIFS
	Timer->wait (DCF_TSifs, XM_DATA);
	// Note: the standard says that having received CTS, wait blindly for
	// SIFS and transmit the packet, so no further checks are needed here.

    state XM_DATA:

	_trc ("XMIT DATA H");
	// Transmit a DATA packet after RTS/CTS handshake
	RFM->xmit_data (CP, XM_DATD);

    state XM_DATD:

	_trc ("DATA H DONE");
	// Stop data transmission
	RFM->xstop ();
	// Notify upper layers
	RFM->UPPER_snt_data (CP);
	// Wait for ACK
	RFM->TDExp = Time + DCF_TIMEOUT_bot;

    transient XM_WACB:

	RFM->wait_bot_event (XM_WACK, XM_COLD);

    state XM_WACK:

	RFM->wait_ack_event (XM_ACKD, XM_ACKN, XM_COLD);

    state XM_ACKN:

	// Another BOT
	_trc ("ACK BOT SUP");
	if (RFM->TDExp > Time)
		// Keep waiting
		sameas XM_WACB;

    transient XM_COLD:

	_trc ("NO H ACK");
	// DATA-level collision
	RFM->UPPER_col_ack (RetrL, RetrS);
	if (++RetrL >= DCF_RTR_long) {
		// Limit reached; drop the packet
		_trc ("DROPPING A [%1d,%1d] %s", RetrS, RetrL, _dpk (CP));
		RFM->UPPER_fai_data (CP, RetrS, RetrL);
		CP = NULL;
		// Reset backoff
		RFM->backoff_reset ();
	} else {
		// Push the backoff window
		RFM->backoff_push ();
		// Flag == data retransmitted
		setFlag (CP->DCFP_Flags, DCFP_FLAG_RETRANS);
	}
	// Generate backoff
	RFM->backoff_generate ();

    transient XM_BACK:

	// Start the backoff if silence and wait for it; we check for activity
	// immediately to avoid races
	if (!RFM->busy ()) {
		_trc ("IDLE I");
		// Activate the backoff
		TIME t = RFM->backoff_delay ();
		if (t == 0)
			sameas XM_READY;
		Timer->wait (t, XM_READY);
		if (RFM->wait_activity_event (XM_BUSY))
			sameas XM_BUSY;
		sleep;
	}

    transient XM_BUSY:

	// An activity
	_trc ("BUSY");
	if (!RFM->backoff_on ()) {
		// Generate backoff, if not on already
		RFM->backoff_generate ();
		// Perhaps should put this check into backoff_generate
	} else {
		RFM->backoff_stop ();
	}

	if (RFM->wait_silence_event (XM_IDLE))
		sameas XM_IDLE;

    state XM_ACKD:

	_trc ("ACK H RECEIVED");
	// Notify the upper layers
	RFM->UPPER_rcv_ack (CP->DCFP_R, RFM->RSSI);
	// Tell them about the success
	RFM->UPPER_suc_data (CP, RetrS, RetrL);

XM_EData:
	CP = NULL;
	// Reset the backoff window
	RFM->backoff_reset ();
	// Generate backoff (left suspended until run)
	RFM->backoff_generate ();
	// The role of this is to make the next slot has actually begun
	// when we start waiting for DIFS; in particular, whatever activity
	// is detected, even immediately after we start waiting, it will
	// result in the proper advancement of the backoff
	skipto XM_SLOT;

    state XM_SLOT:

	// We have to wait for DIFS before backing off
	Timer->wait (DCF_TDifs, XM_BACK);
	// And if anything happens in the meantime, we shall know
	if (RFM->wait_activity_event (XM_BUSY))
		sameas XM_BUSY;

    state XM_IDLE:

	if (RFM->FLG_garbage) {
		// Delay for EIFS
		_trc ("IDLE S EIFS");
		Timer->wait (DCF_TEifs, XM_BACK);
	} else {
		_trc ("IDLE S DIFS");
		Timer->wait (DCF_TDifs, XM_BACK);
	}

	if (RFM->wait_activity_event (XM_BUSY))
		sameas XM_BUSY;

    state XM_ESHRT:

	// End of short transmission
	_trc ("DATA S DONE");
	RFM->xstop ();
	RFM->UPPER_snt_data (CP);
	// Wait for ACK
	RFM->TDExp = Time + DCF_TIMEOUT_bot;

    transient XM_WASB:

	RFM->wait_bot_event (XM_WASK, XM_COLS);

    state XM_WASK:

	RFM->wait_ack_event (XM_ACKD, XM_ASKN, XM_COLS);

    state XM_ASKN:

	// Another BOT
	_trc ("ASK BOT SUP");
	if (RFM->TDExp > Time)
		// Keep waiting
		sameas XM_WASB;
	
    transient XM_COLS:

	_trc ("NO S ACK");
	// DATA-level collision
	RFM->UPPER_col_ack (RetrL, RetrS);
	if (++RetrS >= DCF_RTR_short) {
		// Limit reached; drop the packet
		_trc ("DROPPING [%1d,%1d] %s", RetrS, RetrL, _dpk (CP));
		RFM->UPPER_fai_data (CP, RetrS, RetrL);
		CP = NULL;
		// Reset backoff
		RFM->backoff_reset ();
	} else {
		// Push the backoff window
		RFM->backoff_push ();
		// Flag == data retransmitted
		setFlag (CP->DCFP_Flags, DCFP_FLAG_RETRANS);
	}
	// Generate the backoff
	RFM->backoff_generate ();

	sameas XM_BACK;

    state XM_EBCST:

	// End of a broadcast transmission
	_trc ("DATA B DONE");
	RFM->xstop ();
	RFM->UPPER_snt_data (CP);
	RFM->UPPER_suc_data (CP, 0, 0);

	goto XM_EData;
}

Receiver::perform {

// =============================================================================
// This is what we do when we are done with the interpretation of a reception
// episode

#define	rc_endcycle	do { \
			  if (RFM->active ()) \
			    sameas RC_STILLACTIVE; \
			  else \
			    sameas RC_SILENCE; \
			} while (0)

// =============================================================================

    state RC_LOOP:

	RFM->wait_reception (RC_ACTIVITY);

    state RC_ACTIVITY:

	RFM->mark_activity ();

    transient RC_STILLACTIVE:

	RFM->wait_packet (RC_PACKET, RC_SILENCE);

    state RC_SILENCE:

	// This is channel silence, we are also responsible for handling
	// the NAV
	if (RFM->mark_silence (RC_ACTIVITY, RC_SILENCE))
		// Note: mark_silence does nothing in that case
		sameas RC_STILLACTIVE;

    state RC_PACKET:

	// Beginning of packet
	RFM->signal_bot ();
	RFM->init_reception (RC_GETPACKET);

    state RC_GETPACKET:

	RFM->trace_packet (RC_RECEIVED, RC_ABORTED);

    state RC_RECEIVED:

	// Packet received
	RFM->FLG_garbage = NO;

	if (flagSet (TheDCFP->DCFP_Flags, DCFP_FLAG_BCST)) {
		assert (TheDCFP->DCFP_Type == DCFP_TYPE_DATA, 
			"rfmodule rcv: broadcast packet not DATA");
		// Receive a brodcast DATA packet
		RFM->receive ();
		rc_endcycle;
	}

	// A non-broadcast packet must be addressed to us (in data link)
	if (!RFM->my_packet (TheDCFP)) {
		// Set the NAV
		if (TheDCFP->DCFP_Type == DCFP_TYPE_RTS) {
			// NAV is set to a constant timeout for the data
			// packet; if the data packet arrives, the NAV will
			// be reset. This is an option.
			_trc ("RTS OTHER");
			RFM->set_nav (DCF_NAV_rts_auto);
		} else {
			// Take it at the face value
			_trc ("NON-RTS OTHER");
			RFM->set_nav (TheDCFP->NAV);
		}
		rc_endcycle;
	}

RC_spacket:

	switch (TheDCFP->DCFP_Type) {

	    case DCFP_TYPE_DATA:

		// A non-broadcast data packet received without a handshake
		// must be short; not really, everything is possible
		// assert (TheDCFP->TLength <= DCF_RTS_ths,
		// 	"rfmdule rcv: non-handshake data packet not short");

		RFM->receive ();
		// Delay before sending ACK
		Timer->wait (DCF_TSifs, RC_SACK);
		sleep;

	    case DCFP_TYPE_RTS:

		// RTS packet to us. Do not set your own NAV. Any existing
		// NAV setting will be honored when we get to sending CTS.

		// Save the sender identity for CTS
		CS = TheDCFP->DCFP_S;
		if ((NV = TheDCFP->NAV) != TIME_0)
			// Calculate the pure DATA component of the NAV; we
			// need it for the CTS
			NV -= DCF_NAV_rts;
		_trc ("RTS MY %f", (double) NV);

		// Notify the upper layers
		RFM->UPPER_rcv_rts (CS, RFM->RSSI);
		// Delay for SIFS; the logical channel status remains busy
		Timer->wait (DCF_TSifs, RC_SCTS);
		sleep;

	    case DCFP_TYPE_CTS:

		// CTS packet: signal the transmitter. Note that in this case
		// the upper layers will be notified by the transmitter, if
		// the CTS is relevant.
		_trc ("CTS MY");
		RFM->signal_cts ();
		rc_endcycle;

	    case DCFP_TYPE_ACK:

		// ACK packet: signal the transmitter
		_trc ("ACK MY");
		RFM->signal_ack ();
		rc_endcycle;

	    default:

		excptn ("rfmodule rcv: illegal packet type %1d",
			TheDCFP->DCFP_Type);
	}

    state RC_ABORTED:

	_trc ("RCPT ABORTED");
	// This is an aborted something that at some point looked like a
	// packet. FLG_garbage will be set by default.
	rc_endcycle;

    state RC_SCTS:

	// We have received RTS and are supposed to respond with CTS, and the
	// time is now. The function (xcts) checks if we are not within a
	// (foreign) NAV and does nothing returning NO in such a case. Note
	// that the transmitter is kept under the impression that medium is
	// busy, so it will not interfere.

	if (!RFM->xmit_cts (CS, NV, RC_SCTSD))
		rc_endcycle;

    state RC_SCTSD:

	_trc ("CTS SENT");
	// Done sending CTS
	RFM->xstop ();
	// Notify upper layers
	RFM->UPPER_snt_cts (TheDCFP->DCFP_R);
	// Wait for the data packet
	RFM->FLG_garbage = YES;
	// Set the BOT timer
	RFM->TDExp = Time + DCF_TIMEOUT_bot;

    transient RC_WDATA:

	RFM->wait_bot (RC_DATAP, RC_NODATA);

    state RC_DATAP:

	RFM->init_reception (RC_GETDATA);

    state RC_GETDATA:

	RFM->trace_packet (RC_DATARCV, RC_DATAABT);

    state RC_DATAABT:

	// Should we reset FLG_garbage to 0?
	_trc ("EXP DATA ABORT");
	if (Time < RFM->TDExp)
		// Keep waiting
		sameas RC_WDATA;

    transient RC_NODATA:

	_trc ("EXP DATA NODATA");
	RFM->UPPER_col_data ();
	rc_endcycle;

    state RC_DATARCV:

	RFM->FLG_garbage = NO;

	// This must be a non-broadcast data packet addressed to us
	if (flagSet (TheDCFP->DCFP_Flags, DCFP_FLAG_BCST)) {
		// Broadcast: signal a collision to the upper layers
		_trc ("EXP DATA BCST");
		RFM->UPPER_col_data ();
		// ... but receive the packet anyway (as a gift)
		RFM->receive ();
		rc_endcycle;
	}

	// A non-broadcast packet must be addressed to us
	if (!RFM->my_packet (TheDCFP)) {
		_trc ("EXP DATA FOREIGN");
		RFM->UPPER_col_data ();
		// Set NAV and ignore the packet
		RFM->set_nav (TheDCFP->NAV);
		rc_endcycle;
	}

	if (TheDCFP->DCFP_Type != DCFP_TYPE_DATA) {
		// Not a data packet
		_trc ("EXP DATA NODATA");
		RFM->UPPER_col_data ();
		// process as for a standard reception
		goto RC_spacket;
	}

	// Data packet: receive it
	RFM->receive ();
	// Delay before sending ACK
	Timer->wait (DCF_TSifs, RC_SACK);
	sleep;

    state RC_SACK:

	RFM->xmit_ack (CS, RC_SACKD);

    state RC_SACKD:

	_trc ("ACK SENT");
	RFM->xstop ();
	// Upper layers go again
	RFM->UPPER_snt_ack (TheDCFP->DCFP_R);
	rc_endcycle;
}

#endif
