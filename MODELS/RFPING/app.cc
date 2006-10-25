/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "node.h"

#define	ENCRYPT			0
#define	MIN_PACKET_LENGTH	24
#define	MAX_PACKET_LENGTH	46

#define	IBUFLEN		82

#define	PKT_ACK		0x1234
#define	PKT_DAT		0xABCD

#define	ACK_LENGTH	12
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

	states { RC_TRY, RC_DATA, RC_SACK, RC_ACK };

	void setup (void *dummy) { };

	perform;
};

receiver::perform {

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

	if (packet [1] == PKT_ACK) {
		S->last_ack = ntowl (((lword*)packet) [1]);
		proceed RC_ACK;
	}

	// Data packet
	S->last_rcv = ntowl (((lword*)packet) [1]);

    transient RC_DATA:

	S->ser_outf (RC_DATA, "RCV: [%x] %lu (len = %d), pow = %d qua = %d\r\n",
		packet [1],
		S->last_rcv, S->tcv_left (packet) - 2,
		((byte*)packet) [S->tcv_left (packet) - 1],
		((byte*)packet) [S->tcv_left (packet) - 2]
	);

	S->tcv_endp (packet);

	// Acknowledge it

    transient RC_SACK:

	if (S->XMTon) {
		packet = S->tcv_wnp (RC_SACK, S->sfd, ACK_LENGTH);
		packet [0] = 0;
		packet [1] = PKT_ACK;
		((lword*)packet) [1] = wtonl (S->last_rcv);

		packet [4] = (word) (S->entropy);
#if ENCRYPT
		encrypt (packet + 1, 4, secret);
#endif
		S->tcv_endp (packet);
	}
	proceed RC_TRY;

    state RC_ACK:

	S->ser_outf (RC_ACK, "ACK: %lu (len = %d)\r\n", S->last_ack,
		S->tcv_left (packet));

	S->tcv_endp (packet);
	trigger ((int)(&S->last_ack));
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

	states { SN_SEND, SN_NEXT, SN_NEXT1 };

	perform;

	void setup (word del) {
		packet_length = 12;
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
	S->wait ((int)(&S->tkillflag), SN_SEND);

	if (S->last_ack != S->last_snt) {
		S->delay (tdelay, SN_NEXT);
		S->wait ((int)(&S->last_ack), SN_SEND);
		sleep;
	}

	S->last_snt++;

	packet_length = gen_packet_length ();

	if (packet_length < 10)
		packet_length = 10;
	else if (packet_length > MAX_PACKET_LENGTH)
		packet_length = MAX_PACKET_LENGTH;

	proceed SN_NEXT;

    state SN_NEXT:

	if (S->tkillflag)
		goto Finish;

	S->wait ((int)(&S->tkillflag), SN_SEND);

	packet = S->tcv_wnp (SN_NEXT, S->sfd, packet_length + 2);

	packet [0] = 0;
	packet [1] = PKT_DAT;

	// In words
	pl = packet_length / 2;
	((lword*)packet)[1] = wtonl (S->last_snt);

	for (pp = 4; pp < pl; pp++)
		packet [pp] = (word) (S->entropy);

#if ENCRYPT
	encrypt (packet + 1, pl-1, secret);
#endif
	S->tcv_endp (packet);

    transient SN_NEXT1:

	S->ser_outf (SN_NEXT1, "SND %lu, len = %d\r\n", S->last_snt,
		packet_length);
	proceed SN_SEND;
}

int Node::snd_start (int del) {

	tkillflag = NO;
	last_ack = last_snt;
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
		RS_RCMDM1,
		RS_RCMD,
		RS_RCMD1,
		RS_SND,
		RS_SND1,
		RS_RCV,
		RS_QRCV,
		RS_QXMT,
		RS_QUIT,
		RS_QUIT1,
		RS_PAR,
		RS_PAR1,
		RS_SSID,
		RS_AUTOSTART,
		RS_RES
	};

	perform;
};

NRoot::perform {

    state RS_INIT:

	ibuf = (char*) umalloc (IBUFLEN);
	ibuf [0] = 0;

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
		"s intvl  -> start/reset sending interval (2 secs default)\r\n"
		"r        -> start receiver\r\n"
		"d i v    -> change phys parameter i to v\r\n"
		"g        -> get received power\r\n"
		"o        -> stop receiver\r\n"
		"t        -> stop transmitter\r\n"
		"q        -> stop both\r\n"
		"i        -> set station Id\r\n"
		"z        -> reset the node\r\n"
	);

    transient RS_RCMDM1:

	if ((unsigned char) ibuf [0] == 0xff)
		S->ser_out (RS_RCMDM1,
			"No command in 10 seconds -> start s 1024, r\r\n"
				);
    transient RS_RCMD:

diag ("COMMAND: %d %x", ibuf [0], ibuf [1]);

	if ((unsigned char) ibuf [0] == 0xff)
		S->delay (1024*10, RS_AUTOSTART);
  
	k = S->ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	if (ibuf [0] == 's')
		proceed RS_SND;
	if (ibuf [0] == 'r')
		proceed RS_RCV;
	if (ibuf [0] == 'd')
		proceed RS_PAR;
	if (ibuf [0] == 'q')
		proceed RS_QUIT;
	if (ibuf [0] == 'o')
		proceed RS_QRCV;
	if (ibuf [0] == 't')
		proceed RS_QXMT;
	if (ibuf [0] == 'i')
		proceed RS_SSID;
	if (ibuf [0] == 'z')
		proceed RS_RES;

    transient RS_RCMD1:

	S->ser_out (RS_RCMD1, "Illegal command or parameter\r\n");
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

    state RS_PAR:

	if (S->scan (ibuf + 1, "%u %u", p+0, p+1) < 2)
		proceed RS_RCMD1;

	if (p [0] > 5)
		proceed RS_RCMD1;

	S->tcv_control (S->sfd, PHYSOPT_SETPARAM, p);

    transient RS_PAR1:

	S->ser_outf (RS_PAR1, "Parameter %u set to %u\r\n", p [0], p [1]);
	proceed RS_RCMD;

    state RS_SSID:

	n1 = 0;
	S->scan (ibuf + 1, "%d", &n1);
	S->tcv_control (S->sfd, PHYSOPT_SETSID, (address) &n1);
	proceed RS_RCMD;

    state RS_AUTOSTART:
	  
	S->snd_start (1024);
  	S->rcv_start ();
	terminate;

    state RS_RES:

	S->reset ();
	// We should be killed past this
}

void Node::appStart () {

	create NRoot;
}
