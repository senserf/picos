/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "node.h"

#define	MIN_PACKET_LENGTH	24
#define	MAX_PACKET_LENGTH	46

#define	IBUFLEN		82

#define MAXPLEN		(MAX_PACKET_LENGTH + 2)

#if ENCRYPT
#include "encrypt.h"

static const lword secret [4] = { 0xbabadead,0x12345678,0x98765432,0x6754a6cd };

#endif

static word gen_packet_length (void) {

#if MIN_PACKET_LENGTH >= MAX_PACKET_LENGTH
	return MIN_PACKET_LENGTH;
#else
	return ((RND () % (MAX_PACKET_LENGTH - MIN_PACKET_LENGTH + 1)) +
			MIN_PACKET_LENGTH) & 0xFFE;
#endif

}

process	receiver (Node) {

	address packet;

	states { RC_TRY, RC_DATA };

	void setup (void *dummy) { };

	perform;
};

receiver::perform {

	lword sernum;

    state RC_TRY:

	if (S->rkillflag) {
		S->rkillflag = NO;
		S->RCVon = NO;
		terminate;
	}

	S->wait ((int)(&S->rkillflag), RC_TRY);
	packet = S->tcv_rnp (RC_TRY, S->sfd);
#if ENCRYPT
	decrypt (packet + 1, (S->tcv_left (packet) >> 1) - 2, secret);
#endif

	sernum = ntowl (((lword*) packet) [1]);

	if (S->last_rcv == MAX_ULONG) {
		// Initializing
		S->last_rcv = sernum;
		S->lost = 0;
	} else {
		S->last_rcv ++;
		if (sernum != S->last_rcv) {
			// A loss
			S->lost += sernum - S->last_rcv;
			S->last_rcv = sernum;
		}
	}

    transient RC_DATA:

#if 0
	S->ser_outf (RC_DATA, "RCV: [%d %d %d %d] <%lu, %lu> %u\r\n",
		packet [1],					// Sender Id
		S->tcv_left (packet) - 2,			// Length
		((byte*)packet) [S->tcv_left (packet) - 1],	// RSSI
		((byte*)packet) [S->tcv_left (packet) - 2],	// Quality
		S->last_rcv,					// Serial number
		S->lost,					// Lost
		S->memfree (0, NULL)
		);
#endif
	diag ("RCV: [%d %d %d %d] <%lu, %lu> %u",
		packet [1],					// Sender Id
		S->tcv_left (packet) - 2,			// Length
		((byte*)packet) [S->tcv_left (packet) - 1],	// RSSI
		((byte*)packet) [S->tcv_left (packet) - 2],	// Quality
		S->last_rcv,					// Serial number
		S->lost,					// Lost
		S->memfree (0, NULL)
		);

	S->tcv_endp (packet);

	proceed RC_TRY;
}

int Node::rcv_start () {

	rkillflag = 0;
	tcv_control (sfd, PHYSOPT_RXON, NULL);
	if (!RCVon) {
		fork (receiver, NULL);
		RCVon = YES;
		return 1;
	}
	return 0;
}

int Node::rcv_stop () {

	if (RCVon) {
		tcv_control (sfd, PHYSOPT_RXOFF, NULL);
		rkillflag = YES;
		trigger ((int)(&rkillflag));
		return 1;
	}

	return 0;
}

process sender (Node) {

	address packet;
	word	tdelay, packet_length;

	states { SN_SEND, SN_WBUF, SN_DONE };

	perform;

	void setup (word del) {
		tdelay = del;
	};
};

sender::perform {

    word pl;
    int  pp;

    state SN_SEND:

	if (S->tkillflag) {
Finish:
		S->tkillflag = 0;
		S->XMTon = NO;
		terminate;
	}

	packet_length = gen_packet_length ();

	if (packet_length < 10)
		packet_length = 10;
	else if (packet_length > MAX_PACKET_LENGTH)
		packet_length = MAX_PACKET_LENGTH;

    transient SN_WBUF:

	S->wait ((int)(&S->tkillflag), SN_SEND);
	packet = S->tcv_wnp (SN_WBUF, S->sfd, packet_length + 2);

	packet [1] = S->nodeid;

	((lword*) packet) [1] = wtonl (S->last_snt);

	// In words
	pl = packet_length / 2;

	for (pp = 4; pp < pl; pp++)
		packet [pp] = (word) (S->entropy);

	S->entropy += S->last_snt;
	S->last_snt++;

#if ENCRYPT
	encrypt (packet + 1, pl-1, secret);
#endif
	S->tcv_endp (packet);

    transient SN_DONE:

	S->ser_outf (SN_DONE, "SND: [%d %d] <%lu> %u\r\n",
		S->nodeid, packet_length, S->last_snt,
		S->memfree (0, NULL));

	S->delay (tdelay, SN_SEND);
	S->wait ((int)(&S->tkillflag), SN_SEND);
}

