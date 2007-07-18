#ifndef __rfmod_dcf_h__
#define	__rfmod_dcf_h__

// Radio module (based on shadowing channel model, which can be easily replaced,
// if needed) implementing generic DCF of 802.11 and providing hooks for doing
// tricky things, like directional antennas, for example.

#include "wchansh.h"

// Set these to 1 for tracing
#define	TRACE_RFMODULE		0
#define	TRACE_COLLISIONS	0

// Packet length in (info) bits excluding physical preambles, but including
// everything else
#define	PKT_LENGTH_RTS	(20 * 8)
#define	PKT_LENGTH_CTS	(14 * 8)
#define	PKT_LENGTH_ACK	(14 * 8)

// These are global "constants" intialized from input data

extern TIME 	DCF_TDifs, 	// DIFS (in ITU)
		DCF_TSifs, 	// SIFS
		DCF_TSifs2,	// Precalculated 2 * SIFS
		DCF_TEifs, 	// EIFS

		// Extra NAV margin for safety (in ITU). This is added to every
		// NAV calculated based on the packet timing.
		DCF_NAV_delta,

		// Fixed NAV component starting from RTS, i.e., add to this the
		// transmission time of data packet; see rfmod_dcf.cc for more
		// explanation
		DCF_NAV_rts,	// fixed NAV component starting from RTS
		DCF_NAV_cts,	// fixed NAV component starting from CTS
		DCF_NAV_data,	// fixed NAV component starting from DATA
		DCF_TIMEOUT_cts,	// How long to wait for CTS
		DCF_TIMEOUT_ack,	// How long to wait for ACK
		DCF_PRE_time;		// Physical preamble transmission time

extern double	DCF_Slot;	// Slot time in seconds

extern RATE DCF_XRate;		// Transmission rate (should be same for all)

extern Long DCF_CW_min, DCF_CW_max;	// Min/max window size

// Initalizes shared (global) parameters of the DCF scheme
void initDCF (
		double,		// SIFS	(in seconds)
		double,		// DIFS 
		double,		// EIFS 
		double,		// NAV delta (safety margin, in seconds)
		double,		// Slot size (in seconds)
		Long,		// CW_min (in slots)
		Long		// CW_max (in slots)
	     );

// Event identifiers for xmitter and receiver processes
#define	DCF_EVENT_CTS	((void*)&TAccs)
#define	DCF_EVENT_ACK	((void*)&PQ)
#define	DCF_EVENT_SIL	((void*)&TIdle)
#define	DCF_EVENT_ACT	((void*)&S)

// Packet flags
#define	DCFP_FLAG_RETRANS	0	// Bit number zero
#define	DCFP_FLAG_BCST		1	// Bit number 1, broadcast, e.g, HELLO

// Packet types
#define	DCFP_TYPE_DATA		0
#define	DCFP_TYPE_RTS		1
#define	DCFP_TYPE_CTS		2
#define	DCFP_TYPE_ACK		3

// These are options of the backoff function. All descriptions I could find
// were somewhat evasive, i.e., they looked nice on paper, but the
// implementation was far from clear. Thus, I am leaving some room for
// creativity.
#define	BKF_BUSY		0	// Backoff after channel busy
#define	BKF_COLL		1	// Backoff after collision
#define	BKF_NAV			2	// Backoff after NAV expiration

#if TRACE_RFMODULE
#undef	TRACE_COLLISIONS
#define	TRACE_COLLISIONS	1

#define trc_pkt(p,hdr) \
		trace (hdr ", DCFPKT: T %1d [%1d], F %1x, S %1d [%1d], " \
			"R %1d [%1d], L %1d, N %gus", \
				(p)->TP, (p)->DCFP_Type, (p)->DCFP_Flags, \
				(p)->Sender, (p)->DCFP_S, \
				(p)->Receiver, (p)->DCFP_R, \
				(p)->TLength, \
				ituToEtu ((p)->NAV) * 1000000.0)

