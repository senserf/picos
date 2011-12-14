/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"
#include "pinopts.h"

#define	MIN_PACKET_LENGTH	24
#define	MAX_PACKET_LENGTH	42

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"
#include "phys_cc1100.h"
#include "plug_null.h"

#define	IBUFLEN		82

#define	PKT_ACK		0x1234
#define	PKT_DAT		0xABCD

#define	ACK_LENGTH	12
#define MAXPLEN		(MAX_PACKET_LENGTH + 2)

static int sfd;

static	int	rkillflag = 0;
static	lint	last_snt, last_rcv, last_ack;
static  char 	XMTon = 0, RCVon = 0;

static word gen_packet_length (void) {

#if MIN_PACKET_LENGTH >= MAX_PACKET_LENGTH
	return MIN_PACKET_LENGTH;
#else
	return ((rnd () % (MAX_PACKET_LENGTH - MIN_PACKET_LENGTH + 1)) +
			MIN_PACKET_LENGTH) & 0xFFE;
#endif

}

fsm receiver {

  address packet;

  state RC_TRY:

	if (rkillflag) {
		rkillflag = 0;
		finish;
	}
	wait (&rkillflag, RC_TRY);
	packet = tcv_rnp (RC_TRY, sfd);
	if (packet [1] == PKT_ACK) {
		last_ack = ntowl (((lword*)packet) [1]);
		proceed RC_ACK;
	}

	// Data packet
	last_rcv = ntowl (((lword*)packet) [1]);

  state RC_DATA:

	ser_outf (RC_DATA, "RCV: [%x] %lu (len = %d), pow = %d qua = %d\r\n",
		packet [1],
		last_rcv, tcv_left (packet) - 2,
#ifdef LITTLE_ENDIAN
		((byte*)packet) [tcv_left (packet) - 1],
		((byte*)packet) [tcv_left (packet) - 2]
#else
		((byte*)packet) [tcv_left (packet) - 2],
		((byte*)packet) [tcv_left (packet) - 1]
#endif
	);
	tcv_endp (packet);

	// Acknowledge it

  state RC_SACK:

	if (XMTon) {
		packet = tcv_wnp (RC_SACK, sfd, ACK_LENGTH);
		packet [0] = 0;
		packet [1] = PKT_ACK;
		((lword*)packet) [1] = wtonl (last_rcv);

		packet [4] = (word) entropy;
		tcv_endp (packet);
	}
	proceed RC_TRY;

  state RC_ACK:

	ser_outf (RC_ACK, "ACK: %lu (len = %d)\r\n", last_ack,
		tcv_left (packet));

	tcv_endp (packet);
	trigger (&last_ack);
	proceed RC_TRY;
}

int rcv_start (void) {

	rkillflag = 0;
	tcv_control (sfd, PHYSOPT_RXON, NULL);
	if (!running (receiver)) {
		runfsm receiver;
		RCVon = 1;
		return 1;
	}
	return 0;
}

int rcv_stop (void) {

	if (running (receiver)) {
		tcv_control (sfd, PHYSOPT_RXOFF, NULL);
		rkillflag = 1;
		trigger (&rkillflag);
		RCVon = 0;
		return 1;
	}
	return 0;
}

/* ============= */
/* Packet sender */
/* ============= */

static	int	tdelay, tkillflag = 0;

