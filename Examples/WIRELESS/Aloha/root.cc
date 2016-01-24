identify (Simple ALOHA);

#include "types.h"

Long NNodes;
double MinBackoff, MaxBackoff;
double MinAckWait, MaxAckWait;
Long MinPL, MaxPL, AckPL, FrameLength;
DataTraffic *UTrf;

process Root {

	void initNodes ();
	void initTraffic ();
	void setTimeLimit ();

	states { Start, Stop };

	perform;
};

// ============================================================================

void Root::initNodes () {

	sxml_t data, curr;
	const char *att;
	nparse_t np [12];
	unsigned short prd, pra, rad, raa, chd, cha, pwd, pwa;
	Long ncnt;
	double x, y;

	if ((data = sxml_child (xml_data, "nodes")) == NULL)
		excptn ("Root: no <nodes>");

	// Global parameters
	if ((curr = sxml_child (data, "parameters")) == NULL)
		excptn ("Root: no <parameters> in <nodes>");

	att = sxml_txt (curr);
	np [0 ].type = TYPE_int;
	np [1 ].type = TYPE_int;
	np [2 ].type = TYPE_int;
	np [3 ].type = TYPE_int;
	np [4 ].type = TYPE_int;
	np [5 ].type = TYPE_int;
	np [6 ].type = TYPE_int;
	np [7 ].type = TYPE_int;
	np [8 ].type = TYPE_double;
	np [9 ].type = TYPE_double;
	np [10].type = TYPE_double;
	np [11].type = TYPE_double;

	if (parseNumbers (att, 12, np) != 12)
		excptn ("Root: illegal <parameters>");

	prd = (unsigned short) (np [0] . IVal);
	pra = (unsigned short) (np [1] . IVal);
	rad = (unsigned short) (np [2] . IVal);
	raa = (unsigned short) (np [3] . IVal);
	chd = (unsigned short) (np [4] . IVal);
	cha = (unsigned short) (np [5] . IVal);
	pwd = (unsigned short) (np [6] . IVal);
	pwa = (unsigned short) (np [7] . IVal);
	MinBackoff = np [8 ] . DVal;
	MaxBackoff = np [9 ] . DVal;
	MinAckWait = np [10] . DVal;
	MaxAckWait = np [11] . DVal;

	print ("\nParameters shared by all nodes:\n\n");
	print (prd,		" Preamble length (d):", 10, 26);
	print (pra,		" Preamble length (a):", 10, 26);
	print (rad,		" Transmission rate (d):", 10, 26);
	print (raa,		" Transmission rate (a):", 10, 26);
	print (chd,		" Channel (d):", 10, 26);
	print (cha,		" Channel (a):", 10, 26);
	print (pwd,		" Transmit power (d):", 10, 26);
	print (pwa,		" Transmit power (a):", 10, 26);
	print (MinBackoff,	" Minimum backoff:", 10, 26);
	print (MaxBackoff,	" Maximum backoff:", 10, 26);
	print (MinAckWait,	" Minimum backoff:", 10, 26);
	print (MaxAckWait,	" Maximum backoff:", 10, 26);

	np [0] . type = TYPE_double;
	np [1] . type = TYPE_double;

	print ("\nNode locations\n\n");

	NNodes = NNodes / 2;

	for (ncnt = 0, curr = sxml_child (data, "node"); curr != NULL;
	    ncnt++, curr = sxml_next (curr)) {

		att = sxml_txt (curr);
		if (parseNumbers (att, 2, np) != 2)
			excptn ("Root: illegal location of node %1d", ncnt);

		create WirelessNode (prd, pra, rad, raa, chd, cha, pwd, pwa,
			x = np [0].DVal, y = np [1].DVal);

		create (ncnt) DReceiver;
		create (ncnt) AReceiver;
		create (ncnt) DTransmitter;
		print (form (" Node %4d at <%7.3f,%7.3f>\n", ncnt, x, y));
	}

	if (ncnt != NNodes)
		excptn ("Root: illegal number if <node> elements, %1d, "
			"expected %1d", ncnt, NNodes);

	print ("\n");
}
		
void Root::initTraffic () {

	sxml_t data, curr;
	double MinML, MaxML, MINT;
	nparse_t np [4];
	const char *att;

	if ((data = sxml_child (xml_data, "traffic")) == NULL)
		excptn ("Root: no <traffic>");

	if ((curr = sxml_child (data, "packets")) == NULL)
		excptn ("Root: no <packets> in <traffic>");

	att = sxml_txt (curr);

	np [0].type = TYPE_int;
	np [1].type = TYPE_int;
	np [2].type = TYPE_int;
	np [3].type = TYPE_int;

	if (parseNumbers (att, 4, np) != 4)
		excptn ("Root: illegal <packets> parameters");

	FrameLength = (Long) (np [0] . IVal);
	MinPL = (Long) (np [1] . IVal);
	MaxPL = (Long) (np [2] . IVal);
	AckPL = (Long) (np [3] . IVal);

	if ((curr = sxml_child (data, "messages")) == NULL)
		excptn ("Root: no <messages> in <traffic>");

	att = sxml_txt (curr);

	np [0].type = TYPE_double;
	np [1].type = TYPE_double;
	np [2].type = TYPE_double;

	if (parseNumbers (att, 3, np) != 3)
		excptn ("Root: illegal <packets> parameters");
	
	MinML = np [0] . DVal;
	MaxML = np [1] . DVal;
	MINT  = np [2] . DVal;

	print ("Packet parameters:\n\n");
	print (FrameLength,	"  Frame length:", 10, 26);
	print (MinPL,		"  Minimum packet length:", 10, 26);
	print (MinPL,		"  Maximum packet length:", 10, 26);
	print (AckPL,		"  Ack packet length:", 10, 26);
	print ("\n");

	print ("Traffic parameters:\n\n");
	print (MinML, "  Minimum message length:", 10, 26);
	print (MaxML, "  Maximum message length:", 10, 26);
	print (MINT,  "  Mean interarrival time:", 10, 26);
	print ("\n");

	UTrf =
	    create DataTraffic (MIT_exp + MLE_unf, MINT, MinML, MaxML);

	// All node are senders
	UTrf->addSender (ALL);
	// ... and receivers
	UTrf->addReceiver (ALL);
}

void Root::setTimeLimit () {

	sxml_t data;
	double tl;
	nparse_t np [1];
	const char *att;

	if ((data = sxml_child (xml_data, "timelimit")) == NULL)
		excptn ("Root: no <timelimit>");

	att = sxml_txt (data);

	np [0].type = TYPE_double;

	if (parseNumbers (att, 1, np) != 1)
		excptn ("Root: illegal <timelimit>");

	setLimit (0, etuToItu (tl = np [0] . DVal));

	print (tl, "  Time limit:", 10, 26);

	print ("\n");
}

Root::perform {

	state Start:

		sxml_t data;

		settraceFlags (
			TRACE_OPTION_TIME +
			TRACE_OPTION_ETIME +
			TRACE_OPTION_STATID +
			TRACE_OPTION_STATE
		);

		NNodes = initNetwork ();
		initNodes ();
		initTraffic ();
			
		// Reset after inits
		TheStation = System;

		setTimeLimit ();

		sxml_free (xml_data);

		Kernel->wait (DEATH, Stop);

	state Stop:

		Client->printPfm ();
		terminate;
}