#define	trc_tim(hdr) \
		trace (hdr ", TIMERS: TA %g, TE %g, TI %g, TB %g, CW %1d", \
			ituToEtu (TAccs), ituToEtu (TEnav), \
			TIdle == TIME_inf ? -1.0 : ituToEtu (TIdle), \
			ituToEtu (TBusy), CWin)

#define	trc_sig(hdr)	trace (hdr ", SIGNAL SENT")

#else

#define trc_pkt(p,hdr) 	do { } while (0)
#define trc_tim(hdr) 	do { } while (0)
#define trc_sig(hdr) 	do { } while (0)

#endif

#if	TRACE_COLLISIONS
#define trc_col(p,hdr) \
		trace (hdr ", R %1d [%1d], L %1d", \
			(p)->Receiver, \
			(p)->DCFP_R, \
			(p)->TLength)

#define trc_unx(hdr)	 trace (hdr)
#else
#define trc_col(p,hdr) 	do { } while (0)
#define trc_unx(hdr)	do { } while (0)
#endif

packet DCFPacket {

	// All our packets are at least DCFPackets

	TIME	NAV;
	Long	DCFP_S, DCFP_R;		// Data-link sender-recipient
	unsigned short DCFP_Flags;	// Flags (mostly for grabs)
	unsigned short DCFP_Type;	// Internal (Data-link) type
};

#define	TheDCFP	((DCFPacket*)ThePacket)

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

packet RTSPacket : DCFPacket {		// RTS

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
		NAV = Ether->RFC_xmt (DCF_XRate, p->TLength) + DCF_NAV_rts;
		DCFP_Flags = 0;
		DCFP_Type = DCFP_TYPE_RTS;
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
		DCFP_Type = DCFP_TYPE_CTS;
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
		DCFP_Type = DCFP_TYPE_ACK;
	};
};

class	RFModule {

    // The is the model of the RF interface. It must be used as a prefix
    // class in an actual RFModule, because at least one of its virtual
    // methods must be redefined.

    friend class Xmitter;	// We run two processes with obvious purposes
    friend class Receiver;

    private:

	TIME	TAccs,		// Time when backoff expires
		TEnav,		// Time when NAV expires

		// Note: we need the next two to properly monitor backoff time,
		// which, according to my flimsy understanding, is not supposed
		// to flow while the medium is sensed busy.

		TIdle,		// Starting time of current IDLE period
		TBusy,		// Starting time of current BUSY period

		// The next two ones are needed by the receiver to properly
		// diagnose a lost data packet, i.e., one that is expected
		// because we have sent CTS for it. I didn't need anything
		// like that in a previous version (and the receiver was
		// considerably simpler), but when contemplating quirks for
		// directional antenna settings, this came up. And, mind you,
		// it is only required when we allow directional reception.
		// Why, you ask. Because you may have set the receive antenna
		// for the expected DATA packet, and then it might get lost.
		// Normally it's no big deal. An omni receiver needs no special
		// logic to handle such a case. But when you have set the
		// directional receive antenna for the packet, and the packet
		// won't make it, you must know when to re-set the antenna to
		// "normal".

		TData,		// Time when data wait expires (if activity)
		TDats;		// Time when data wait expires (if silence)

	double	RSSI;		// Signal level of the packet being received

	Transceiver *Xcv;

	PQueue *PQ;		// Queue for outgoing packets
	Station *S;		// Station owning the module

	Long CWin;		// Current backoff window

    public:

	// Maximum number of retransmissions. I don't think we want to change
	// this dynamically, but just in case, I have turned it into a public
	// attribute, so the "upper layers" can change it if really needed.
	Long RMax;

	// A bunch of virtual methods that allow the "upper layers" to 
	// intercept various potentially interesting events. Some of them
	// (well, most of them) will be useful for implementing directionality.
	// You see, directionality means gain, which is conveniently set
	// completely outside. But we need to intercept the exact moments
	// when that gain must be affected.

	// --- rcv group --- reception of various packets

	virtual void rcv_data (DCFPacket *p, double rssi) {
		// Called for every reception of a data packet. This one is
		// needed to make any sense at all. let us diagnose its
		// absence.
		excptn ("RFModule: pkt_rcv must be defined in my subclass");
	};

