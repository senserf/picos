#include "types.h"

DTransmitter::perform {

	state NPacket:

		if (UTrf->getPacket (Buffer, MinPL, MaxPL, FrameLength) == NO) {
			UTrf->wait (ARRIVAL, NPacket);
			sleep;
		}

		Buffer->SeqNum = ++(SndSeqNums [Buffer->Receiver]);

	transient Ready:

#if 0
		if (DCh->busy ()) {
			DCh->wait (SILENCE, Backoff);
			sleep;
		}
#endif

		DCh->transmit (Buffer, XDone);
	
	state XDone:

		DCh->stop ();

		S->ExpectedAckSender = (Long)(Buffer->Receiver);
		S->AckEvent->wait (0, GotAck);
		Timer->wait (S->backoff (), Ready);

	state GotAck:

		Buffer->release ();
		proceed NPacket;

 	state Backoff:

		Timer->wait (S->backoff (), Ready);
}

DReceiver::perform {

	state Wait:

		DCh->wait (BOT, BPacket);

	state BPacket:

		DCh->follow (ThePacket);
		skipto Watch;

	state Watch:

		DCh->wait (EOT, Received);
		DCh->wait (BERROR, Wait);

	state Received:

		if (ThePacket->isMy ()) {
			Long snd, seq;
			snd = ThePacket->Sender;
			seq = ((DataPacket*)ThePacket)->SeqNum;
			if (seq != RcvSeqNums [snd]) {
				Client->receive (ThePacket, TheTransceiver);
				RcvSeqNums [snd] = seq;
			}
			Ack->fill (S->getId (), ThePacket->Sender,
				AckPL + FrameLength, AckPL);
			Ack->Receiver = ThePacket->Sender;
			ACh->transmit (Ack, ADone);
			sleep;
		}
		proceed Wait;

	state ADone:

		ACh->stop ();
		proceed Wait;	
}

AReceiver::perform {

	state Wait:

		ACh->wait (BOT, BPacket);

	state BPacket:

		ACh->follow (ThePacket);
		skipto Watch;

	state Watch:

		ACh->wait (EOT, Received);
		ACh->wait (BERROR, Wait);

	state Received:

		if (ThePacket->isMy () && ThePacket->Sender ==
		    S->ExpectedAckSender)
			S->AckEvent->signal (0);

		proceed Wait;
}
