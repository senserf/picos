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

extern Long AckPL, MinPL, MaxPL, FrameLength;
extern DataTraffic *UTrf;

// ============================================================================

// Two counters for evaluating packet delivery fraction
extern Long TotalSentPackets, TotalReceivedPackets;

// ============================================================================

station WirelessNode {

	DataPacket Buffer;	// For Client packets to be transmitted
	AckPacket Ack;
	Transceiver *DCh, *ACh;
	Long *RcvSeqNums;
	Long *SndSeqNums;
	Long ExpectedAckSender;
	Mailbox *AckEvent;

	void setup (
			// Default power level for sending data packets
			// (this is irrelevant, needed formally, so the
			// reset-to-default operation works)
			unsigned short prd,
			// Default power level for sending ACKs (irrelevant)
			unsigned short pra,
			// Data rate for data
			unsigned short rad,
			// Data rate for ACKs
			unsigned short raa,
			// Data channel
			unsigned short chd,
			// ACK channel
			unsigned short cha,
			// Actual power level for sending data packet
			unsigned short pwd,
			// Power level for sending ACKs
			unsigned short pwa, 	double x, double y) {

		DCh = create Transceiver (RATE_1, (int)prd, 1.0, 1.0, x, y);
		ACh = create Transceiver (RATE_1, (int)pra, 1.0, 1.0, x, y);

		// See network.cc for these operations
		setrfrate (DCh, rad);
		setrfchan (DCh, chd);
		setrfpowr (DCh, pwd);

		setrfrate (ACh, raa);
		setrfchan (ACh, cha);
		setrfpowr (ACh, pwa);

		idToRFChannel (0) -> connect (DCh);
		idToRFChannel (0) -> connect (ACh);

		// We use sequence numbers to match ACKs to data packets
		// for improved reliability; every node has a pair of entries
		// for every other node (a potential sender/receiver)
		RcvSeqNums = new Long [NNodes];
		SndSeqNums = new Long [NNodes];
		memset (RcvSeqNums, 0, NNodes * sizeof (Long));
		memset (SndSeqNums, 0, NNodes * sizeof (Long));

		// To signal ACKs received
		AckEvent = create Mailbox (-1);

		ExpectedAckSender = -1;
	};

	TIME backoff () {
		return etuToItu (dRndUniform (MinBackoff, MaxBackoff));
	};
};

process DTransmitter (WirelessNode) {
//
// Data transmitter (see node.cc for the code)
//
	Transceiver *DCh;
	DataPacket *Buffer;
	Long *SndSeqNums;

	void setup () {
		// Use private copies of station attributes for more
		// convenient access (so we don't have to write S->...)
		DCh = S->DCh;
		Buffer = &(S->Buffer);
		SndSeqNums = S->SndSeqNums;
	};

	states { NPacket, Ready, XDone, GotAck, Backoff };
	perform;
};

process DReceiver (WirelessNode) {
//
// Data receiver
//
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
//
// ACK receiver
//
	Transceiver *ACh;

	void setup () {
		ACh = S->ACh;
	};

	states { Wait, BPacket, Watch, Received };
	perform;
};

#endif