fsm sender {

  address packet;
  word packet_length = 12;

  state SN_SEND:

	if (tkillflag) {
		tkillflag = 0;
		finish;
	}
	wait (&tkillflag, SN_SEND);

	if (last_ack != last_snt) {
		delay (tdelay, SN_NEXT);
		wait (&last_ack, SN_SEND);
		release;
	}

	last_snt++;

	packet_length = gen_packet_length ();
	if (packet_length < 10)
		packet_length = 10;
	else if (packet_length > MAX_PACKET_LENGTH)
		packet_length = MAX_PACKET_LENGTH;

	proceed SN_NEXT;

  state SN_NEXT:

	word pl;
	int  pp;

	if (tkillflag) {
		tkillflag = 0;
		finish;
	}
	wait (&tkillflag, SN_SEND);

	packet = tcv_wnp (SN_NEXT, sfd, packet_length + 2);

	packet [0] = 0;
	packet [1] = PKT_DAT;

	// In words
	pl = packet_length / 2;
	((lword*)packet)[1] = wtonl (last_snt);

	for (pp = 4; pp < pl; pp++)
		packet [pp] = (word) entropy;

	tcv_endp (packet);

  state SN_NEXTP1:

	ser_outf (SN_NEXTP1, "SND %lu, len = %d\r\n", last_snt, packet_length);
	proceed SN_SEND;
}

int snd_start (int del) {

	tdelay = del;
	tkillflag = 0;
	last_ack = last_snt;

	tcv_control (sfd, PHYSOPT_TXON, NULL);
	if (!running (sender)) {
		runfsm sender;
		XMTon = 1;
		return 1;
	}

	return 0;
}

int snd_stop (void) {

	if (running (sender)) {
		tkillflag = 1;
		tcv_control (sfd, PHYSOPT_TXOFF, NULL);
		trigger (&tkillflag);
		XMTon = 0;
		return 1;
	}
	return 0;
}

/* ================= */
/* End packet sender */
/* ================= */

const static word parm_power = 255;

