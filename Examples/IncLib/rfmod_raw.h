#ifndef __rfmod_raw_h__
#define	__rfmod_raw_h__

// This is a raw module resembling unstructured cheap transceivers, e.g.,
// TR1000, DM2200, CC1100, and so on. There is no MAC except for
// parameterizeable LBT (listen before transmit) + (parameterizeable)
// backoff. It assumed the shadowing channel model, which can be easily
// replaced.

#include "wchansh.h"

#ifndef	DEBUGGING
#define	DEBUGGING	0
#endif

#if DEBUGGING
// Tracing/debugging macros
#define	_trc(a, ...)	trace (a, ## __VA_ARGS__)
extern	char *_zz_dpk (Packet*);
#define	_dpk(p)		_zz_dpk (p)
#else
#define	_trc(a, ...)	do { } while (0)
#define	_dpk(p)		0
#endif

#define	RAW_EVENT_TXGO	MONITOR_LOCAL_EVENT(16)
#define	RAW_EVENT_RXGO	MONITOR_LOCAL_EVENT(17)

mailbox PQueue (Packet*) {

	// Queue of packets awaiting transmission

	inline Boolean queue (Packet *t) {
		if (free ()) {
			put (t);
			return YES;
		}
		return NO;
	};
};

class RFModule;

process Collector {

	RFModule *RFM;

	double		ATime,		// Accumulated sampling time
			Average,	// Average signal so far
			CLevel;		// Current (last) signal level
	TIME		Last;		// Last sample time

	double sigLevel () {

		double DT, NA, res;

		DT = (double)(Time - Last);
		NA = ATime + DT;
		res = ((Average * ATime) / NA) + (CLevel * DT) / NA;
		return res;
	};

	states { SSN_WAIT, SSN_RESUME, SSN_UPDATE, SSN_STOP };

	perform;

	void setup (RFModule*);
};

class	RFModule {

    // The is the model of the RF interface. It must be used as a prefix
    // class in an actual RFModule, because at least one of its virtual
    // methods must be redefined.

    friend class Xmitter;	// We run two processes with obvious purposes
    friend class Receiver;
    friend class Collector;

    private:

	TIME	TBackoff,	// Time when current backoff expires
		LBT_delay;	// LBT interval
	double	LBT_threshold;	// LBT signal level threshold
	double	RSSI;		// Signal level of the packet being received
	int 	CCounter;	// Counts LBT failures of the last packet

	PQueue *PQ;		// Queue for outgoing packets
	Station *S;		// Station owning the module

	Boolean	Receiving, Xmitting;

	Collector *SigSensor;

    public:

	Transceiver *Xcv;

	// --- Virtual methods that can be overriden in a subtype -------------

	virtual void UPPER_lbt (int ccnt) {
		// Called when activity is sensed before a transmission
	};

	// This one is called whenever a packet is received. The packet does not
	// have to be addressed to this node. The module receives all packets,
	// and it is up to the "upper layers" to figure out what to do next.
	virtual void UPPER_rcv (Packet *p, double rssi) = 0;

	// Called just before starting to transmit the packet
	virtual void UPPER_snd (Packet *p) {
		// About to transmit a packet (pointed to by p)
	};

	virtual void UPPER_snt (Packet *p) {
		// Immediately after completing a packet transmission
	};

	// --- end of virtual methods -----------------------------------------

	inline void backoff (double d) {
		// Sets the backoff to the indicated number of ETUs (seconds)
		TBackoff = Time + etuToItu (d);
		_trc ("Backoff time: %f msec", d * 1000.0);
	};

	inline Packet *get_packet (int st) {
		// Acquire next packet for transmission
		Packet *p;
		if (PQ->empty ()) {
			PQ->wait (NONEMPTY, st);
			// Note, we do return (no sleep) even if no packet is
			// available
			sleep;
		}
		p = PQ->get ();
		_trc ("Acquired packet: %s", _dpk (p));
		CCounter = 0;
		return p;
	};

	inline void wait_backoff (int st) {
		if (TBackoff > Time) {
			Timer->wait (Time - TBackoff, st);
			sleep;
		}
	};

	inline void wait_receiver (int st) {
		if (Receiving) {
			if (LBT_threshold > 0.0)
				// Treat this as LBT sense
				UPPER_lbt (CCounter++);
			// Wait until receiver done
			Monitor->wait (RAW_EVENT_TXGO, st);
			sleep;
		}
	};

	inline void wait_lbt (int st) {

		if (LBT_threshold > 0.0) {
			// LBT implemented: start the signal integrator
			SigSensor->signal ((void*)YES);
			Timer->wait (LBT_delay, st);
			sleep;
		}
	};

	inline void xmit_start (Packet *p, int st) {
		Xmitting = YES;
		UPPER_snd (p);
		Xcv->transmit (p, st);
		_trc ("Start xmit: %s", _dpk (p));
		sleep;
	};

	inline void xmit_stop (Packet *p) {
		Xcv->stop ();
		Xmitting = NO;
		UPPER_snt (p);
		_trc ("Stop xmit: %s", _dpk (p));
		Monitor->signal (RAW_EVENT_RXGO);
	};

	inline void lbt_ok (int st) {
		SigSensor->signal ((void*)NO);
		wait_receiver (st);
		if (SigSensor->sigLevel () >= LBT_threshold) {
			UPPER_lbt (CCounter++);
			// Proceed
			Timer->wait (0, st);
			sleep;
		}
	};

	inline double sig_monitor (int updt, int done) {
		Xcv->wait (ANYEVENT, updt);
		TheProcess->wait (SIGNAL, done);
		return Xcv->sigLevel ();
	};

	inline void init_rcv (int st) {
		Receiving = NO;
		Xcv->wait (BOT, st);
		sleep;
	};

	inline void start_rcv (int fai, int suc) {
		if (Xmitting) {
			// Ignore and wait until the transmitter is done
			Monitor->wait (RAW_EVENT_RXGO, fai);
			sleep;
		}
		// BOT heard: trace the packet to detect bit errors
		Receiving = YES;
		_trc ("Start rcv: %s", _dpk (ThePacket));
		RSSI = Xcv->sigLevel (ThePacket, SIGL_OWN);
		Xcv->follow (ThePacket);
		Timer->wait (TIME_1, suc);
		sleep;
	};

	inline void wait_rcv (int ok, int fai, int res) {
		Xcv->wait (EOT, ok);
		Xcv->wait (BERROR, fai);
		Xcv->wait (BOT, res);
		sleep;
	};

	inline void receive () {
		_trc ("Received: %s", _dpk (ThePacket));
		UPPER_rcv (ThePacket, RSSI);
	};

	inline Boolean send (Packet *p) {
		// Called by upper layers to deposit a packet for transmission
		_trc ("Send: %s", _dpk (p));
		return PQ->queue (p);
	};

	// Checks is the queue can acommodate a new packet
	inline Long room () { return PQ->free (); };

	RFModule (Transceiver*, Long, double, double);
};

process Xmitter {

	RFModule *RFM;
	Packet *CP;		// Packet currently being transmitted

	states { XM_LOOP, XM_TXDONE, XM_LBS };

	perform;

	void setup (RFModule *rfm) {
		CP = NULL;
		RFM = rfm;
		RFM->Xcv->setAevMode (NO);
		RFM->Xcv->setMinDistance (SEther->RDist);
	};
};

process Receiver {

	RFModule *RFM;

	states { RCV_GETIT, RCV_START, RCV_RECEIVE, RCV_GOTIT };

	perform;

	void setup (RFModule *rfm) { RFM = rfm; };
};

#endif