	virtual void rcv_rts (Long snd, double rssi) {
		// Called for every RTS reception with snd indicating the sender
	};

	virtual void rcv_cts (Long snd, double rssi) {
		// Called for every (solicited) CTS reception with snd
		// indicating the sender. I say "solicited" because there may
		// be a spurios CTS for which we haven't sent an RTS. Such
		// a CTS is ignored without a trace.
	};

	virtual void rcv_ack (Long snd, double rssi) {
		// Called for every (solicited) ACK reception with snd
		// indicating the sender
	};

	// --- snd group --- prelude to sending the various packets. Note:
	// this is where you can set the antennas before packet transmission.

	virtual void snd_data (DCFPacket *p) {
		// About to transmit a data packet (pointed to by p)
	};

	virtual void snd_rts (Long rcp) {
		// About to send an RTS to rcp
	};

	virtual void snd_cts (Long rcp) {
		// About to send a CTS to rcp
	};

	virtual void snd_ack (Long rcp) {
		// About to send an ACK to rcp
	};

	// --- snt group --- after sending the packets. Note: this is where you
	// can reset your antennas back to "neutral" or whatever.

	virtual void snt_data (DCFPacket *p) {
		// sent a data packet (pointed to by p)
	};

	virtual void snt_rts (Long rcp) {
		// an RTS to rcp
	};

	virtual void snt_cts (Long rcp) {
		// send a CTS to rcp
	};

	virtual void snt_ack (Long rcp) {
		// sent an ACK to rcp
	};

	// --- success group

	virtual void suc_data (DCFPacket *p, Long retrans) {
		// Called after a successful packet transmission. I mean
		// DATA sent and acknowledged. Also, a brodcast packet (like
		// HELLO) sent (just sent - no ACK for it).

		// The default version deallocates the packet (as any version
		// should do at the end).
		delete p;
	};

	// --- collision group

	virtual void col_data () {
		// Called when the data packet fails to arrive after CTS. See,
		// this is where you can reset the receive antenna prepared to
		// accept a DATA packet that hasn't made it. Note that when you
		// receive the DATA packet (as per rcv_data), you can do it
		// as well.
	};

	virtual void col_cts () {
		// Called when CTS fails to arrive after RTS
	};

	virtual void col_ack () {
		// Called when ACK fails to arrive after DATA
	};

	// --- failure group

	virtual void fai_data (DCFPacket *p) {
		// Called when a data packet is dropped because of too many
		// retransmissions.
		delete p;
	};

	// --- end of virtual methods --- //

	// For the following functions (used by the processes) the comment in
	// the right margin indicates which process needs it.
	// XMT - xmitter, RCV - receiver, XCV - both.

	inline DCFPacket *getp (int st) {				// [XMT]
		// Acquire next packet for transmission
		if (PQ->empty ()) {
			PQ->wait (NONEMPTY, st);
			sleep;
		}
		return PQ->get ();
	};

