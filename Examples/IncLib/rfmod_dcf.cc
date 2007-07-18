#ifndef __rfmod_dcf_cc__
#define __rfmod_dcf_cc__

#include "rfmod_dcf.h"

// Global variables, i.e., constants, of the DCF scheme
TIME	DCF_TDifs, DCF_TSifs, DCF_TSifs2, DCF_TEifs, DCF_NAV_delta, DCF_NAV_rts,
		DCF_NAV_cts, DCF_NAV_data, DCF_TIMEOUT_cts, DCF_TIMEOUT_ack,
			DCF_PRE_time;

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
	// Pre-computed SIFS * 2
	DCF_TSifs2 = DCF_TSifs + DCF_TSifs;
	DCF_TDifs = etuToItu (df);
	DCF_TEifs = etuToItu (ef);
	// Safety margin added to every NAV (calculated solely on exact packet
	// timing)
	DCF_NAV_delta = etuToItu (nd);
	// Slot time in seconds
	DCF_Slot = ss;
	// Minimum and maximum window size in slots
	DCF_CW_min = cwmin;
	DCF_CW_max = cwmax;
}

RFModule::RFModule (Transceiver *r, Long pqs, Long retr) {

	RATE xr;

	S = TheStation;
	Xcv = r;
	PQ = create PQueue (pqs);
	RMax = retr;
	rstb ();			// Make sure CWin is initialized

	xr = r->getTRate ();
	if (DCF_XRate == TIME_0) {

		// First time around: initialize global "constants"

		DCF_XRate = xr;		// Transmission rate

		// This tacitly assumes that everybody is using the same
		// preamble length. If not, we will have to make it the
		// maximum (and wait with the initialization of globals
		// until the last RFModule has been created).
		DCF_PRE_time = Xcv->getPreambleTime();

		/*
		 * Pre-calculate the fixed NAV components: 
		 */

		// This is the amount of NAV time for a DATA packet to allow
		// everybody to provide room for the following ACK.
		// We start counting from the end of the DATA packet. Formally,
		// NAV in that case should be SIFS + the transmission time of
		// the ACK, but the SIFS should be included twice because it
		// represents the (maximum) propagation time across the medium
		// diameter. The ACK is transmitted SIFS time units after the
		// perception of the end of the DATA packet, so the maximum
		// gap separating DATA and ACK perceived by a third party can
		// only be bounded by 2*SIFS.
		DCF_NAV_data =
			// Time to transmit the ACK packet sans the preamble
			Ether->RFC_xmt (DCF_XRate, PKT_LENGTH_ACK) 
			// Time to send the preamble
			+ DCF_PRE_time
			// The SIFS contribution (maximum separation of DATA
			// and ACK)
			+ DCF_TSifs2	// This is SIFS * 2
			// And any explicit extra margin
			+ DCF_NAV_delta;
		// Note that at this time DCF_PRE_time is the exact preamble
		// transmission time. Later (see below) we inflate it by 15%
		// to provide a safe bound for the receiver (see the comments
		// there).
		// I am not sure if there are any specific recommendations for
		// the "delta" of NAV (I would be surprised if there were, or 
		// if there were, then if the manufacturers cared about them).

		// Here is the NAV setting for a CTS packet (without the
		// actual DATA transmission time, which will be added to it.
		DCF_NAV_cts =
			// We need all that is needed for DATA
			DCF_NAV_data
			// Plus DATA preamble (note: we want to make sure that
			// the only component missing from this is the actual
			// transmission time of the "pure" DATA packet (minus
			// its preamble).
			+ DCF_PRE_time
			// Plus the gap separating CTS and DATA
			+ DCF_TSifs2;
		// Note that this applies after we have seen the CTS packet,
		// so we are expecting DATA to follow after max SIFS * 2.
		// And remember that this does not include the DATA transmission
		// time.

		// And here is the NAV setting for RTS on top of DATA, i.e.,
		// you have to add to this the pure DATA transmission time to
		// get the NAV setting for the RTS packet.
		DCF_NAV_rts =
			// We need all that is needed for a CTS
			DCF_NAV_cts + 
			// Plus the pure transmission time of the CTS packet
			Ether->RFC_xmt (DCF_XRate, PKT_LENGTH_RTS)
			// Plus the CTS preamble
			+ DCF_PRE_time
			// Plus the gap separating RTS from CTS
			+ DCF_TSifs2;

		// Now tip the preamble time 15% as, from now on, it will be
		// used for detecting timeouts in the receiver
		DCF_PRE_time = (TIME)(((double)DCF_PRE_time) * 1.15);

		// CTS timeout: this is the amount of time that the sender
		// of RTS has to wait (following the last transmitted bit of
		// CTS) before it concludes that no RTS is forthcoming. This
		// time is measured until the complete reception of the CTS,
		// i.e., the transmitter will wait for this much until the
		// receiver tells it that the CTS has been received.
		DCF_TIMEOUT_cts =
			// The separation (as usual)
			DCF_TSifs2
			// CTS preamble (this time inflated by 15%)
			+ DCF_PRE_time
			// The actual duration of the pure CTS packet
			+ Ether->RFC_xmt (DCF_XRate, PKT_LENGTH_CTS);

		// ACK timeout: same thing
		DCF_TIMEOUT_ack = DCF_TSifs2
			+ DCF_PRE_time
			+ Ether->RFC_xmt (DCF_XRate, PKT_LENGTH_ACK);

	} else {
		Assert (DCF_XRate == xr,
			"RFModule init: transmission rates of all nodes "
				"must be the same");
		Assert (DCF_PRE_time ==
		    (TIME)(((double)Xcv->getPreambleTime()) * 1.15),
			"RFModule init: preamble lengths of all nodes "
			    "must be the same");
	}

	TAccs = TIdle = TBusy = TEnav = TIME_0;

	create Xmitter (this);
	create Receiver (this);
}

