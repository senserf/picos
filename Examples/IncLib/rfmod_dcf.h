#ifndef __rfmod_dcf_h__
#define	__rfmod_dcf_h__

// Radio module (based on shadowing channel model, which can be easily replaced,
// if needed) implementing generic DCF of 802.11

#include "wchansh.h"

#define	TRACE_RFMODULE		0
#define	TRACE_COLLISIONS	0

#define	PKT_LENGTH_RTS	(20 * 8)
#define	PKT_LENGTH_CTS	(14 * 8)
#define	PKT_LENGTH_ACK	(14 * 8)

extern TIME 	DCF_TDifs, 	// DIFS (in ITU)
		DCF_TSifs, 	// SIFS
		DCF_TEifs, 	// EIFS
		DCF_NAV_delta, 	// extra NAV margin for safety (in ITU)
		DCF_NAV_rts,	// fixed NAV component starting from RTS
		DCF_NAV_cts,	// fixed NAV component starting from CTS
		DCF_NAV_data,	// fixed NAV component starting from DATA
		DCF_TIMEOUT_cts,	// How long to wait for CTS
		DCF_TIMEOUT_ack;	// How long to wait for ACK

extern double	DCF_Slot;

extern RATE DCF_XRate;		// Transmission rate (should be same for all)

extern Long DCF_CW_min, DCF_CW_max;

// Initalizes shared (global) parameters of the DCF algorithm
void initDCF (
		double,		// SIFS	(in seconds)
		double,		// DIFS 
		double,		// EIFS 
		double,		// NAV delta (safety margin, in seconds)
		double,		// Slot size (in seconds)
		Long,		// CW_min (in slots)
		Long		// CW_max (in slots)
	     );

#define	DCF_EVENT_CTS	((void*)&TAccs)
#define	DCF_EVENT_ACK	((void*)&PQ)
#define	DCF_EVENT_SIL	((void*)&TIdle)
#define	DCF_EVENT_ACT	((void*)&S)

#define	DCFP_FLAG_RETRANS	0	// Bit number zero
#define	DCFP_FLAG_BCST		1	// Broadcast, e.g, HELLO

#define	DCFP_TYPE_DATA		0
#define	DCFP_TYPE_RTS		1
#define	DCFP_TYPE_CTS		2
#define	DCFP_TYPE_ACK		3

#define	BKF_BUSY		0	// Backoff type oridnals
#define	BKF_COLL		1
#define	BKF_NAV			2

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

	// MAC-level sender/recipient
	TIME	NAV;
	Long	DCFP_S, DCFP_R;
	unsigned short DCFP_Flags;
	unsigned short DCFP_Type;
};

#define	TheDCFP	((DCFPacket*)ThePacket)

// Packet return codes
#define	PKT_RET_RECEIVED	0
#define	PKT_RET_SENT		1
#define	PKT_RET_DROPPED		2

// Signature of the packet return function:
//	- packet pointer
//	- what happend (PKT_RET_...)
//	- other parameters depend on the code
typedef void (*PRETFUN)(DCFPacket*, int, ...);

mailbox PQueue (DCFPacket*) {

	inline Boolean queue (DCFPacket *t) {
		if (free ()) {
			put (t);
			return YES;
		}
		return NO;
	};
};

packet RTSPacket : DCFPacket {

	void setup (DCFPacket *p) {

		TIME tn;

		DCFP_S = p -> DCFP_S;
		DCFP_R = p -> DCFP_R;
		TLength = PKT_LENGTH_RTS;
		ILength = 0;
		// NAV of the data packet always looks the same
		p->NAV = DCF_NAV_data;
		// This is the NAV setting for RTS
		NAV = Ether->RFC_xmt (DCF_XRate, p->TLength) + DCF_NAV_rts;
		DCFP_Flags = 0;
		DCFP_Type = DCFP_TYPE_RTS;
	};
};

packet CTSPacket : DCFPacket {

	void setup (Long snd, TIME nv) {

		TIME tn;

		// CTS does not have this attribute
		DCFP_S = NONE;
		DCFP_R = snd;
		TLength = PKT_LENGTH_CTS;
		ILength = 0;

		// Update NAV
		NAV = (nv != TIME_0) ? nv + DCF_NAV_cts : TIME_0;
		DCFP_Flags = 0;
		DCFP_Type = DCFP_TYPE_CTS;
	};
};