	inline void bckf (int sts, int stb) {				// [XMT]

		// Check if can grab the medium and transmit, or delay
		// accordingly to backoff, nav, whatever

		// sts - state to go to try again
		// stb - state to generate backoff

		TIME t;

		if (TIdle == TIME_inf) {
			// Medium is busy (TIdle == TIME_inf can be viewed as
			// a flag). Note: in xmitter we do not sense the
			// medium directly (to avoid races), but rely on the
			// receiver's perception. The receiver is supposed to
			// communicate to us status changes. That complicates
			// the receiver, but simplifies the transmitter. I
			// wanted to avoid a third process, which would do it
			// more elegantly - perhaps I shouldn't have.
			trc_tim ("BCKF BUSY");
			// Wait for a SILENCE event from the receiver and
			// generate backoff. We are supposed to back off after
			// perceiving the medium busy, right?
			Monitor->wait (DCF_EVENT_SIL, stb);
			sleep;
		}
		if (Time < TAccs) {
			// Waiting for backoff (TAccs is the earliest moment
			// we are allowed to go)
			trc_tim ("BCKF BACK");
			// Wait until allowed and try again
			Timer->wait (TAccs - Time, sts);
			sleep;
		}
		if (Time < TEnav) {
			// Just in case: shouldn't normally happen as it
			// implies backoff, or doesn't it ... ?
			// This means we are waiting for NAV (TEnav is the
			// time it will expire)
			trc_tim ("BCKF NAV");
			// Wait and try again
			Timer->wait (TEnav - Time, sts);
			sleep;
		}
		// Mark: no backoff in effect, i.e., we have served it. TAccs
		// is also used as a flag. If nonzero, it means that there was
		// a backoff. If TAccs >= Time, we are still backed off, if not,
		// we are past the backoff. By setting TAccs to zero, we flag
		// that the last backoff time has been served. This is used in
		// genb (see below).
		TAccs = TIME_0;
		if ((t = TIdle + DCF_TDifs) > Time) {
			// We need at least DIFS of silence. TIdle is the time
			// the medium was last sensed idle (note that it is
			// idle now).
			trc_tim ("BCKF DIFS");
			Timer->wait (t - Time, sts);
			// If the medium becomes busy in the meantime, we should
			// back off (this is why we go to state stb). Note that,
			// according to my understanding, this will be viewed as
			// a new collision (and trigger a new backoff - TAccs ==
			// 0).
			Monitor->wait (DCF_EVENT_ACT, stb);
			sleep;
		}
		trc_tim ("BCKF OK");
		// When we get here, we are allowed to send an RTS (or transmit
		// a broadcast packet)
	};

	inline void rstb () {
		// Reset backoff parameters, i.e., CWin. It is set to NONE
		// to allow genb (see below) to be more flexible.
		CWin = NONE;
		trc_tim ("RSTB");
	};

	inline void genb (int ord) {
		// This one generates a backoff delay. The argument identifies
		// the circumstance.
		if (TAccs != TIME_0) {
			// Do nothing: honor the backoff currently pending. This
			// means that the "busy" status of the medium is not a
			// new "collision" but has been perceived while waiting
			// for some timers to expire.
			trc_tim ("GENB IGN");
			return;
		}
		if (CWin == NONE) {
			// Restart CWin from the minimum, if reset
			CWin = DCF_CW_min;
		} else if (ord != BKF_NAV) {
			// I am assuming that NAV delays do not inflate CWin
			// (are not viewed as cummulative collisions)
			if ((CWin = CWin + CWin) > DCF_CW_max)
				// Otherwise, the window is doubled until
				// CW_max
				CWin = DCF_CW_max;
		}
		TAccs = Time +
		etuToItu ((double)lRndUniform ((LONG)0, (LONG)CWin) * DCF_Slot);
		trc_tim ("GENB");
	};

	inline Boolean busy () { return Xcv->busy (); };		// [RCV]

	inline void snav (TIME nav) {					// [RCV]
		// Set NAV delay (after receiving a packet specifying NAV)
		TIME t;
		if (nav != TIME_0) {
			// Only if there is a NAV to honor
			t = Time + nav;
			if (TEnav < t) {
				// If we don't have a longer NAV already active
				TEnav = t;
				// Correct me if I am wrong, but the way I
				// understand it, we need a backoff after his,
				// i.e., after resetting the NAV
				genb (BKF_NAV);
			}
		}
		trc_tim ("SNAV");
	};
		
	inline void xrts (DCFPacket *p, int st) {			// [XMT]
		// Send RTS, go to st when done
		RTSPacket *rts;
		rts = create RTSPacket (p);
		// Notify the "upper layers" - this is one of those virtual
		// (and empty-by-default) functions
		snd_rts (rts->DCFP_R);
		// Start transmitting RTS
		Xcv->transmit (rts, st);
		trc_tim ("XRTS");
		// Delete the packet
		delete rts;
	};