fsm root {

  char *ibuf;
  int k, n1;
  char *fmt, obuf [32];
  word p [2];

  state RS_INIT:

	ibuf = (char*) umalloc (IBUFLEN);
	ibuf [0] = 0xff;

	phys_cc1100 (0, MAXPLEN);

	tcv_plug (0, &plug_null);

	sfd = tcv_open (NONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

	tcv_control (sfd, PHYSOPT_SETPOWER, (address) &parm_power);

  state RS_RCMDM2:

	ser_out (RS_RCMDM2,
		"\r\nRF Ping Test\r\n"
		"Commands:\r\n"
		"s intvl  -> start/reset sending interval (2 secs default)\r\n"
		"r        -> start receiver\r\n"
		"p v      -> set transmit power\r\n"
		"g        -> get received power\r\n"
		"o        -> stop receiver\r\n"
		"t        -> stop transmitter\r\n"
		"q        -> stop both\r\n"
		"i        -> set station Id\r\n"
		"x p r    -> read ADC pin 'p' with reference r (0/1 1.5V/2.5V)\r\n"
		"y p v    -> set pin 'p' to v (0/1)\r\n"
		"z p      -> show the value of pin 'p'\r\n"
		"v        -> show unused stack space\r\n"
		"S r      -> set UART rate\r\n"
		"G        -> get UART rate\r\n"
	);

  state RS_RCMDM1:

	if ((unsigned char) ibuf [0] == 0xff)
		ser_out (RS_RCMDM1,
			"No command in 10 seconds -> start s 1024, r\r\n");

  state RS_RCMD:

	if ((unsigned char) ibuf [0] == 0xff)
		delay (1024*10, RS_AUTOSTART);
  
	k = ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	if (ibuf [0] == 's')
		proceed RS_SND;
	if (ibuf [0] == 'r')
		proceed RS_RCV;
	if (ibuf [0] == 'x')
		proceed RS_GADC;
	if (ibuf [0] == 'y')
		proceed RS_SPIN;
	if (ibuf [0] == 'z')
		proceed RS_GPIN;
	if (ibuf [0] == 'v')
		proceed RS_STK;
	if (ibuf [0] == 'S')
		proceed RS_URS;
	if (ibuf [0] == 'G')
		proceed RS_URG;
	if (ibuf [0] == 'p')
		proceed RS_POW;
	if (ibuf [0] == 'g')
		proceed RS_RCP;
	if (ibuf [0] == 'q')
		proceed RS_QUIT;
	if (ibuf [0] == 'o')
		proceed RS_QRCV;
	if (ibuf [0] == 't')
		proceed RS_QXMT;
	if (ibuf [0] == 'i')
		proceed RS_SSID;

  state RS_RCMDP1:

	ser_out (RS_RCMDP1, "Illegal command or parameter\r\n");
	proceed RS_RCMDM2;

  state RS_SND:

	/* Default */
	n1 = 2048;
	scan (ibuf + 1, "%d", &n1);
	if (n1 < 16)
		n1 = 16;
	snd_start (n1);

  state RS_SNDP1:

	ser_outf (RS_SNDP1, "Sender rate: %d\r\n", n1);
	proceed RS_RCMD;

  state RS_RCV:

	rcv_start ();
	proceed RS_RCMD;

  state RS_POW:

	/* Default */
	n1 = 0;
	scan (ibuf + 1, "%d", &n1);
	tcv_control (sfd, PHYSOPT_SETPOWER, (address)&n1);

  state RS_POWP1:

	ser_outf (RS_POWP1, "Transmitter power set to %d\r\n", n1);
	proceed RS_RCMD;

  state RS_RCP:

	n1 = tcv_control (sfd, PHYSOPT_GETPOWER, NULL);

  state RS_RCPP1:

	ser_outf (RS_RCPP1, "Received power is %d\r\n", n1);
	proceed RS_RCMD;

  state RS_QRCV:

	rcv_stop ();
	proceed RS_RCMD;

  state RS_QXMT:

	snd_stop ();
	proceed RS_RCMD;

  state RS_QUIT:

	strcpy (obuf, "");
	if (rcv_stop ())
		strcat (obuf, "receiver");
	if (snd_stop ()) {
		if (strlen (obuf))
			strcat (obuf, " + ");
		strcat (obuf, "sender");
	}
	if (strlen (obuf))
		fmt = "Stopped: %s\r\n";
	else
		fmt = "No process stopped\r\n";

  state RS_QUITP1:

	ser_outf (RS_QUITP1, fmt, obuf);

	proceed RS_RCMD;

  state RS_SSID:

	n1 = 0;
	scan (ibuf + 1, "%d", &n1);
	tcv_control (sfd, PHYSOPT_SETSID, (address) &n1);
	proceed RS_RCMD;

  state RS_STK:

	ser_outf (RS_STK, "Free stack space = %d words\r\n", stackfree ());
	proceed RS_RCMD;

  state RS_GADC:

	p [0] = 0;
	p [1] = 0;
	scan (ibuf + 1, "%u %u", p+0, p+1);

  state RS_GADCP1:

	p [0] = pin_read_adc (RS_GADCP1, p [0], p [1], 4);

  state RS_GADCP2:

	ser_outf (RS_GADCP2, "Value: %u\r\n", p [0]);
	proceed RS_RCMD;

  state RS_SPIN:

	p [1] = 0;
	p [1] = 0;
	scan (ibuf + 1, "%u %u", p+0, p+1);
	pin_write (p [0], p [1]);
	proceed RS_RCMD;

  state RS_GPIN:

	p [0] = 1;
	scan (ibuf + 1, "%u", p+0);
	p [0] = pin_read (p [0]);

  state RS_GPINP1:

	ser_outf (RS_GADCP1, "Value: %u\r\n", p [0]);
	proceed RS_RCMD;

  state RS_URS:

	scan (ibuf + 1, "%d", &p [0]);
	ion (UART, CONTROL, (char*) p, UART_CNTRL_SETRATE);
	proceed RS_RCMD;

  state RS_URG:

	ion (UART, CONTROL, (char*) p, UART_CNTRL_GETRATE);

  state RS_URGP1:
	
	ser_outf (RS_URGP1, "Rate: %u[00]\r\n", p [0]);
	proceed RS_RCMD;

  state RS_AUTOSTART:
	  
	snd_start (1024);
  	rcv_start ();

  	finish;
}
