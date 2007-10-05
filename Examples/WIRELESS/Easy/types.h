#ifndef __types_h__
#define	__types_h__

// Global parameters
extern double MinBackoff, MaxBackoff;
extern RATE XmitRate;
extern Long Preamble;
extern double XmitPower;

station WirelessNode {

	Packet Buffer;		// For Client packets to be transmitted
	Transceiver *Xcv;

	void setup (double x, double y) {
		Xcv = create Transceiver (XmitRate, Preamble, XmitPower,
			1.0, x, y);
		idToRFChannel (0) -> connect (Xcv);
	};
};

process Transmitter (WirelessNode) {

	Transceiver *Xcv;
	Packet *Buffer;

	TIME backoff () {
		return etuToItu (dRndUniform (MinBackoff, MaxBackoff));
	};

	void setup () {
		Xcv = S->Xcv;
		Buffer = &(S->Buffer);
	};

	states { NPacket, Ready, XDone, Backoff };
	perform;
};

process Receiver (WirelessNode) {

	Transceiver *Xcv;

	void setup () { Xcv = S->Xcv; };

	states { Wait, BPacket, Watch, Received };
	perform;
};

#endif
