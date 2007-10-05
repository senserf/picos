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
		Timer->wait (backoff (), NPacket);

 	state Backoff:

		Timer->wait (backoff (), Ready);
}

Receiver::perform {

	state Wait:

		Xcv->wait (BMP, BPacket);

	state BPacket:

		Xcv->follow (ThePacket);
		skipto Watch;

	state Watch:

		Xcv->wait (EOT, Received);
		Xcv->wait (BERROR, Wait);

	state Received:

		Client->receive (ThePacket, TheTransceiver);
		proceed Wait;
}