packet ACKPacket : DCFPacket {

	void setup (Long snd) {

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

    private:

	TIME	TAccs,		// Time when backoff expires
		TEnav,		// Time when NAV expires
		TIdle,		// Starting time of current IDLE period
		TBusy;		// Starting time of current BUSY period

	double	RSSI;		// Signal level of the packet being received

	Transceiver *Xcv;

	PQueue *PQ;		// Queue for outgoing packets
	Station *S;		// Station owning the module

	Long CWin;		// Current backoff window

	// Packet return function
	PRETFUN pretfun;
	
    public:

	// Maximum number of retransmissions. I don't think we want to change
	// it, but just in case I have turned it into a public attribute, so
	// that the "application" can change it if really needed.
	Long RMax;

	inline DCFPacket *getp (int st) {				// [XMT]
		if (PQ->empty ()) {
			PQ->wait (NONEMPTY, st);
			sleep;
		}
		return PQ->get ();
	};

	inline void bckf (int sts, int stb) {				// [XMT]

		TIME t;

		if (TIdle == TIME_inf) {
			// BUSY
			trc_tim ("BCKF BUSY");
			Monitor->wait (DCF_EVENT_SIL, stb);
			sleep;
		}
		if (Time < TAccs) {
			// Waiting for backoff
			trc_tim ("BCKF BACK");
			Timer->wait (TAccs - Time, sts);
			sleep;
		}
		if (Time < TEnav) {
			// Just in case: shouldn't normally happen as it
			// implies backoff, or doesn't it ... ?
			trc_tim ("BCKF NAV");
			Timer->wait (TEnav - Time, sts);
			sleep;
		}
		// Mark: no backoff in effect, i.e., we have served it
		TAccs = TIME_0;
		if ((t = TIdle + DCF_TDifs) > Time) {
			// Have to wait for Diffs
			trc_tim ("BCKF DIFS");
			Timer->wait (t - Time, sts);
			Monitor->wait (DCF_EVENT_ACT, stb);
			sleep;
		}
		trc_tim ("BCKF OK");
	};

	inline void rstb () {
		// Reset backoff parameters
		CWin = NONE;
		trc_tim ("RSTB");
	};

	inline void genb (int ord) {
		// Generates a backoff
		if (TAccs != TIME_0) {
			// Do nothing: honor the backoff currently pending
			trc_tim ("GENB IGN");
			return;
		}
		if (CWin == NONE) {
			CWin = DCF_CW_min;
		} else if (ord != BKF_NAV) {
			if ((CWin = CWin + CWin) > DCF_CW_max)
				CWin = DCF_CW_max;
		}
		TAccs = Time +
		etuToItu ((double)lRndUniform ((LONG)0, (LONG)CWin) * DCF_Slot);
		trc_tim ("GENB");
	};

	inline Boolean busy () { return Xcv->busy (); };

	inline void snav (TIME nav) {					// [RCV]
		TIME t;
		if (nav != TIME_0) {
			t = Time + nav;
			if (TEnav < t) 
				TEnav = t;
			// The way I understand it, we need a backoff after NAV
			genb (BKF_NAV);
		}
		trc_tim ("SNAV");
	};
		
	inline void xrts (DCFPacket *p, int st) {			// [XMT]
		RTSPacket *rts;
		rts = create RTSPacket (p);
		Xcv->transmit (rts, st);
		trc_tim ("XRTS");
		delete rts;
	};

	inline void xcts (Long sn, TIME nv, int st) {			// [RCV]
		CTSPacket *cts;
		cts = create CTSPacket (sn, nv);
		Xcv->transmit (cts, st);
		// This is not needed, but doesn't hurt
		TIdle = TIME_inf;
		// TBusy is set while we are doing this
		trc_tim ("XCTS");
		delete cts;
	};

	inline void xack (Long sn, int st) {				// [RCV]
		ACKPacket *ack;
		ack = create ACKPacket (sn);
		Xcv->transmit (ack, st);
		// This is not needed, but doesn't hurt
		TIdle = TIME_inf;
		// TBusy is set while we are doing this
		trc_tim ("XACK");
		delete ack;
	};

	inline void xstop () {						// [XCV]
		Xcv->stop ();
		trc_tim ("XSTOP");
	};

	inline void wcts (int sts, int stf) {				// [XMT]
		Monitor->wait (DCF_EVENT_CTS, sts);
		Timer->wait (DCF_TIMEOUT_cts, stf);
		trc_tim ("WCTS");
	};

	inline void sifs (int sts, int stf) {				// [XMT]
		Timer->wait (DCF_TSifs, sts);
		Xcv->wait (ACTIVITY, stf);
		trc_tim ("SIFS");
	};

	inline void xmit (DCFPacket *p, int st) {			// [XCV]
		// TIdle = TIME_inf;
		// The receiver should perceive this as an activity
		Xcv->transmit (p, st);
		trc_tim ("XMIT");
	};

	inline void wack (int sts, int stf) {				// [XMT]
		Monitor->wait (DCF_EVENT_ACK, sts);
		Timer->wait (DCF_TIMEOUT_ack, stf);
		trc_tim ("WACK");
	};

	inline void wact (int sta) {					// [RCV]
		Xcv->wait (ACTIVITY, sta);
		trc_tim ("WACT");
	};

	inline void sact () {						// [RCV]
		// This means "BUSY" (is also used as a flag)
		if (TIdle != TIME_inf) {
			// Note that we may be formally "BUSY" on NAV
			Monitor->signal (DCF_EVENT_ACT);
			TIdle = TIME_inf;
			TBusy = Time;
			trc_sig ("SACT");
			return;
		}
		// If TIdle == TIME_inf, we are in NAV. This new activity
		// does not change anything, and the TBusy setting at the
		// beginning of NAV is honored until the NAV expires.
		trc_tim ("SACT IG");
	};

	inline Boolean ssil (int sac, int sna) {			// [RCV]
		// Check if should reset backoff time
		if (TEnav > Time) {
			// NAV in effect: wake up also on NAV expiration
			Timer->wait (TEnav - Time, sna);
			// ... and do not mark the channel as IDLE until
			// the NAV expires
			trc_sig ("SSIL NA");
		} else {
			// Not needed, but makes things nicer in case of
			// race
			if (Xcv->busy ()) {
				// Means we are busy simultaneously with
				// NAV expiration
				trc_sig ("SSIL B");
				return YES;
			}
			// Now we are truly IDLE
			TIdle = Time;
			if (TAccs != TIME_0)
				TAccs += Time - TBusy;
			Monitor->signal (DCF_EVENT_SIL);
			trc_sig ("SSIL");
		}
		Xcv->wait (ACTIVITY, sac);
		return NO;
	};

	inline void wbot (int stb, int sts) {				// [RCV]
		Xcv->wait (BOT, stb);
		Xcv->wait (SILENCE, sts);
		trc_tim ("WBOT");
	};

	inline void strc (int sts) {					// [RCV]
		RSSI = Xcv->sigLevel (TheDCFP, SIGL_OWN);
		Xcv->follow (TheDCFP);
		Timer->wait (TIME_1, sts);
		trc_pkt (TheDCFP, "STRC");
	};

	inline void wrec (int str, int sta) {				// [RCV]
		Xcv->wait (EOT, str);
		Xcv->wait (BERROR, sta);
		Xcv->wait (SILENCE, sta);
		trc_tim ("WREC");
	};

	inline void scts () {						// [RCV]
		// Signal CTS reception. FIXME: should we do anything if
		// nobody's waiting (signal returns zero)?
#if TRACE_COLLISIONS
		if (Monitor->signal (DCF_EVENT_CTS) == 0)
			trc_unx ("SPURIOUS CTS SIGNAL");
#else
		Monitor->signal (DCF_EVENT_CTS);
#endif
		trc_tim ("SCTS");
	};

	inline void sack () {						// [RCV]
		// Signal ACK reception
#if TRACE_COLLISIONS
		if (Monitor->signal (DCF_EVENT_ACK) == 0)
			trc_unx ("SPURIOUS ACK SIGNAL");
#else
		Monitor->signal (DCF_EVENT_ACK);
#endif
		trc_tim ("SACK");
	};

	inline Boolean ismy (DCFPacket *p) {				// [RCV]
		return p->DCFP_R == S->getId ();
	};

	inline void recv () {						// [RCV]
		// Receive the current packet
		trc_pkt (TheDCFP, "RECV");
		trc_tim ("RECV");
		(*pretfun) (TheDCFP, PKT_RET_RECEIVED, RSSI);
	};

	inline void done (DCFPacket *p, Long ret) {
		// Called when the packet has been transmitted, returns
		// the number of retransmissions
		(*pretfun) (p, PKT_RET_SENT, ret);
	};

	inline Boolean send (DCFPacket *p) {
		p->NAV = TIME_0;
		trc_pkt (p, "SEND");
		trc_tim ("SEND");
		return PQ->queue (p);
	};

	inline void drop (DCFPacket *p) {
		(*pretfun) (p, PKT_RET_DROPPED);
	};

	inline Long room () { return PQ->free (); };

	RFModule (Transceiver*,
			Long,			// Outgoing packet queue size
			Long,			// Maximum retransmissions
			PRETFUN			// Packet return function
		 );
};

process Xmitter {

	RFModule *RFM;
	DCFPacket *CP;	// Packet currently being transmitted
	int Retrans;

	states { XM_LOOP, XM_TRY, XM_BACK, XM_RTS, XM_DATA, XM_XMT,
		    XM_NOCTS, XM_SBUSY, XM_WAK, XM_DONE, XM_EBCST, XM_COLL };

	perform;

	void setup (RFModule *rfm) { RFM = rfm; };
};

process Receiver {

	RFModule *RFM;
	Long CS;	// RTS sender
	TIME NV;	// DATA NAV from RTS

	states { RC_LOOP, RC_ACT, RC_SIL, RC_BOT, RC_RCV, RC_RCVD, RC_SACK,
		    RC_SACKD, RC_RABT, RC_SCTS, RC_SCTSD };

	perform;

	void setup (RFModule *rfm) { RFM = rfm; };
};

#endif