Xmitter::perform {

    state XM_LOOP:

	// Acquire a packet to transmit
	CP = RFM->getp (XM_LOOP);
	// Reset the retransmission counter
	Retrans = 0;
	// This flag will be set if the packet is retransmitted. I saw this
	// as a requirement of 802.11 and put it in just in case.
	clearFlag (CP->DCFP_Flags, DCFP_FLAG_RETRANS);
	trc_pkt (CP, "ACQUIRED");
	// Fall through

    transient XM_TRY:

	// Obey backoff, NAV, and difs
	RFM->bckf (XM_TRY, XM_BACK);
	if (flagSet (CP->DCFP_Flags, DCFP_FLAG_BCST)) {
		// Broadcast packet, no handshake, no NAV
		CP->NAV = TIME_0;
		trc_pkt (CP, "BROADCAST");
		RFM->xmit (CP, XM_EBCST);
		sleep;
		// No way past this point
	}
	// Send RTS: this also sets the packet's NAV (see xrts in RFModule)
	RFM->xrts (CP, XM_RTS);

    state XM_BACK:

	// Backoff required, i.e., channel became busy or idle after busy.
	// Note: genb honors previous backoff in progress
	RFM->genb (BKF_BUSY);
	proceed XM_TRY;

    state XM_RTS:

	// Stop RTS
	RFM->xstop ();
	// Notify upper layers
	RFM->snt_rts (TheDCFP->DCFP_R);
	// Wait for CTS
	RFM->wcts (XM_DATA, XM_NOCTS);

    state XM_DATA:

	// CTS received: notify upper layers
	RFM->rcv_cts (CP->DCFP_R, RFM->RSSI);
	// Wait for SIFS
	RFM->sifs (XM_XMT, XM_SBUSY);

    state XM_XMT:

	// SIFS obeyed, transmit data
	RFM->xmit (CP, XM_WAK);

    state XM_SBUSY:

	// Busy while waiting for SIFS (before sending data); I am not sure
	// we have to monitor for this. Perhaps we should just go ahead (as
	// in Ethernet), to enforce "collision consensus". Don't think so, but
	// wouldn't be surprised if other implementation did just that.
	trc_col (CP, "INTERRUPTED CTS-DATA SIFS");
	RFM->genb (BKF_BUSY);
	proceed XM_TRY;

    state XM_WAK:

	// Stop data transmission
	RFM->xstop ();
	// Notify upper layers
	RFM->snt_data (CP);
	// Wait for ACK
	RFM->wack (XM_DONE, XM_COLL);

    state XM_DONE:

	// ACK received: notify upper layers
	RFM->rcv_ack (CP->DCFP_R, RFM->RSSI);
	// Tell them about the success
	RFM->suc_data (CP, Retrans);
	// Reset backof parameters
	RFM->rstb ();
	// And start from the beginning
	proceed XM_LOOP;

    state XM_EBCST:

	// End of a brodcast (no handshake)
	RFM->xstop ();
	// Upper layers
	RFM->snt_data (CP);
	RFM->suc_data (CP, 0);
	// Reset backoff
	RFM->rstb ();
	proceed XM_LOOP;

    state XM_NOCTS:

	// CTS timeout
	trc_col (CP, "CTS TIMEOUT");
	// Upper layers
	RFM->col_cts ();
	if (Retrans >= RFM->RMax) {
		// Retransmission limit reached: drop the packet
		trc_col (CP, "DROPPING DATA PACKET");
		// Failure conveyed to upper layers
		RFM->fai_data (CP);
		// Start over
		RFM->rstb ();
		proceed XM_LOOP;
	}

	trc_pkt (CP, "RETRYING");
	// Update the retransmission counter
	Retrans++;
	// Backoff
	RFM->genb (BKF_COLL);
	// And so on ...
	proceed XM_TRY;

    state XM_COLL:

	// ACK timeout - essentially as for a lost CTS
	trc_col (CP, "ACK TIMEOUT");
	// Upper layers
	RFM->col_ack ();
	if (Retrans >= RFM->RMax) {
		// Drop the packet
		trc_col (CP, "DROPPING DATA PACKET");
		RFM->fai_data (CP);
		RFM->rstb ();
		proceed XM_LOOP;
	}

	trc_pkt (CP, "RETRYING");
	Retrans++;
	RFM->genb (BKF_COLL);
	// The only difference w.r.t. NOCTS is the setting of this flag. 
	// I understand that this is what it means: the packet was actually
	// transmitted before
	setFlag (CP->DCFP_Flags, DCFP_FLAG_RETRANS);
	proceed XM_TRY;
}