	inline Boolean xcts (Long sn, TIME nv, int st) {		// [RCV]

		// Send CTS. Note that nv is the DATA transmission time
		// recovered from RTS NAV (see receiver, state RC_RCVD,
		// case DCFP_TYPE_RTS).
		CTSPacket *cts;

		if (Time < TEnav || busy ())
			// cannot send the CTS because of NAV: do nothing
			// then
			return NO;
		// Go ahead
		cts = create CTSPacket (sn, nv);
		// Message to upper layers
		snd_cts (sn);
		Xcv->transmit (cts, st);
		// This is not needed, but doesn't hurt
		TIdle = TIME_inf;
		// TBusy is set while we are doing this because we are the
		// receiver responding to RTS reception (and we are in charge
		// of the medium status as perceived by the transmitter).
		trc_tim ("XCTS");
		delete cts;
		return YES;
	};

	inline void xack (Long sn, int st) {				// [RCV]
		// Send ACK. Again, this is done by the receiver.
		ACKPacket *ack;
		ack = create ACKPacket (sn);
		snd_ack (sn);
		Xcv->transmit (ack, st);
		// This is not needed, but doesn't hurt
		TIdle = TIME_inf;
		// TBusy is set while we are doing this (see above)
		trc_tim ("XACK");
		delete ack;
	};

	inline void xstop () {						// [XCV]
		Xcv->stop ();
		trc_tim ("XSTOP");
	};

	inline void wcts (int sts, int stf) {				// [XMT]
		// Wait for CTS: sts <- success, stf <- timeout
		Monitor->wait (DCF_EVENT_CTS, sts);	// Event from receiver
		Timer->wait (DCF_TIMEOUT_cts, stf);	// Timeout
		trc_tim ("WCTS");
	};

	inline void sifs (int sts, int stf) {				// [XMT]
		// Delay for SIFS while monitoring medium status: sts <- OK,
		// stf <- busy
		Timer->wait (DCF_TSifs, sts);
		Xcv->wait (ACTIVITY, stf);
		trc_tim ("SIFS");
	};

	inline void xmit (DCFPacket *p, int st) {			// [XCV]
		// Transmit a DATA packet: st <- done. Note that, if we are
		// xmitter, our own reciver will perceive this as an activity
		// and update the chanel status accordingly. When the receiver
		// transmits, it simply neglects to notify the transmitter
		// that there was an opportunity. As perceived by the
		// transmitter, the channel remains busy. That always happens
		// inside a handshake (CTS, ACK) and is the right thing to do.
		snd_data (p);		// Notify upper layers
		Xcv->transmit (p, st);
		trc_tim ("XMIT");
	};

	inline void wack (int sts, int stf) {				// [XMT]
		// Wait for ACK: sts <- success, stf <- timeout
		Monitor->wait (DCF_EVENT_ACK, sts);
		Timer->wait (DCF_TIMEOUT_ack, stf);
		trc_tim ("WACK");
	};

	inline void wact (int sta) {					// [RCV]
		// Wait for activity in the medium
		Xcv->wait (ACTIVITY, sta);
		trc_tim ("WACT");
	};

	inline void sact () {						// [RCV]
		// Called by the receiver on ACTIVITY to update the medium
		// status and convey the event to xmitter
		if (TIdle != TIME_inf) {
			// This means that the activity actually changes
			// something, i.e., we were idle; send a signal to
			// xmitter
			Monitor->signal (DCF_EVENT_ACT);
			// Mark the medium as busy
			TIdle = TIME_inf;
			// New busy start time
			TBusy = Time;
			trc_sig ("SACT");
			return;
		}
		// If TIdle == TIME_inf, we may be in NAV. This new activity
		// does not change anything, and the TBusy setting at the
		// beginning of NAV is honored until the NAV expires.
		trc_tim ("SACT IG");
	};

