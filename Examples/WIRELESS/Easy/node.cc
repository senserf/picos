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

Transmitter::perform {

	state NPacket:

		if (Client->getPacket (Buffer) == NO) {
			Client->wait (ARRIVAL, NPacket);
			sleep;
		}

	transient Ready:

		if (Xcv->busy ()) {
			Xcv->wait (SILENCE, Backoff);
			sleep;
		}

		Xcv->transmit (Buffer, XDone);
	
	state XDone:

		Xcv->stop ();
		Buffer->release ();
		Timer->wait (PSpace + backoff (), NPacket);

 	state Backoff:

		Timer->wait (backoff (), Ready);
}

Receiver::perform {

	state Wait:

		Xcv->wait (BOT, BPacket);

	state BPacket:

		Xcv->follow (ThePacket);
		skipto Watch;

	state Watch:

		Xcv->wait (EOT, Received);
		Xcv->wait (BERROR, Wait);

	state Received:

		if (ThePacket->isMy ())
			Client->receive (ThePacket, TheTransceiver);
		proceed Wait;
}