Receiver::perform {
//
// Note: in addition to reception, this process is responsible for monitoring
// the channel status, like in IDLE, BUSY, and conveing those changes to the
// transmitter. Thus it looks messy.
//
    state RC_LOOP:

	// In this state we are idle - waiting for any activity
	RFM->wact (RC_ACT);

    state RC_ACT:

// =============================================================================
// This is what we do when we are done with the interpretation of a reception
// episode

#define	rc_endcycle	do { \
			    if (RFM->busy ()) goto RC_wbot; else goto RC_ssil; \
			} while (0)

// =============================================================================

	// Activity perceived. Note: we use labels to eliminate some 'proceeds'
	// for efficiency and elimination of potential races.
RC_sact:
	// Set the status
	RFM->sact ();
RC_wbot:
	// Wait for beginning of packet
	RFM->wbot (RC_BOT, RC_SIL);

    state RC_SIL:

RC_ssil:
	// Silence
	if (RFM->ssil (RC_ACT, RC_SIL))
		// Still busy
		goto RC_sact;

    state RC_BOT:

	// BOT sensed, initialize packet reception
	RFM->strc (RC_RCV);

    state RC_RCV:

	// Wait for the outcome
	RFM->wrec (RC_RCVD, RC_RABT);

    state RC_RCVD:

	// Packet received

	if (flagSet (TheDCFP->DCFP_Flags, DCFP_FLAG_BCST)) {
		assert (TheDCFP->DCFP_Type == DCFP_TYPE_DATA, 
			"rfmodule rcv: broadcast packet not DATA");
		// Receive a brodcast DATA packet (no handshake, no NAV)
		RFM->recv ();
		// Direct transition as needed (we can be busy or silent)
		rc_endcycle;
	}

	// A non-broadcast packet must be addressed to us (in data link)
	if (!RFM->ismy (TheDCFP)) {
		// No, set NAV and ignore the packet
		RFM->snav (TheDCFP->NAV);
		// ... and go wherever needed
		rc_endcycle;
	}

