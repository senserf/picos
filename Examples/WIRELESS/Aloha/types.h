#ifndef __types_h__
#define	__types_h__

#include "network.h"

// ============================================================================

packet DataPacket { Long SeqNum; };
packet AckPacket { };
traffic DataTraffic (Message, DataPacket) { };

// ============================================================================

extern Long NNodes;
extern double MinBackoff, MaxBackoff;
extern double MinAckWait, MaxAckWait;

extern Long AckPL, MinPL, MaxPL, FrameLength;
extern DataTraffic *UTrf;

// ============================================================================

station WirelessNode {

	DataPacket Buffer;	// For Client packets to be transmitted
	AckPacket Ack;
	Transceiver *DCh, *ACh;
	Long *RcvSeqNums;
	Long *SndSeqNums;
	Long ExpectedAckSender;
	Mailbox *AckEvent;

	void setup (	unsigned short prd,
			unsigned short pra,
			unsigned short rad,
			unsigned short raa,
			unsigned short chd,
			unsigned short cha,
			unsigned short pwd,
			unsigned short pwa, 	double x, double y) {

		DCh = create Transceiver (RATE_1, (int)prd, 1.0, 1.0, x, y);
		ACh = create Transceiver (RATE_1, (int)pra, 1.0, 1.0, x, y);

		setrfrate (DCh, rad);
		setrfchan (DCh, chd);
		setrfpowr (DCh, pwd);

		setrfrate (ACh, raa);
		setrfchan (ACh, cha);
		setrfpowr (ACh, pwa);

		idToRFChannel (0) -> connect (DCh);
		idToRFChannel (0) -> connect (ACh);

		RcvSeqNums = new Long [NNodes];
		SndSeqNums = new Long [NNodes];
		memset (RcvSeqNums, 0, NNodes * sizeof (Long));
		memset (SndSeqNums, 0, NNodes * sizeof (Long));

		AckEvent = create Mailbox (-1);

		ExpectedAckSender = -1;
	};

	TIME backoff () {
		return etuToItu (dRndUniform (MinBackoff, MaxBackoff));
	};

	TIME ackwait () {
		return etuToItu (dRndUniform (MinAckWait, MaxAckWait));
	}
};

process DTransmitter (WirelessNode) {

	Transceiver *DCh;
	DataPacket *Buffer;
	Long *SndSeqNums;

	void setup () {
		DCh = S->DCh;
		Buffer = &(S->Buffer);
		SndSeqNums = S->SndSeqNums;
	};

	states { NPacket, Ready, XDone, GotAck, Backoff };
	perform;
};

process DReceiver (WirelessNode) {

	Transceiver *DCh, *ACh;
	AckPacket *Ack;
	Long *RcvSeqNums;

	void setup () {
		DCh = S->DCh;
		ACh = S->ACh;
		Ack = &(S->Ack);
		RcvSeqNums = S->RcvSeqNums;
	};

	states { Wait, BPacket, Watch, Received, ADone };
	perform;
};

process AReceiver (WirelessNode) {

	Transceiver *ACh;

	void setup () {
		ACh = S->ACh;
	};

	states { Wait, BPacket, Watch, Received };
	perform;
};

#endif
