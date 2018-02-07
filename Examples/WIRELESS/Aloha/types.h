#ifndef __types_h__
#define	__types_h__

#define	LOCAL_MEASURES

#define	DATA_RATE	9600
#define	PACKET_LENGTH	(80 * 8)
#define	ACK_LENGTH	0
#define	FRAME_LENGTH	(32 + 32)
#define	PREAMBLE_LENGTH	90
#define	MIN_BACKOFF	0.2
#define	MAX_BACKOFF	1.5
#define	SWITCH_DELAY	0.01
#define	ACK_DELAY	0.02

#include "rf.h"

// ============================================================================

packet DataPacket { unsigned int AB; };
packet AckPacket { unsigned int AB; };

// ============================================================================

typedef	struct {
	unsigned int AB;
	Long Terminal;
	TIME when;
} ackrq_t;

mailbox AckQueue (ackrq_t*) {

	void newAck (Long rcv, unsigned int ab) {
		ackrq_t *a = new ackrq_t;
		a->Terminal = rcv;
		a->AB = ab;
		a->when = Time + etuToItu (ACK_DELAY);
		put (a);
	};

	Boolean getAck (AckPacket *p, int st) {
		ackrq_t *a;
		TIME w;
		if (empty ()) {
			wait (NONEMPTY, st);
			return NO;
		}
		if ((w = first () -> when) > Time) {
			Timer->wait (w - Time, st);
			return NO;
		}
		a = get ();
		p->Receiver = a->Terminal;
		p->AB = a->AB;
		delete a;
		return YES;
	};
};

traffic ALOHATraffic (Message, DataPacket) { };

// ============================================================================

extern Long NTerminals;

extern ALOHATraffic *HTTrf, *THTrf;

extern ALOHARF *HTChannel, *THChannel;

#ifdef	LOCAL_MEASURES
extern	Long *HXmt, *TRcv;
#endif

// ============================================================================

#define	TheHub		((Hub*)idToStation(NTerminals))
#define	TheTerminal(i)	((Terminal*)idToStation(i))

station ALOHADevice abstract {

	DataPacket Buffer;
	Transceiver HTI, THI;

	void setup () {

		double x, y;

		readIn (x);
		readIn (y);
		HTChannel -> connect (&HTI);
		THChannel -> connect (&THI);
		setLocation (x, y);
#if 0
		Ouf << "Node: " << getId () << " at [" << x << ", " << y <<
			"]\n";
#endif
	};
};

station Hub : ALOHADevice {

	unsigned int *AB;
	AckQueue AQ;
	AckPacket ABuffer;

	void setup ();

	Packet *getPacket (int st)  {
		Packet *p;
		if (AQ.getAck (&ABuffer, st))
			return &ABuffer;
		if (Client->getPacket (Buffer, 0, PACKET_LENGTH,
		    FRAME_LENGTH)) {
			unwait ();
			return &Buffer;
		}
		Client->wait (ARRIVAL, st);
		sleep;
	};

	void receive (DataPacket *p) {

		Long sn;
		unsigned int ab;

		sn = p->Sender;
		ab = p->AB;

		// Always ACK
		AQ.newAck (sn, ab);

		if (ab == AB [sn]) {
			Client->receive (p, &THI);
			AB [sn] = 1 - AB [sn];
		}
	};
};

station Terminal : ALOHADevice {

	unsigned int AB;
	TIME When;

#ifdef	LOCAL_MEASURES
	RVariable *RC;
	int rc;
#endif
	void setup ();

	Boolean getPacket (int st) {
		if (When > Time) {
			Timer->wait (When - Time, st);
			return NO;
		}
		if (Buffer.isFull ())
			return YES;
		if (Client->getPacket (Buffer, 0, PACKET_LENGTH,
		    FRAME_LENGTH)) {
			Buffer.AB = AB;
#ifdef	LOCAL_MEASURES
			rc = 0;
#endif
			return YES;
		}
		Client->wait (ARRIVAL, st);
		return NO;
	};

	void backoff () {
		When = Time + etuToItu (dRndUniform (MIN_BACKOFF, MAX_BACKOFF));
	};

	void receive (Packet *p) {
		if (p->TP == NONE) {
			if (Buffer.isFull () &&
			    ((AckPacket*)ThePacket)->AB == Buffer.AB) {
#ifdef	LOCAL_MEASURES
				RC->update (rc);
#endif
				Buffer.release ();
				AB = 1 - AB;
				// One more implementation decision:
				// When = TIME_0;
			}
		} else {
#ifdef	LOCAL_MEASURES
			TRcv [getId ()] ++;
#endif
			Client->receive (p, &HTI);
		}
	};

#ifdef	LOCAL_MEASURES
	void printPfm () {
		Long id = getId ();
		LONG sc;
		double dist, min, max, ave [2];
		dist = THI.distTo (&(TheHub->THI));
		Ouf << getOName () << ", distance " <<
			form ("%3.0f:\n", dist / 1000.0);
		Ouf << "  From hub ---> " <<
			form (" Sent: %8d    Rcvd: %8d    PDF: %5.3f\n",
				HXmt [id], TRcv [id],
				HXmt [id] ? (double) TRcv [id]/HXmt [id] : 0.0);
		RC->calculate (min, max, ave, sc);
		Ouf << "  To hub <----- " <<
			form (" Sent: %8d    Retr: %8.6f\n\n",
				(Long) sc, ave [0]);
	};
#endif

};

process HTransmitter (Hub) {

	Packet *Pkt;
	Transceiver *Up;

	states { NPacket, PDone };

	void setup () {
		Up = &(S->HTI);
	};

	perform;
};

process HReceiver (Hub) {

	Transceiver *Down;

	states { Listen, GotBMP, GotEMP };

	void setup () {
		Down = &(S->THI);
	};

	perform;
};

process THalfDuplex (Terminal) {

	Transceiver *Up, *Down;

	states { Loop, GotBMP, GotEMP, Xmit, XDone };

	void setup () {
		Up = &(S->THI);
		Down = &(S->HTI);
	};

	perform;
};
	
#endif
