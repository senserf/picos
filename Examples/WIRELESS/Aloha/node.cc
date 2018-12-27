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

#include "types.h"

// ============================================================================

HTransmitter::perform {

	state NPacket:

		Pkt = S->getPacket (NPacket);
		Up -> transmit (Pkt, PDone);

	state PDone:

		Up->stop ();
		if (Pkt == &(S->Buffer)) {
#ifdef	LOCAL_MEASURES
			HXmt [Pkt->Receiver]++;
#endif
			S->Buffer.release ();
		}

		skipto NPacket;
}

HReceiver::perform {

	state Listen:

		Down->wait (BMP, GotBMP);
		Down->wait (EMP, GotEMP);

	state GotEMP:

		S->receive ((DataPacket*)ThePacket);
		// Should add space before waiting for next BMP

	transient GotBMP:

		skipto Listen;
}

// ============================================================================

THalfDuplex::perform {

	state Loop:

		if (S->getPacket (Loop)) {
			Timer->delay (SWITCH_DELAY, Xmit);
			sleep;
		}

		Down->wait (BMP, GotBMP);
		Down->wait (EMP, GotEMP);

	state GotEMP:

		S->receive (ThePacket);

	transient GotBMP:

		skipto Loop;

	state Xmit:

#ifdef	LOCAL_MEASURES
		S->rc++;
#endif
		Up->transmit (S->Buffer, XDone);

	state XDone:

		Up->stop ();
		S->backoff ();
		Timer->delay (SWITCH_DELAY, Loop);
};