RC_spacket:

	switch (TheDCFP->DCFP_Type) {

	    case DCFP_TYPE_DATA:

		// Non-broadcast DATA packets are received elsewhere (see
		// below). If we see it here, it means that something was
		// wrong (the sender couldn't possibly have sent it without
		// our CTS). So we ignore it.
		trc_unx ("N-B DATA PACKET RECEIVED WITH NO PRIOR CTS");
		rc_endcycle;

	    case DCFP_TYPE_RTS:

		// RTS packet to me. I am not setting my own NAV (any existing
		// setting means that it comes from an overheard handshake,
		// and I will honor it by not responding with CTS). My xmitter
		// will still think the medium is busy, as I am not going to
		// tell it otherwise.

		// Save the sender identity for CTS
		CS = TheDCFP->DCFP_S;
		if ((NV = TheDCFP->NAV) != TIME_0)
			// Calcuate the pure DATA component of the NAV
			NV -= DCF_NAV_rts;
		// Notify the upper layers
		RFM->rcv_rts (CS, RFM->RSSI);
		// Delay for SIFS while keeping the channel's appearance as
		// busy for the transmitter
		Timer->wait (DCF_TSifs, RC_SCTS);
		sleep;

	    case DCFP_TYPE_CTS:

		// CTS packet: signal the transmitter. Note that in this case
		// the upper layers will be notified by the transmitter, if the
		// CTS is relevant.
		RFM->scts ();
		rc_endcycle;

	    case DCFP_TYPE_ACK:

		// ACK packet: signal the transmitter
		RFM->sack ();
		rc_endcycle;

	    default:

		excptn ("rfmodule rcv: illegal packet type %1d",
			TheDCFP->DCFP_Type);
	}

    state RC_RABT:

	// This is an aborted something that at some point looked like a
	// packet. If I understand the protocol correctly, we should now
	// delay for EIFS.
	RFM->snav (DCF_TEifs);
	rc_endcycle;

    state RC_SCTS:

	// We have received RTS and are supposed to respond with CTS, and the
	// time is now. The function (xcts) checks if we are not within a
	// (foreign) NAV and does nothing returning NO in such a case. Note
	// that the transmitter is kept under the impression that medium is
	// busy, so it will not interfere.

	if (!RFM->xcts (CS, NV, RC_SCTSD))
		rc_endcycle;

    state RC_SCTSD:

	// Done sending CTS
	RFM->xstop ();
	// Notify upper layers
	RFM->snt_cts (TheDCFP->DCFP_R);

	// Now for the tricky bit: wait for DATA to detect collisions on time
	// and report on them to the "upper layers". We expect a data packet
	// to commence within SIFS. But don't forget to keep the transmitter
	// under the impression that the channel is busy - until it is safe
	// to tell it otherwise. Note that we haven't set our own NAV on the
	// RTS.

	// Silence timeout for expecting DATA packet (the 'active' timeout
	// is equal to this + DCF_PRE_time
	RFM->TData = Time + DCF_TSifs2;

	if (RFM->busy ())
		goto RD_sact;

    transient RD_SIL:

	if (RFM->dsil (RD_ACT, RD_NDATA))
		goto RD_ndat;

    state RD_ACT:

RD_sact:
	if (RFM->wbod (RD_BOT, RD_SIL))
		goto RD_ndat;

    // =======================================================================
    // This is essentially the same receiver redone for capturing a data packet
    // following CTS.  The additional twist is that we want to diagnose ASAP
    // the case of a missing data packet. This will happen when
    //    
    // - there has been no BOT for DCF_TSifs from the end of CTS
    // - there has been a BOT, but the packet reception has been aborted
    // - a packet has been received, but it isn't DATA
    //
    // A simplification is that we don't have to keep track of the medium
    // status and report it to the transmitter, which is left to believe that
    // the medium is busy all this time - until we have figured our whatever
    // has happened to the DATA packet.
    // =======================================================================

    state RD_BOT:

	RFM->strc (RD_RCV);

    state RD_RCV:

	RFM->wrec (RD_RCVD, RD_RABT);

    state RD_RCVD:

	// This must be a non-broadcast data packet addressed to us
	if (flagSet (TheDCFP->DCFP_Flags, DCFP_FLAG_BCST)) {
		// Broadcast: signal a collision to the upper layers
		RFM->col_data ();
		// ... but receive the packet anyway (as a gift)
		RFM->recv ();
		rc_endcycle;
	}

	// A non-broadcast packet must be addressed to us
	if (!RFM->ismy (TheDCFP)) {
		RFM->col_data ();
		// Set NAV and ignore the packet
		RFM->snav (TheDCFP->NAV);
		rc_endcycle;
	}

	if (TheDCFP->DCFP_Type != DCFP_TYPE_DATA) {
		// Not a data packet
		RFM->col_data ();
		// process as for a standard reception
		goto RC_spacket;
	}

	// Data packet: receive it
	RFM->recv ();
	// Delay before sending ACK
	Timer->wait (DCF_TSifs, RD_SACK);
	sleep;

    state RD_SACK:

	RFM->xack (CS, RD_SACKD);

    state RD_SACKD:

	RFM->xstop ();
	// Upper layers go again
	RFM->snt_ack (TheDCFP->DCFP_R);
	rc_endcycle;

    state RD_RABT:

	// Something looking like a packet but aborted. I think we should
	// reasonably expect it was for us and avoid setting NAV to EIFS.

	// RFM->snav (DCF_TEifs);

	// Fall through

    transient RD_NDATA:

RD_ndat:

	RFM->col_data ();
	rc_endcycle;
}

#endif