int Node::snd_start (int del) {

	tkillflag = NO;
	tcv_control (sfd, PHYSOPT_TXON, NULL);

	if (!XMTon) {
		fork (sender, del);
		// create sender (del);
		XMTon = 1;
		return 1;
	}

	return 0;
}

int Node::snd_stop () {

	tcv_control (sfd, PHYSOPT_TXOFF, NULL);
	if (XMTon) {
		tkillflag = YES;
		trigger ((int)(&tkillflag));
		return 1;
	}
	return 0;
}

process NRoot (Node) {

	char *ibuf;
	int k, n1;
	char *fmt, obuf [32];
	word p [2];

	states {
		RS_INIT,
		RS_RCMDM2,
		RS_RCMD,
		RS_ERROR,
		RS_SND,
		RS_SND1,
		RS_RCV,
		RS_QRCV,
		RS_QXMT,
		RS_QUIT,
		RS_QUIT1,
		RS_SSID,
		RS_SDID,
		RS_CCNT,
		RS_RES,
		RS_SMEM,
		RS_SMEM1
	};

	perform;
};

NRoot::perform {

    state RS_INIT:

	ibuf = (char*) umalloc (IBUFLEN);

	S->phys_dm2200 (0, MAXPLEN);
	// WARNING: the SMURPH model assumes that the plugin is static, i.e.,
	// all nodes use the same plugin. This is easy to change later, but ...
	// for now ...
	// Thus, only the first tcv_plug operation is effective, and the
	// subsequent ones (with the same ordinal) will be ignored. It is
	// checked, however, whether the plugin is the same in their all
	// instances.
	S->tcv_plug (0, &plug_null);
	S->sfd = S->tcv_open (WNONE, 0, 0);
	if (S->sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

    transient RS_RCMDM2:

	S->ser_out (RS_RCMDM2,
		"\r\nRF Ping Test\r\n"
		"Commands:\r\n"
		"i        -> set network Id\r\n"
		"d        -> set node Id\r\n"
		"s intvl  -> start/reset sending interval (2 secs default)\r\n"
		"r        -> start receiver\r\n"
		"o        -> stop receiver\r\n"
		"t        -> stop transmitter\r\n"
		"q        -> stop both\r\n"
		"c        -> clear received counter\r\n"
		"m        -> show free mem\r\n"
		"z        -> reset the node\r\n"
	);

    transient RS_RCMD:

	S->ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
		case 'i' :	proceed RS_SSID;
		case 'd' :	proceed RS_SDID;
		case 's' :	proceed RS_SND;
		case 'r' :	proceed RS_RCV;
		case 'o' :	proceed RS_QRCV;
		case 't' :	proceed RS_QXMT;
		case 'q' :	proceed RS_QUIT;
		case 'c' :	proceed RS_CCNT;
		case 'z' :	proceed RS_RES;
		case 'm' :	proceed RS_SMEM;
	}

    transient RS_ERROR:

	S->ser_out (RS_ERROR, "Illegal command or parameter\r\n");
	proceed RS_RCMDM2;

    state RS_SND:

	/* Default */
	n1 = 2048;
	S->scan (ibuf + 1, "%d", &n1);
	if (n1 < 16)
		n1 = 16;
	S->snd_start (n1);

    transient RS_SND1:

	S->ser_outf (RS_SND1, "Sender rate: %d\r\n", n1);
	proceed RS_RCMD;

    state RS_RCV:

	S->rcv_start ();
	proceed RS_RCMD;

    state RS_QRCV:

	S->rcv_stop ();
	proceed RS_RCMD;

    state RS_QXMT:

	S->snd_stop ();
	proceed RS_RCMD;

    state RS_QUIT:

	strcpy (obuf, "");
	if (S->rcv_stop ())
		strcat (obuf, "receiver");
	if (S->snd_stop ()) {
		if (strlen (obuf))
			strcat (obuf, " + ");
		strcat (obuf, "sender");
	}
	if (strlen (obuf))
		fmt = "Stopped: %s\r\n";
	else
		fmt = "No process stopped\r\n";

    transient RS_QUIT1:

	S->ser_outf (RS_QUIT+1, fmt, obuf);
	proceed RS_RCMD;

    state RS_SSID:

	n1 = 0;
	S->scan (ibuf + 1, "%d", &n1);
	S->tcv_control (S->sfd, PHYSOPT_SETSID, (address) &n1);
	proceed RS_RCMD;

    state RS_SDID:

	S->nodeid = 0;
	S->scan (ibuf + 1, "%d", & (S->nodeid));
	proceed RS_RCMD;

    state RS_CCNT:

	S->last_rcv = MAX_ULONG;
	proceed RS_RCMD;
	
    state RS_RES:

	S->reset ();
	// We should be killed past this

    state RS_SMEM:

	p [0] = S->memfree (0, p + 1);

    transient RS_SMEM1:

	S->ser_outf (RS_SMEM1, "Free mem = %d [%d]\r\n", p [0], p [1]);
	proceed RS_RCMD;
}

void Node::appStart () {

	create NRoot;
}
