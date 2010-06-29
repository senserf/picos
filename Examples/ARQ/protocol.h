#include "swindow.h"
#include "network.h"

station Sender {

	Frame	OF;		// Buffer for the current frame
	Port	*Out, *In;	// Channel interface

	BIG	TransmissionCounter,
		RetransmissionCounter,
		TransmittedBits,
		RetransmittedBits;

	// The constructor
	void setup (int, TIME);
};

process Transmitter (Sender) {

	int	NFrames,	// Number of frames currently in the window
		WS,		// Window size
		LastAck,	// Sequence number being currently acknowledged
		OldAck,		// Previously acknowledged sequence number
		Current,	// Current position within the window
		Outgoing;	// Next sequence number for outgoing frame

	TIME	Timeout;

	SlidingWindow	*SW;	// Sender's sliding window

	Port	*Out;		// Copies of the relevant station attributes
	Frame	*OF;

	int nextSN ();
	void setup (int, TIME);
	states {WakeUp, XDone};
	perform;
};

process AckMonitor (Sender) {

	Transmitter	*XM;	// Pointer to your Transmitter
	Port		*In;

	void setup (Transmitter*);
	states {WaitAck, NextAck};
	perform;
};

station Recipient {

	Port	*Out, *In;
	Ack	*ACK;
	
	void setup (int, TIME);
};

process Acknowledger (Recipient) {

	int	WS, LastReceived, Expected;
	TIME	Timeout;
	Port	*Out;
	Ack	*ACK;

	void nextSN ();
	void setup (int, TIME);
	states {WaitSignal, SendAck, XDone};
	perform;
};

process Receiver (Recipient) {

	Acknowledger	*AC;
	Port		*In;

	void setup (Acknowledger*);
	states {WaitFrame, NewFrame};
	perform;
};
