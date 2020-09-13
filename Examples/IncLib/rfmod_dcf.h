/*
	Copyright 1995-2018, 2019 Pawel Gburzynski

	This file is part of SMURPH/SIDE.

	SMURPH/SIDE is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	SMURPH/SIDE is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SMURPH/SIDE. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef __rfmod_dcf_h__
#define	__rfmod_dcf_h__

// Radio module (based on shadowing channel model, which can be easily replaced,
// if needed) implementing generic DCF of 802.11 and providing hooks for doing
// tricky things, like directional antennas, for example.

#include "wchansh.h"

#ifndef	DEBUGGING
#define	DEBUGGING	0
#endif

// Packet length in (info) bits excluding physical preambles, but including
// everything else (like the PLCP header)
#define	PKT_LENGTH_RTS	(20 * 8)
#define	PKT_LENGTH_CTS	(14 * 8)
#define	PKT_LENGTH_ACK	(14 * 8)

// These are global "constants" intialized from input data

extern TIME 	DCF_TSifs,	// SIFS time: one of two input parameters
		DCF_TDifs,	// DIFS: this is calculated from SIFS & SLot
		DCF_TEifs,	// So is this

		// Fixed NAV component starting from RTS, i.e., add to this the
		// transmission time of data packet
		DCF_NAV_rts,
		DCF_NAV_rts_auto,

		DCF_NAV_cts,	// fixed NAV component starting from CTS
		DCF_NAV_data,	// fixed NAV component starting from DATA

		DCF_TIMEOUT_bot,	// How long to wait for BOT after SIFS
		DCF_TIMEOUT_cts,	// How long to wait for CTS
		DCF_TIMEOUT_ack;	// How long to wait for ACK

extern double	DCF_DSlot;	// Slot time (in double ITUs for calculations)

extern RATE DCF_XRate;		// Transmission rate (should be same for all)

extern int DCF_CW_min, DCF_CW_max;	// Min/max window size

extern int DCF_RTR_short, DCF_RTR_long;	// Retransmission limits

extern Long DCF_RTS_ths;

void initDCF (
		double,		// SIFS	(in seconds)
		double,		// SLOT (in seconds)

		Long,		// RTS threshold: maximum packet length to
				// be sent with no handshake

		int,		// Short Retry Limit
		int, 		// Long Retry Limit

		int,		// CW_min
		int		// CW_max
	     );

// Event identifiers for xmitter and receiver processes; it makes sense to use
// pointers specific to the RFModule
#define	DCF_EVENT_ACT	((void*)(((char*)this) + 16))
#define	DCF_EVENT_BOT	((void*)(((char*)this) + 17))
#define	DCF_EVENT_SIL	((void*)(((char*)this) + 18))
#define	DCF_EVENT_CTS	((void*)(((char*)this) + 19))
#define	DCF_EVENT_ACK	((void*)(((char*)this) + 20))

// Packet flags
#define	DCFP_FLAG_RETRANS	0	// Bit number zero
#define	DCFP_FLAG_BCST		1	// Bit number 1, broadcast, e.g, HELLO

// Packet types: use large negative values for the special packets to leave
// room for any special packets needed by the "higher layers"
#define	DCFP_TYPE_SPECIAL_BASE	MININT
#define	DCFP_TYPE_RTS		(DCFP_TYPE_SPECIAL_BASE + 0)
#define	DCFP_TYPE_CTS		(DCFP_TYPE_SPECIAL_BASE + 1)
#define	DCFP_TYPE_ACK		(DCFP_TYPE_SPECIAL_BASE + 2)
#define	DCFP_TYPE_MIN_DATA	(DCFP_TYPE_SPECIAL_BASE + 3)
#define	DCFP_IS_DATA(p)		((p)->TP >= DCFP_TYPE_MIN_DATA)

packet DCFPacket {

	// All our packets are at least DCFPackets

	TIME	NAV;
	Long	DCFP_S, DCFP_R;		// Data-link sender-recipient
	FLAGS	DCFP_Flags;		// Flags (mostly for grabs)
};

#define	TheDCFP	((DCFPacket*)ThePacket)

#if DEBUGGING
// Tracing/debugging macros
#define	_trc(a, ...)	trace (a, ## __VA_ARGS__)
extern	char *_zz_dpk (DCFPacket*);
#define	_dpk(p)		_zz_dpk (p)
#else
#define	_trc(a, ...)	do { } while (0)
#define	_dpk(p)		0
#endif

mailbox PQueue (DCFPacket*) {

	// Queue of packets awaiting transmission

	inline Boolean queue (DCFPacket *t) {
		if (free ()) {
			put (t);
			return YES;
		}
		return NO;
	};
};

// A few subtypes

packet RTSPacket : DCFPacket {

	void setup (DCFPacket *p) {

		TIME tn;

		// RTS is setup from a data packet that we want to transmit
		// within the handshake

		DCFP_S = p -> DCFP_S;		// RTS sender = packet sender
		DCFP_R = p -> DCFP_R;		// same for recipient
		TLength = PKT_LENGTH_RTS;	// The length is fixed and known
		ILength = 0;			// No "info" bits

		// As a side effect, we set the NAV of the DATA packet here,
		// such that it will be ready when the DATA packet is eventually
		// transmitted. Note that DATA NAV is fixed.
		p->NAV = DCF_NAV_data;

		// And this is the NAV setting for RTS (see RFModule::RFmodule
		// in rfmod_dcf.cc)
		NAV = Ether->RFC_xmt (DCF_XRate, p) + DCF_NAV_rts;

		// Note: this setting is only used by CTS, as the RTS NAV is
		// assummed blindly to be DCF_NAV_rts_auto. We could optimize
		// things a bit with this assumption, but perhaps it makes
		// better sense to keep things simple.

		DCFP_Flags = 0;
		TP = DCFP_TYPE_RTS;
	};
};

packet CTSPacket : DCFPacket {

	void setup (Long snd, TIME dtt) {

		TIME tn;

		// This one is setup based on the RTS packet. The snd argument
		// is the RTS sender and dtt is the transmission time of the
		// pure (no phys preamble) DATA packet, which is obtained from
		// the RTS NAV vector (by subtracting DCF_NAV_rts - see
		// RFModule::RFModule).

		// CTS does not have this attribute
		DCFP_S = NONE;
		DCFP_R = snd;
		TLength = PKT_LENGTH_CTS;
		ILength = 0;

		// Update NAV. Note that we theoretically admit the
		// option that there is no NAV (TIME_0) - not used in the
		// present implementation.
		NAV = (dtt != TIME_0) ? dtt + DCF_NAV_cts : TIME_0;
		DCFP_Flags = 0;
		TP = DCFP_TYPE_CTS;
	};
};

packet ACKPacket : DCFPacket {

	void setup (Long snd) {

		// This one is created based on the sender of the DATA packet.
		// Note that an ACK packet has no NAV, or rather its NAV is 0;
		// OK, OK, I understand that we can have multiple exchanges
		// with long NAVs specifying intervals for multiple packets.
		// This can be easily accommodated later. First things first.

		DCFP_S = NONE;
		DCFP_R = snd;
		TLength = PKT_LENGTH_ACK;
		ILength = 0;
		NAV = TIME_0;
		DCFP_Flags = 0;
		TP = DCFP_TYPE_ACK;
	};
};

class	RFModule {

    // The is the model of the RF interface. It must be used as a prefix
    // class in an actual RFModule, because at least one of its virtual
    // methods must be redefined.

    friend class Xmitter;	// We run two processes with obvious purposes
    friend class Receiver;

    private:

	TIME	TEnav,		// Time when NAV expires
		TDExp,		// Time when data wait (after CTS) expires
		BKSet;		// Since when counting backoff

	Boolean	FLG_vbusy,
		FLG_garbage,
		FLG_backon,	// Backoff on
		FLG_backtv;	// In progress, i.e., waiting

	double	RSSI;		// Signal level of the packet being received

	PQueue *PQ;		// Queue for outgoing packets
	Station *S;		// Station owning the module

	int 	CWin,		// Current backoff window + 1
		BKSlots;	// Remaining backoff slots

    public:

	Transceiver *Xcv;

	// Backoff functions --------------------------------------------------

	inline void backoff_clear () {
		FLG_backon = NO;
		BKSlots = 0;
	};

	inline void backoff_reset () {
		// Note CWin is equal to current window + 1
		CWin = DCF_CW_min + 1;
	};

	inline void backoff_generate () {

		FLG_backon = YES;
		FLG_backtv = NO;
		BKSlots = (int) toss (CWin + 1);
		_trc ("BKF GEN: %1d slots", BKSlots);
	};

	inline TIME backoff_delay () {

		Assert (!FLG_backtv, "backoff_delay: double wait");
		FLG_backtv = YES;
		if (BKSlots) {
			// There is actually some backoff to wait
			BKSet = Time;
			_trc ("BKF DEL: %g [%g]", BKSlots * DCF_DSlot, 
				Itu * (BKSlots * DCF_DSlot));
			return (TIME) (BKSlots * DCF_DSlot);
		} else {
			_trc ("BKF DEL: NULL");
			return TIME_0;
		}
	};

	inline void backoff_stop () {

		Long nsl;

		if (FLG_backtv) {
			// How many slots have we been waiting?
			nsl = (Long) ceil ((double)(Time - BKSet) / DCF_DSlot);
			if (nsl >= BKSlots)
				BKSlots = 0;
			else
				BKSlots -= (int) nsl;
			_trc ("BKF STOP: waited %1d, left %1d", nsl, BKSlots);
			FLG_backtv = NO;
		}
	};

	inline void backoff_push () {

		if (CWin <= DCF_CW_max) {
			// Note that CWin is equal to window size + 1
			CWin = CWin + CWin - 1;
			if (CWin > DCF_CW_max + 1)
				CWin = DCF_CW_max + 1;
		}
		_trc ("BKF PUSH: cwin = %1d", CWin);
	};

	inline Boolean backoff_on () { return FLG_backon; };

	// --------------------------------------------------------------------

	// A bunch of virtual methods that allow the "upper layers" to 
	// intercept various potentially interesting events. Some of them
	// (well, most of them) will be useful for implementing directionality.
	// You see, directionality means gain, which is conveniently set
	// completely outside. But we need to intercept the exact moments
	// when that gain must be affected.

	// --- rcv group --- reception of various packets

	// This one is abstract, i.e., must be defined in a subtype
	virtual void UPPER_rcv_data (DCFPacket *p, double rssi) = 0;

	virtual void UPPER_rcv_rts (Long snd, double rssi) {
		// Called for every RTS reception with snd indicating the sender
	};

	virtual void UPPER_rcv_cts (Long snd, double rssi) {
		// Called for every (solicited) CTS reception with snd
		// indicating the sender. I say "solicited" because there may
		// be a spurios CTS for which we haven't sent an RTS. Such
		// a CTS is ignored without a trace.
	};

	virtual void UPPER_rcv_ack (Long snd, double rssi) {
		// Called for every (solicited) ACK reception with snd
		// indicating the sender
	};

	// --- snd group --- prelude to sending the various packets. Note:
	// this is where you can set the antennas before packet transmission.

	virtual void UPPER_snd_data (DCFPacket *p) {
		// About to transmit a data packet (pointed to by p)
	};

	virtual void UPPER_snd_rts (Long rcp) {
		// About to send an RTS to rcp
	};

	virtual void UPPER_snd_cts (Long rcp) {
		// About to send a CTS to rcp
	};

	virtual void UPPER_snd_ack (Long rcp) {
		// About to send an ACK to rcp
	};

	// --- snt group --- after sending the packets. Note: this is where you
	// can reset your antennas back to "neutral" or whatever.

	virtual void UPPER_snt_data (DCFPacket *p) {
		// sent a data packet (pointed to by p)
	};

	virtual void UPPER_snt_rts (Long rcp) {
		// an RTS to rcp
	};

	virtual void UPPER_snt_cts (Long rcp) {
		// send a CTS to rcp
	};

	virtual void UPPER_snt_ack (Long rcp) {
		// sent an ACK to rcp
	};

	// --- success group

	virtual void UPPER_suc_data (DCFPacket *p, int sh, int ln) {
		// Called after a successful packet transmission. I mean
		// DATA sent and acknowledged. Also, a broadcast packet (like
		// HELLO) sent (just sent - no ACK).
		// The default version deallocates the packet (as any version
		// should do at the end).
		delete p;
	};

	// --- collision group

	virtual void UPPER_col_data () {
		// Called when the data packet fails to arrive after CTS. See,
		// this is where you can reset the receive antenna prepared to
		// accept a DATA packet that hasn't made it. Note that when you
		// receive the DATA packet (as per rcv_data), you can do it
		// as well.
	};

	virtual void UPPER_col_cts (int sh, int ln) {
		// Called when CTS fails to arrive after RTS
	};

	virtual void UPPER_col_ack (int sh, int ln) {
		// Called when ACK fails to arrive after DATA
	};

	// --- failure group

	virtual void UPPER_fai_data (DCFPacket *p, int sh, int ln) {
		// Called when a data packet is dropped because of too many
		// retransmissions.
		delete p;
	};

	// --- end of virtual methods --- //

	inline DCFPacket *get_packet (int st) {
		// Acquire next packet for transmission
		DCFPacket *p;
		if (PQ->empty ()) {
			PQ->wait (NONEMPTY, st);
			// Note, we do return (no sleep) even if no packet is
			// available
			return NULL;
		}
		p = PQ->get ();
		// Make sure we are the MAC-level transmitter
		p->DCFP_S = S->getId ();
		return p;
	};

	inline Boolean busy () { return FLG_vbusy; };
	inline Boolean active () { return Xcv->busy (); };

	inline void xmit_data (DCFPacket *p, int st) {
		UPPER_snd_data (p);
		Xcv->transmit (p, st);
	};

	inline Boolean wait_activity_event (int st) {
		if (!FLG_vbusy) {
			Monitor->wait (DCF_EVENT_ACT, st);
			return NO;
		} else {
			return YES;
		}
	};

	inline Boolean wait_silence_event (int st) {
		if (FLG_vbusy) {
			Monitor->wait (DCF_EVENT_SIL, st);
			return NO;
		} else {
			return YES;
		}
	};

	inline void wait_cts_event (int suc, int irp, int fai) {
		Timer->wait (DCF_TIMEOUT_cts, fai);
		Monitor->wait (DCF_EVENT_CTS, suc);
		Monitor->wait (DCF_EVENT_BOT, irp);
	};

	inline void wait_ack_event (int suc, int irp, int fai) {
		Timer->wait (DCF_TIMEOUT_ack, fai);
		Monitor->wait (DCF_EVENT_ACK, suc);
		Monitor->wait (DCF_EVENT_BOT, irp);
	};

	inline void signal_bot () { Monitor->signal (DCF_EVENT_BOT); };
	inline void signal_cts () { Monitor->signal (DCF_EVENT_CTS); };
	inline void signal_ack () { Monitor->signal (DCF_EVENT_ACK); };

	inline void mark_activity () {
		if (!FLG_vbusy) {
			FLG_vbusy = YES;
			Monitor->signal (DCF_EVENT_ACT);
		}
		// Be pessimistic; EOT resets it
		FLG_garbage = YES;
	};

	inline Boolean mark_silence (int act, int sil) {

		if (FLG_vbusy) {
			if (Xcv->busy ())
				// To avoid race
				return YES;
			// To monitor traffic
			Xcv->wait (ACTIVITY, act);
			// Check if the NAV is set
			if (TEnav > Time) {
				// Retry when the NAV runs out, FLG_vbusy
				// remains set
				Timer->wait (TEnav - Time, sil);
				sleep;
			}
			// No NAV active, revert to idle
			FLG_vbusy = NO;
			Monitor->signal (DCF_EVENT_SIL);
		} else {
			// We are "virtually" idle; don't worry about races
			Xcv->wait (ACTIVITY, act);
		}
		return NO;
	};

	inline void xmit_rts (DCFPacket *p, int st) {

		RTSPacket *rts;

		rts = create RTSPacket (p);

		// Notify the "upper layers" - this is one of those virtual
		// (and empty-by-default) functions
		UPPER_snd_rts (rts->DCFP_R);

		// Start transmitting RTS
		Xcv->transmit (rts, st);
		// Delete the packet
		delete rts;
	};

	inline void set_nav (TIME nav) {

		TIME nn;

		if (nav != TIME_0) {
			if ((nn = Time + nav) > TEnav) {
				TEnav = nn;
				if (!FLG_vbusy) {
					FLG_vbusy = YES;
					Monitor->signal (DCF_EVENT_ACT);
				}
				_trc ("NAV: %1.0f [%8.6f] %8.6fs",
					(double) TEnav, ituToEtu (TEnav),
						ituToEtu (nav));
			}
		}
	};

	inline void xstop () {
		Xcv->stop ();
		// Do this after own transmission because Receiver won't
		// detect our own EOT
		FLG_garbage = 0;
	};

	inline void wait_reception (int s) {
		_trc ("WAIT RC: %1d, %8.6f", FLG_vbusy, ituToEtu (TEnav));
		Xcv->wait (ACTIVITY, s);
	};

	inline void wait_packet (int suc, int fai) {
		_trc ("WAIT PK: %1d, %8.6f", FLG_vbusy, ituToEtu (TEnav));
		Xcv->wait (BOT, suc);
		Xcv->wait (SILENCE, fai);
	};

	inline void init_reception (int st) {
		RSSI = Xcv->sigLevel (TheDCFP, SIGL_OWN);
		_trc ("INIT RC: %1d, %8.6f [%g] -> %s", FLG_vbusy,
			ituToEtu (TEnav), linTodB (RSSI), _dpk (TheDCFP));
		// Skipto
		Timer->wait (TIME_1, st);
	};

	inline void trace_packet (int suc, int fai) {
		Xcv->wait (EOT, suc);
		Xcv->wait (BERROR, fai);
		Xcv->wait (SILENCE, fai);
		// Just in case: a sanity check
		Xcv->wait (BOT, fai);
	};
		
	inline void receive () {
		_trc ("RECV %s", _dpk (TheDCFP));
		UPPER_rcv_data (TheDCFP, RSSI);
	};

	inline Boolean my_packet (DCFPacket *p) {
		// Check if we are the recipient of this data packet
		return p->DCFP_R == S->getId ();
	};

	inline Boolean xmit_cts (Long sn, TIME nv, int st) {

		// Send CTS. Note that nv is the DATA transmission time
		// recovered from RTS NAV (see receiver, state RC_RCVD,
		// case DCFP_TYPE_RTS).
		CTSPacket *cts;

		// Optional ?
		// if (Xcv->busy ())
		// 	return NO;

		if (TEnav > Time) {
			// cannot send the CTS because of NAV: do nothing
			// then
			_trc ("CANT CTS %f [%8.6f]", (double) TEnav,
				ituToEtu (TEnav));
			return NO;
		}

		// Go ahead
		cts = create CTSPacket (sn, nv);
		// Message to upper layers
		UPPER_snd_cts (sn);
		_trc ("SENDING CTS");
		Xcv->transmit (cts, st);
		delete cts;
		return YES;
	};

	inline void wait_bot_event (int suc, int fai) {
		Timer->wait (TDExp - Time, fai);
		Monitor->wait (DCF_EVENT_BOT, suc);
	};

	inline void wait_bot (int suc, int fai) {
		Timer->wait (TDExp - Time, fai);
		Xcv->wait (BOT, suc);
	};

	inline void xmit_ack (Long sn, int st) {

		// Send ACK; done by the receiver.
		ACKPacket *ack;

		ack = create ACKPacket (sn);
		UPPER_snd_ack (sn);
		_trc ("SENDING ACK");
		Xcv->transmit (ack, st);
		delete ack;
	};

	inline Boolean send (DCFPacket *p) {
		// Called by upper layers to deposit a packet for transmission
		p->NAV = TIME_0;
		return PQ->queue (p);
	};

	// Checks is the queue can acommodate a new packet
	inline Long room () { return PQ->free (); };

	RFModule (Transceiver*, Long);
};

process Xmitter {

	RFModule *RFM;
	DCFPacket *CP;		// Packet currently being transmitted
	int RetrS, RetrL;	// Retransmission counters

	states {
		XM_READY,	// Assumed every time backoff runs out
		XM_RTSD,	// RTS sent
		XM_WCTB,	// Wait for CTS BOT
		XM_WCTS,	// Wait for CTS reception
		XM_CTSN,	// BOT while waiting for CTS reception
		XM_COLC,	// CTS timeout
		XM_CTSD,	// CTS received
		XM_DATA,	// Start xmit data packet (in handshake)
		XM_DATD,	// Data transmission done (handshake case)
		XM_WACB,	// Wait for ACK BOT
		XM_WACK,	// Wait for ACK reception
		XM_ACKN,	// BOT while waiting for ACK
		XM_COLD,	// ACK timeout
		XM_BACK,	// Start or continue backoff
		XM_BUSY,	// Channel sensed busy (logically)
		XM_ACKD,	// ACK received
		XM_SLOT,	// Slot advancement after own transmission
		XM_IDLE,	// Channel sensed idle (logically)
		XM_ESHRT,	// End xmit short data (no handshake)
		XM_WASB,	// Wait for ACK (short) BOT
		XM_WASK,	// Wait for ACK reception
		XM_ASKN,	// BOT while waiting for ACK reception
		XM_COLS,	// ACK timeout for short data
		XM_EBCST	// End xmit broadcast data
	};

	perform;

	void setup (RFModule *rfm) {
		CP = NULL;
		RFM = rfm;
		// Not needed any more, done by the channel
		// RFM->Xcv->setAevMode (NO);
		// RFM->Xcv->setMinDistance (SEther->RDist);
	};
};

process Receiver {

	RFModule *RFM;
	Long CS;	// RTS sender
	TIME NV;	// DATA transmission time extracted from RTS NAV

	states {
		RC_LOOP,	// Main loop for monitoring channel events
		RC_ACTIVITY,	// Physical activity detected
		RC_STILLACTIVE,	// New phys activity part of same logical act.
		RC_SILENCE,	// Physical silence
		RC_PACKET,	// Beginning of a packet
		RC_GETPACKET,	// Start packet reception
		RC_RECEIVED,	// Packet successfully received
		RC_ABORTED,	// Reception error
		RC_SCTS,	// Send CTS
		RC_SCTSD,	// CTS transmitted
		RC_WDATA,	// Wait for DATA BOT
		RC_DATAP,	// Beginning of expected data packet
		RC_GETDATA,	// Start reception of expected data packet
		RC_DATAABT,	// Expected data packet aborted
		RC_NODATA,	// Timeout while waiting for data packet
		RC_DATARCV,	// Packet received (expected data)
		RC_SACK,	// Send ACK
		RC_SACKD	// ACK sent
	};

	perform;

	void setup (RFModule *rfm) { RFM = rfm; };
};

#endif
