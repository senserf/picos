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