	inline Boolean ssil (int sac, int sna) {			// [RCV]
		// Used by the receiver, at the front of its main loop,
		// whenever the channel is sensed idle: sac <- activity sensed,
		// sna <- retry; returns: YES if we are in fact busy (only
		// possible after NAV retry), NO if OK.
		if (TEnav > Time) {
			// NAV in effect: make sure that you will wake up when
			// NAV expires, even if the medium does not become busy
			// in the meantime. This is because we will have to
			// signal xmitter at that moment. We are a bit messed
			// up by this extra duty, but:
			// 1. this way we avoid races and simplify xmitter
			// 2. we avoid a third process that could handle such
			//    issues more elegantly (but somewhat expensively,
			//    I am not sure now).
			Timer->wait (TEnav - Time, sna);
			// ... and do not mark the channel as IDLE until
			// the NAV expires
			trc_sig ("SSIL NA");
		} else {
			if (Xcv->busy ()) {
				// Means we are busy simultaneously with NAV
				// expiration. This isn't needed, I believe,
				// but let us be careful about races.
				trc_sig ("SSIL B");
				return YES;
			}
			// Now we have truly become IDLE
			TIdle = Time;
			if (TAccs != TIME_0)
				// This is how much time has elapsed since we
				// became busy, we push TAccs by that much.
				// This is because busy time is subtracted from
				// backoff waiting (if I got that right).
				TAccs += Time - TBusy;
			// Signal the other guy we are idle
			Monitor->signal (DCF_EVENT_SIL);
			trc_sig ("SSIL");
		}
		// Keep monitoring for ACTIVITY
		Xcv->wait (ACTIVITY, sac);
		// Note: state 'sac' is the starting point for trying to
		// receive a packet
		return NO;
	};

	inline void wbot (int stb, int sts) {				// [RCV]
		// Wait for the beginning of a packet (following ACTIVITY)
		Xcv->wait (BOT, stb);
		Xcv->wait (SILENCE, sts);
		trc_tim ("WBOT");
	};

	inline Boolean dsil (int sac, int snf) {			// [RCV]

		// This is a variant of ssil used by the "second-stage"
		// receiver, whose role is to specifically receive a DATA
		// packet following CTS. The problem is hinted at in class
		// RFModule. We have to figure out when we can safely
		// (but as early as possible) proclaim that the DATA packet 
		// won't make it.
		// 
		// sac <- activity sensed
		// snf <- DATA packet lost
		// value: YES -> silence timer already expired
		//
		// The primary difference wrt ssil is that now we don't have to
		// convey channel status changes to the transmitter (which is
		// left believing that the channel is busy) until we have
		// figured out what happens to the DATA packet), but simply
		// detect activity, silence, and timeouts.
		//
		// If there is silence, the timeout need not exceed SIFS * 2.
		// But when there is an activity, the timeout is SIFS * 2 +
		// preamble time (see wbod below). This is because you may
		// have some garbage (which need not affect reception),
		// followed by the preamble, and then the DATA packet. The
		// garbage may technically take up to SIFS * 2 and then be
		// followed by a preamble.
		//
		// Thus, this is how it works: if, following the last bit of
		// CTS, there is a silence of SIFS * 2, abort DATA reception
		// and conclude that the packet has not made it.
		// If there is an ACTIVITY, wait for SIFS * 2 + preamble time.
		// If there is no packet beginning (BOT) recognized by then,
		// the packet has't made it. Otherwise, wait until the packet
		// develops into 1) a succesful reception. If it is a DATA
		// packet, fine, otherwise, we have received something else
		// and the DATA packet hasn't made it after all. 2) aborted
		// reception: the DATA packet clearly hasn't made it. To make
		// it trickier, when you revert to silence after some short
		// garbage, you have to be within the short timeout.

		if (TData <= Time)
			// Silence timer expired
			return YES;

		Timer->wait (TData - Time, snf);
		Xcv->wait (ACTIVITY, sac);
		return NO;
	};

	inline Boolean wbod (int stb, int sts) {			// [RCV]

		// This is a variant of wbot used by the second-stage DATA
		// receiver (see above).

		if (TData + DCF_PRE_time <= Time)
			// Note that the timeout is now increased by the
			// preamble time
			return YES;

		Xcv->wait (BOT, stb);
		Xcv->wait (SILENCE, sts);
		trc_tim ("WBOD");
		return NO;
	};

