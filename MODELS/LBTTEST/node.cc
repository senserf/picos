#include "node.h"

void Node::setup (
		word mem,
		double	X,		// Coordinates
		double  Y,
		double	XP,		// Power
		double	RP,
		Long	BCmin,		// Backoff
		Long	BCmax,
		Long	LBTDel, 	// LBT delay (ms) and threshold (dBm)
		double	LBTThs,
		RATE	rate,		// Transmission rate
		Long	PRE,		// Preamble
		Long	UMODE,		// UART mode
		Long	UBS,		// UART buffer size
		Long	USP,		// UART rate
		char	*UIDV,		// Input device for UART
		char	*UODV		// Output device for UART
    ) {

	PicOSNode::setup (mem, X, Y, XP, RP, BCmin, BCmax, LBTDel, LBTThs,
		rate, PRE, UMODE, UBS, USP, UIDV, UODV);

	NNode::setup ();

	init ();
}

void Node::init () {

	tkillflag = rkillflag = NO;
	XMTon = RCVon = NO;
	last_snt = lost = 0;
	last_rcv = MAX_ULONG;
	nodeid = 0;
	// Start application root
	appStart ();
}

void Node::reset () {

	NNode::reset ();

	init ();
}
