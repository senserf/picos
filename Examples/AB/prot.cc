identify "Alternating Bit";

// Declare the extra attributes of packets; use different names, even
// though the types are internally identical, to emphasize that we are
// talking about different-purpose packets
packet PacketType { int SequenceBit; };

packet AckType { int SequenceBit; };

station SenderType {

	PacketType PacketBuffer;
	Port *IncomingPort, *OutgoingPort;
	Mailbox *AlertMailbox;
	int LastSent;
	void setup ();
};

station RecipientType {

	AckType AckBuffer;
	Port *IncomingPort, *OutgoingPort;
	Mailbox *AlertMailbox;
	int Expected;
	void setup ();
};

traffic TrafficType (Message, PacketType) { };

process TransmitterType (SenderType) {

	Port *Channel;
	PacketType *Buffer;
	Mailbox *Alert;
	TIME Timeout;
	states { NextPacket, Retransmit, EndXmit, Acked };
	// The setup argument is the ACK wait timeout
	void setup (TIME);
	perform;
};

process AckReceiverType (SenderType) {

	Port *Channel;
	Mailbox *Alert;
	states { WaitAck, AckBegin, AckArrival };
	void setup ();
	perform;
};

process ReceiverType (RecipientType) {

	Port *Channel;
	Mailbox *Alert;
	TIME Timeout;
	states { WaitPacket, BeginPacket, PacketArrival, TimeOut };
	// The setup argument is the (data) packet wait timeout
	void setup (TIME);
	perform;
};

process AcknowledgerType (RecipientType) {

	Port *Channel;
	AckType *Ack;
	Mailbox *Alert;
	states { WaitAlert, SendAck, EndXmit };
	void setup ();
	perform;
};

process Root {

	void readData (), buildNetwork (), defineTraffic (), startProtocol ();
	void printResults ();
	states { Start, Stop };
	perform;
};

// ============================================================================
// Model parameters readable from the data file
// ============================================================================

int    HeaderLength,		// Packet header length in bits
       AckLength,		// ACK length (fixed, header excluded)
       MinPktLength,		// Minimum packet length
       MaxPktLength;		// Maximum packet length

TIME   TransmissionRate,
       SenderTimeout,		// ACK wait timeout
       RecipientTimeout;	// Data packet wait timeout

double MessageLength,		// Traffic parameters
       MeanMessageInterarrivalTime,
       Distance,		// Channel length
       FaultRate;		// Link fault rate (BER)

long   MessageNumberLimit;	// Termination condition

// ============================================================================
// Pointers to network objects
// ============================================================================

SenderType       *Sender;	// The sender station
RecipientType    *Recipient;	// The receiver station

Link             *STRLink,	// S->R channel
                 *RTSLink;	// R->S channel

// ============================================================================

void SenderType::setup () {
//
// SMURPH "conctructor" for the Sender Station
//
	IncomingPort = create Port;
	// Only a port on which we transmit needs a rate
	OutgoingPort = create Port (TransmissionRate);
	// This is a signalling interface between the the processes run by
	// the station
	AlertMailbox = create Mailbox (1);
	// Packet counter (the alternating bit)
	LastSent = 0;
};

void RecipientType::setup () {
//
// SMURPH "conctructor" for the Recipient Station
//
	IncomingPort = create Port;
	OutgoingPort = create Port (TransmissionRate);
	AlertMailbox = create Mailbox (1);
	// Pre-fill the ACK packet (the part that never changes)
	AckBuffer.fill (this, Sender, AckLength + HeaderLength, AckLength);
	Expected = 0;
};

void TransmitterType::setup (TIME tmout) {
//
// Constructor for the transmitter process (run by Sender)
//
	// Local pointers to the relevant attributes of the station
	Channel = S->OutgoingPort;
	Buffer = &(S->PacketBuffer);
	Alert = S->AlertMailbox;
	Timeout = tmout;
};

void AckReceiverType::setup () {
//
// ACK receiver, the second process run by Sender

	Channel = S->IncomingPort;
	Alert = S->AlertMailbox;
};

void ReceiverType::setup (TIME tmout) {
//
// Run by Recipient
//
	Channel = S->IncomingPort;
	Alert = S->AlertMailbox;
	Timeout = tmout;
};

void AcknowledgerType::setup () {

	Channel = S->OutgoingPort;
	Ack = &(S->AckBuffer);
	Alert = S->AlertMailbox;
};

// ===========================================================================
// Process code implementations
// ===========================================================================