	inline void strc (int sts) {					// [RCV]
		// Start reception after beginning of packet has been sensed
		RSSI = Xcv->sigLevel (TheDCFP, SIGL_OWN);

		// follow not required any more if we are after BOT, which sets
		// the 'followed' packet automatically these days

		// Xcv->follow (TheDCFP);

		// Simulated skipto -> sts
		Timer->wait (TIME_1, sts);
		trc_pkt (TheDCFP, "STRC");
	};

	inline void wrec (int str, int sta) {				// [RCV]
		// Wait for the outcome of packet reception:
		// str <- received OK, sta <- aborted
		Xcv->wait (EOT, str);
		Xcv->wait (BERROR, sta);
		Xcv->wait (SILENCE, sta);
		// This one is just in case: recognizable BOT within an
		// unaborted packet should not be considered normal
		Xcv->wait (BOT, sta);
		trc_tim ("WREC");
	};

	inline void scts () {						// [RCV]
		// Signal CTS reception
#if TRACE_COLLISIONS
		// if (Monitor->signalP (DCF_EVENT_CTS) == REJECTED)
		if (Monitor->signal (DCF_EVENT_CTS) != 1)
			// This means spurious: xmitter has not ordered it
			trc_unx ("SPURIOUS CTS SIGNAL");
#else
		Monitor->signal (DCF_EVENT_CTS);
		// Monitor->signalP (DCF_EVENT_CTS);
#endif
		trc_tim ("SCTS");
	};

	inline void sack () {						// [RCV]
		// Signal ACK reception
#if TRACE_COLLISIONS
		// if (Monitor->signalP (DCF_EVENT_ACK) == REJECTED)
		if (Monitor->signal (DCF_EVENT_ACK) != 1)
			// Spurious
			trc_unx ("SPURIOUS ACK SIGNAL");
#else
		Monitor->signal (DCF_EVENT_ACK);
		// Monitor->signalP (DCF_EVENT_ACK);
#endif
		trc_tim ("SACK");
	};

	inline Boolean ismy (DCFPacket *p) {				// [RCV]
		// I am the data-link recipient of this packet
		return p->DCFP_R == S->getId ();
	};

	inline void recv () {						// [RCV]
		// Receive a data packet
		trc_pkt (TheDCFP, "RECV");
		trc_tim ("RECV");
		// This is one of those upper-layer interface methods
		rcv_data (TheDCFP, RSSI);
	};

	inline Boolean send (DCFPacket *p) {
		// Called by upper layers to deposit a packet for transmission
		p->NAV = TIME_0;
		trc_pkt (p, "SEND");
		trc_tim ("SEND");
		return PQ->queue (p);
	};

	// Checks is the queue can acommodate a new packet
	inline Long room () { return PQ->free (); };

	RFModule (Transceiver*,
			Long,			// Outgoing packet queue size
			Long			// Maximum retransmissions
		 );
};

process Xmitter {

	RFModule *RFM;
	DCFPacket *CP;	// Packet currently being transmitted
	int Retrans;	// Number of retransmissions

	states { XM_LOOP, XM_TRY, XM_BACK, XM_RTS, XM_DATA, XM_XMT,
		    XM_NOCTS, XM_SBUSY, XM_WAK, XM_DONE, XM_EBCST, XM_COLL };

	perform;

	void setup (RFModule *rfm) { RFM = rfm; };
};

process Receiver {

	RFModule *RFM;
	Long CS;	// RTS sender
	TIME NV;	// DATA transmission time extracted from RTS NAV

	states { RC_LOOP, RC_ACT, RC_SIL, RC_BOT, RC_RCV, RC_RCVD, RC_RABT,
		RC_SCTS, RC_SCTSD, RD_ACT, RD_SIL, RD_BOT, RD_RCV, RD_RCVD,
			RD_RABT, RD_NDATA, RD_SACK, RD_SACKD };

	perform;

	void setup (RFModule *rfm) { RFM = rfm; };
};

#endif
