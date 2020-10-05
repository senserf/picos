/*
	Copyright 1995-2020 Pawel Gburzynski

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

#ifndef __types_h__
#define	__types_h__

// Global parameters
extern double MinBackoff, MaxBackoff, PSpace;
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