TransmitterType::perform {

	state NextPacket:

		if (!Client->getPacket (Buffer, MinPktLength, MaxPktLength,
		    HeaderLength)) {
			// No packet available, wait for arrival
			Client->wait (ARRIVAL, NextPacket);
			sleep;
		}

		// A new packet to transmit
		Buffer->SequenceBit = S->LastSent;

	transient Retransmit:

		Channel->transmit (Buffer, EndXmit);

	state EndXmit:

		Channel->stop ();

		// Wait for signal from the ACK-receiver ...
		Alert->wait (RECEIVE, Acked);
		// ... or timeout, whichever comes sooner
		Timer->wait (Timeout, Retransmit);

	state Acked:

		// Mark the buffer as empty (done with the packet)
		Buffer->release ();
		// Flip the alternating bit
		S->LastSent = 1 - S->LastSent;
		// Alert->erase ();
		// And continue for next outgoing packet
		proceed NextPacket;
};

AckReceiverType::perform {

	state WaitAck:

		// Wait for the beginning of a packet addressed to this
		// station
		Channel->wait (BMP, AckBegin);

	state AckBegin:

		// The ACK can complete normally, like this:
		Channel->wait (EMP, AckArrival);

		// Or may just disappear into silence, meaning that it has
		// an error:
		Channel->wait (SILENCE, WaitAck);

	state AckArrival:

		// We have received an ACK packet
		if (((AckType*)ThePacket)->SequenceBit == S->LastSent)
			// As expected, notify the Transmitter process
			Alert->put ();

		skipto WaitAck;
};

ReceiverType::perform {

	state WaitPacket:

		// Wait for a data packet from Sender ...
		Channel->wait (BMP, BeginPacket);
		// ... or for timeout
		Timer->wait (Timeout, TimeOut);

	state BeginPacket:

		// The packet can complete normally:
		Channel->wait (EMP, PacketArrival);

		// ... or may disappear into silence
		Channel->wait (SILENCE, WaitPacket);

	state PacketArrival:

		if (((PacketType*)ThePacket)->SequenceBit == S->Expected) {
			// The alternating bit is as expected, receive the
			// packet ...
			Client->receive (ThePacket, Channel);
			// ... and flip the "expected" bit
			S->Expected = 1 - S->Expected;
		}
		// Whatever has happened, kick the ACK-sender process to issue
		// an ACK

		Alert->put ();
		skipto WaitPacket;

	state TimeOut:

		Alert->put ();
		sameas WaitPacket;
};

AcknowledgerType::perform {

	state WaitAlert:

		// Wait for a kick from the data receiver process
		Alert->wait (RECEIVE, SendAck);

	state SendAck:

		// In response to the kick, send an ACK packet with the
		// alternating bit set to the inverse of "expected" (to
		// acknowledge the "last received" packet)
		Ack->SequenceBit = 1 - S->Expected;
		Channel->transmit (Ack, EndXmit);

	state EndXmit:

		Channel->stop ();
		proceed WaitAlert;

};

Root::perform {

	state Start:

		readData ();
		buildNetwork ();
		defineTraffic ();
		startProtocol ();
		Kernel->wait (DEATH, Stop);

	state Stop:

		printResults ();
};

void Root::readData () {

	readIn (HeaderLength);
	readIn (AckLength);
	readIn (MinPktLength);
	readIn (MaxPktLength);

	readIn (TransmissionRate);
	readIn (SenderTimeout);
	readIn (RecipientTimeout);
	readIn (Distance);

	readIn (MessageLength);
	readIn (MeanMessageInterarrivalTime);

	readIn (FaultRate);

	readIn (MessageNumberLimit);
};

void Root::buildNetwork () {

	Port *from, *to;
	
	Sender = create SenderType;
	Recipient = create RecipientType;
	STRLink = create Link (2);
	RTSLink = create Link (2);
	(from = Sender->OutgoingPort)->connect (STRLink);
	(to = Recipient->IncomingPort)->connect (STRLink);
	from->setDTo (to, Distance);
	RTSLink->setFaultRate (FaultRate);
	(from = Recipient->OutgoingPort)->connect (RTSLink);
	(to = Sender->IncomingPort)->connect (RTSLink);
	from->setDTo (to, Distance);
	STRLink->setFaultRate (FaultRate);
};

void Root::defineTraffic () {

	TrafficType *tp;

	tp = create TrafficType (MIT_exp + MLE_fix,
		MeanMessageInterarrivalTime, MessageLength);
	tp->addSender (Sender);
	tp->addReceiver (Recipient);
	setLimit (MessageNumberLimit);
};

void Root::startProtocol () {

	create (Sender) TransmitterType (SenderTimeout);
	create (Sender) AckReceiverType;
	create (Recipient) ReceiverType (RecipientTimeout);
	create (Recipient) AcknowledgerType;
};

void Root::printResults () {

	Client->printPfm ();
	STRLink->printPfm ();
	RTSLink->printPfm ();

};
