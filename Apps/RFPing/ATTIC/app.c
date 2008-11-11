/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"

#ifdef	PIN_TEST
#include "pinopts.h"
#endif

#define	ENCRYPT	0
#define	MIN_PACKET_LENGTH	24

#if RF24L01
#define	MAX_PACKET_LENGTH	30
#else
#define	MAX_PACKET_LENGTH	42
#endif

/*
 * You should run this application on two boards at the same time. When
 * started (the 's' command), it will periodically send a packet over
 * the radio channel awaiting an acknowledgment from the other board
 * before proceeding with the next packet. Also, it will be responding
 * with acknowledgments to packets received from the other board.
 *
 * If no command is entered over the serial interface within 10 seconds,
 * the application starts with "s 1024\nr", i.e., it turns the receiver
 * on and begins to send packets at 1 sec intervals.
 *
 */

heapmem {10, 90};

#if UART_DRIVER
#include "ser.h"
#include "serf.h"
#include "form.h"
#endif

#if LCD_DRIVER
#include "lcd.h"
#include "form.h"
#endif

#if CC1000
#include "phys_cc1000.h"
#endif

#if CC1100
#include "phys_cc1100.h"
#endif

#if DM2100
#include "phys_dm2100.h"
#endif

#if DM2200
#include "phys_dm2200.h"
#endif

#if RF24G
#include "phys_rf24g.h"
#endif

#if RF24L01
#include "phys_rf24l01.h"
#endif

#if RADIO_DRIVER
#include "phys_radio.h"
#endif

#include "plug_null.h"

#if ENCRYPT
#include "encrypt.h"

static const lword secret [4] = { 0xbabadead,0x12345678,0x98765432,0x6754a6cd };

#endif

#define	IBUFLEN		82

#define	PKT_ACK		0x1234
#define	PKT_DAT		0xABCD

#define	ACK_LENGTH	12
#define MAXPLEN		(MAX_PACKET_LENGTH + 2)

static int sfd;

#define	RC_TRY		00
#define	RC_DATA		10
#define	RC_SACK		20
#define	RC_ACK		30

static	int	rkillflag = 0;
static	long	last_snt, last_rcv, last_ack;
static  char 	XMTon = 0, RCVon = 0;

static word gen_packet_length (void) {

#if MIN_PACKET_LENGTH >= MAX_PACKET_LENGTH
	return MIN_PACKET_LENGTH;
#else
	return ((rnd () % (MAX_PACKET_LENGTH - MIN_PACKET_LENGTH + 1)) +
			MIN_PACKET_LENGTH) & 0xFFE;
#endif

}

static void lcd_update (void) {

#if LCD_DRIVER
	char *fm;
	int k;

	fm = (char*) umalloc (34);
	form (fm, "%lu", last_snt);
	k = strlen (fm);
	while (k < 16)
		fm [k++] = ' ';
	form (fm + 16, "%lu", last_rcv);
	upd_lcd (fm);
	ufree (fm);
#endif

}

thread (receiver)

	static address packet;

  entry (RC_TRY)

	if (rkillflag) {
		rkillflag = 0;
		finish;
	}
	wait (&rkillflag, RC_TRY);
	packet = tcv_rnp (RC_TRY, sfd);
#if ENCRYPT
	decrypt (packet + 1, (tcv_left (packet) >> 1) - 2, secret);
#endif
	if (packet [1] == PKT_ACK) {
		last_ack = ntowl (((lword*)packet) [1]);
		proceed (RC_ACK);
	}

	// Data packet
	last_rcv = ntowl (((lword*)packet) [1]);

  entry (RC_DATA)

#if UART_DRIVER

#if     CC1000 || DM2100 || CC1100 || DM2200

	ser_outf (RC_DATA, "RCV: [%x] %lu (len = %u), pow = %u qua = %u\r\n",
		packet [1],
		last_rcv, tcv_left (packet) - 2,
#if LITTLE_ENDIAN
		((byte*)packet) [tcv_left (packet) - 1],
		((byte*)packet) [tcv_left (packet) - 2]
#else
		((byte*)packet) [tcv_left (packet) - 2],
		((byte*)packet) [tcv_left (packet) - 1]
#endif
	);
#else
	ser_outf (RC_DATA, "RCV: %lu (len = %u)\r\n", last_rcv,
		tcv_left (packet) - 2);
#endif
#endif
	tcv_endp (packet);

	// Acknowledge it

  entry (RC_SACK)

	if (XMTon) {
		packet = tcv_wnp (RC_SACK, sfd, ACK_LENGTH);
		packet [0] = 0;
		packet [1] = PKT_ACK;
		((lword*)packet) [1] = wtonl (last_rcv);

		packet [4] = (word) entropy;
#if ENCRYPT
		encrypt (packet + 1, 4, secret);
#endif
		tcv_endp (packet);
		lcd_update ();
	}
	proceed (RC_TRY);

  entry (RC_ACK)

#if UART_DRIVER
	ser_outf (RC_ACK, "ACK: %lu (len = %u)\r\n", last_ack,
		tcv_left (packet));
#endif

	tcv_endp (packet);
	trigger (&last_ack);
	lcd_update ();
	proceed (RC_TRY);

endthread

int rcv_start (void) {

	rkillflag = 0;
	tcv_control (sfd, PHYSOPT_RXON, NULL);
	if (!running (receiver)) {
		runthread (receiver);
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

#undef	RC_TRY
#undef	RC_DATA
#undef	RC_SACK
#undef	RC_ACK

/* ============= */
/* Packet sender */
/* ============= */

static	int	tdelay, tkillflag = 0;

#define	SN_SEND		00
#define	SN_NEXT		01

thread (sender)

	static address packet;
	static word packet_length = 12;

	word pl;
	int  pp;

  entry (SN_SEND)

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

	proceed (SN_NEXT);

  entry (SN_NEXT)

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

#if ENCRYPT
	encrypt (packet + 1, pl-1, secret);
#endif
	tcv_endp (packet);

  entry (SN_NEXT+1)

#if UART_DRIVER
	ser_outf (SN_NEXT+1, "SND %lu, len = %u\r\n", last_snt, packet_length);
#endif
	lcd_update ();
	proceed (SN_SEND);

endthread

int snd_start (int del) {

	tdelay = del;
	tkillflag = 0;
	last_ack = last_snt;

	tcv_control (sfd, PHYSOPT_TXON, NULL);
	if (!running (sender)) {
		runthread (sender);
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

#undef	SN_SEND
#undef	SN_NEXT

/* ================= */
/* End packet sender */
/* ================= */

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_SND		20
#define RS_RCV		30
#define	RS_PAR		40
#define	RS_POW		50
#define	RS_RCP		60
#define	RS_QRCV		63
#define	RS_QXMT		66
#define	RS_QUIT		70
#define	RS_SSID		75
#define	RS_CAL		80
#define	RS_STK		85
#define	RS_GADC		90
#define	RS_LED		95
#define	RS_SPIN		100
#define	RS_GPIN		110
#define	RS_URS		120
#define	RS_URG		130
#define	RS_SDRAM	140
#define	RS_BTS		150
#define	RS_LPM		160
#define	RS_FRE		165
#define	RS_AUTOSTART	200

#if CC1000 || CC1100
const static word parm_power = 255;
#endif

thread (root)

#if UART_DRIVER
	static char *ibuf;
	static int k, n1;
	static char *fmt, obuf [32];
	static word p [2];
	static word n;
#if SDRAM_PRESENT
	static word *mbuf, m, bp;
	static lword nw, i, j;
#endif
#endif

  entry (RS_INIT)

#if LCD_DRIVER
	dsp_lcd ("PicOS ready     RF PING", YES);
#endif
  
#if UART_DRIVER
	ibuf = (char*) umalloc (IBUFLEN);
	ibuf [0] = 0xff;
#endif

#if 0
	ibuf [0] = 0;
#endif

#if CC1000
	// Configure CC1000 for 19,200 bps
	phys_cc1000 (0, MAXPLEN, 96 /* 192 768 384 */);
#endif

#if DM2100
	phys_dm2100 (0, MAXPLEN);
#endif

#if DM2200
	phys_dm2200 (0, MAXPLEN);
#endif

#if CC1100
	phys_cc1100 (0, MAXPLEN);
#endif

#if RF24G
	phys_rf24g (0, 5, 2);
#endif

#if RF24L01
	phys_rf24l01 (0, MAXPLEN);
#endif

#if RADIO_DRIVER
	// Generic
	phys_radio (0, 0, MAXPLEN);
#endif

	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

#if CC1000 || CC1100
	tcv_control (sfd, PHYSOPT_SETPOWER, (address) &parm_power);
#endif

  entry (RS_RCMD-2)

#if UART_DRIVER
	ser_out (RS_RCMD-2,
		"\r\nRF Ping Test\r\n"
		"Commands:\r\n"
		"s intvl  -> start/reset sending interval (2 secs default)\r\n"
		"r        -> start receiver\r\n"
		"d i v    -> change phys parameter i to v\r\n"
#if CC1000 == 0 && DM2100 == 0
		// Not available
		"c btime  -> recalibrate the transceiver\r\n"
#endif

#if DM2100 == 0
		"p v      -> set transmit power\r\n"
#endif
		"g        -> get received power\r\n"
		"o        -> stop receiver\r\n"
		"t        -> stop transmitter\r\n"
		"q        -> stop both\r\n"
		"i        -> set station Id\r\n"
		"l n s    -> led n [012]\r\n"
#ifdef PIN_TEST
		"x p r    -> read ADC pin 'p' with reference r (0/1 1.5V/2.5V)\r\n"
		"y p v    -> set pin 'p' to v (0/1)\r\n"
		"z p      -> show the value of pin 'p'\r\n"
#endif

#if SDRAM_PRESENT
		"M b n    -> SDRAM test: n kwds, b bufsize\r\n"
#endif

#if STACK_GUARD
		"v        -> show unused stack space\r\n"
#endif

#if UART_RATE_SETTABLE
		"S r      -> set UART rate\r\n"
		"G        -> get UART rate\r\n"
#endif

#ifdef	BATTERY_TEST
		"B        -> battery test\r\n"
#endif
		"L n      -> enter PD mode for n secs\r\n"
		"F n      -> freeze for n secs\r\n"
		);
#endif

#if UART_DRIVER

  entry (RS_RCMD-1)

	if ((unsigned char) ibuf [0] == 0xff)
		ser_out (RS_RCMD-1,
			"No command in 10 seconds -> start s 1024, r\r\n"
			);
#endif

  entry (RS_RCMD)

#if UART_DRIVER
	if ((unsigned char) ibuf [0] == 0xff)
		delay (1024*10, RS_AUTOSTART);
  
	k = ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {

		case 's': proceed (RS_SND);
		case 'r': proceed (RS_RCV);
		case 'd': proceed (RS_PAR);
		case 'l': proceed (RS_LED);
#ifdef PIN_TEST
		case 'x': proceed (RS_GADC);
		case 'y': proceed (RS_SPIN);
		case 'z': proceed (RS_GPIN);
#endif

#if SDRAM_PRESENT
		case 'M': proceed (RS_SDRAM);
#endif

#if STACK_GUARD
		case 'v': proceed (RS_STK);
#endif

#if UART_RATE_SETTABLE
		case 'S': proceed (RS_URS);
		case 'G': proceed (RS_URG);
#endif

#ifdef BATTERY_TEST
		case 'B': proceed (RS_BTS);
#endif
		case 'L': proceed (RS_LPM);
		case 'F': proceed (RS_FRE);
	}
#else
	delay (1024, RS_AUTOSTART);
	release;
#endif

#if UART_DRIVER
/* ========= */

#if CC1000 == 0 && DM2100 == 0
	if (ibuf [0] == 'c')
		proceed (RS_CAL);
#endif

#if DM2100 == 0
	if (ibuf [0] == 'p')
		proceed (RS_POW);
#endif

	if (ibuf [0] == 'g')
		proceed (RS_RCP);
	if (ibuf [0] == 'q')
		proceed (RS_QUIT);
	if (ibuf [0] == 'o')
		proceed (RS_QRCV);
	if (ibuf [0] == 't')
		proceed (RS_QXMT);
	if (ibuf [0] == 'i')
		proceed (RS_SSID);

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "Illegal command or parameter\r\n");
	proceed (RS_RCMD-2);

  entry (RS_SND)

	/* Default */
	n1 = 2048;
	scan (ibuf + 1, "%u", &n1);
	if (n1 < 16)
		n1 = 16;
	snd_start (n1);

  entry (RS_SND+1)

	ser_outf (RS_SND+1, "Sender rate: %u\r\n", n1);
	proceed (RS_RCMD);

  entry (RS_RCV)

	rcv_start ();
	proceed (RS_RCMD);

#if DM2100 == 0
  entry (RS_POW)

	/* Default */
	n1 = 0;
	scan (ibuf + 1, "%u", &n1);

#if CC1000 == 0 && CC1100 == 0
	io (NONE, RADIO, CONTROL, (char*) &n1, RADIO_CNTRL_SETPOWER);
#else
	// There's no RADIO device, SETPOWER is available via
	// tcv_control
	tcv_control (sfd, PHYSOPT_SETPOWER, (address)&n1);
#endif

  entry (RS_POW+1)

	ser_outf (RS_POW+1,
		"Transmitter power set to %u\r\n", n1);

	proceed (RS_RCMD);
#endif /* ~DM 2100 */

  entry (RS_RCP)

#if CC1000 == 0 && DM2100 == 0 && CC1100 == 0
	n1 = io (NONE, RADIO, CONTROL, NULL, RADIO_CNTRL_READPOWER);
#else
	// No RADIO device for CC1000 & DM2100
	n1 = tcv_control (sfd, PHYSOPT_GETPOWER, NULL);
#endif

  entry (RS_RCP+1)

	ser_outf (RS_RCP+1,
		"Received power is %u\r\n", n1);

	proceed (RS_RCMD);

  entry (RS_QRCV)

	rcv_stop ();
	proceed (RS_RCMD);

  entry (RS_QXMT)

	snd_stop ();
	proceed (RS_RCMD);

  entry (RS_QUIT)

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

  entry (RS_QUIT+1)

	ser_outf (RS_QUIT+1, fmt, obuf);

	proceed (RS_RCMD);

  entry (RS_PAR)

	if (scan (ibuf + 1, "%u %u", p+0, p+1) < 2)
		proceed (RS_RCMD+1);

	if (p [0] > 5)
		proceed (RS_RCMD+1);

	tcv_control (sfd, PHYSOPT_SETPARAM, p);

  entry (RS_PAR+1)

	ser_outf (RS_PAR+1, "Parameter %u set to %u\r\n", p [0], p [1]);

	proceed (RS_RCMD);

  entry (RS_SSID)

	n1 = 0;
	scan (ibuf + 1, "%u", &n1);
	tcv_control (sfd, PHYSOPT_SETSID, (address) &n1);
	proceed (RS_RCMD);

#if CC1000 == 0 && DM2100 == 0 && CC1100 == 0

  entry (RS_CAL)

	/* The default */
	n1 = 19200;
	scan (ibuf + 1, "%u", &n1);
	io (NONE, RADIO, CONTROL, (char*) &n1, RADIO_CNTRL_CALIBRATE);

  entry (RS_CAL+1)

	ser_outf (RS_CAL+1,
		"Transceiver recalibrated to bit rate = %u bps\r\n", n1);

	proceed (RS_RCMD);
#endif

#if STACK_GUARD

  entry (RS_STK)

	ser_outf (RS_STK, "Free stack space = %u words\r\n", stackfree ());
	proceed (RS_RCMD);
#endif

  entry (RS_LED)

	p [0] = 0;
	p [1] = 0;
	scan (ibuf + 1, "%u %u", p+0, p+1);
	leds (p [0], p [1]);
	proceed (RS_RCMD);

#ifdef PIN_TEST

  entry (RS_GADC)

	p [0] = 0;
	p [1] = 0;
	scan (ibuf + 1, "%u %u", p+0, p+1);

  entry (RS_GADC+1)

	p [0] = pin_read_adc (RS_GADC+1, p [0], p [1], 4);

  entry (RS_GADC+2)

	ser_outf (RS_GADC+2, "Value: %u\r\n", p [0]);
	proceed (RS_RCMD);

  entry (RS_SPIN)

	p [1] = 0;
	p [1] = 0;
	scan (ibuf + 1, "%u %u", p+0, p+1);
	pin_write (p [0], p [1]);
	proceed (RS_RCMD);

  entry (RS_GPIN)

	p [0] = 1;
	scan (ibuf + 1, "%u", p+0);
	p [0] = pin_read (p [0]);

  entry (RS_GPIN+1)

	ser_outf (RS_GADC+1, "Value: %u\r\n", p [0]);
	proceed (RS_RCMD);

#endif

#if UART_RATE_SETTABLE

  entry (RS_URS)

	scan (ibuf + 1, "%u", &p [0]);
	ion (UART, CONTROL, (char*) p, UART_CNTRL_SETRATE);
	proceed (RS_RCMD);

  entry (RS_URG)

	ion (UART, CONTROL, (char*) p, UART_CNTRL_GETRATE);

  entry (RS_URG+1)
	
	ser_outf (RS_URG+1, "Rate: %u[00]\r\n", p [0]);
	proceed (RS_RCMD);
#endif

#if SDRAM_PRESENT

  entry (RS_SDRAM)

	if (scan (ibuf + 1, "%u %u", &m, &n) != 2)
		proceed (RS_RCMD+1);

	if (m == 0 || n == 0)
		proceed (RS_RCMD+1);

	mbuf = umalloc (m + m);

  entry (RS_SDRAM+1)

	ser_outf (RS_SDRAM+1, "Testing %u Kwords using a %u word buffer\r\n",
		n, m);

	delay (512, RS_SDRAM+2);
	release;

  entry (RS_SDRAM+2);

	nw = (lword)n * 1024;

	bp = 0;
	for (i = j = 0; i < nw; i++) {
		mbuf [bp++] = (word) (i + 1);
		if (bp == m) {
			/* Flush the buffer */
			ramput (j, mbuf, bp);
			j += bp;
			bp = 0;
		}
	}

	if (bp)
		/* The tail */
		ramput (j, mbuf, bp);

  entry (RS_SDRAM+3)

	ser_out (RS_SDRAM+3, "Writing complete\r\n");

	delay (512, RS_SDRAM+4);
	release;

  entry (RS_SDRAM+4)

	for (i = 0; i < nw; ) {
		bp = (nw - i > m) ? m : (word) (nw - i);
		ramget (mbuf, i, bp);
		j = i + bp;
		bp = 0;
		while (i < j) {
			if (mbuf [bp] != (word) (i + 1))
				proceed (RS_SDRAM+6);
			bp++;
			i++;
		}
	}

  entry (RS_SDRAM+5)

	ser_outf (RS_SDRAM+5, "Test OK %x\r\n", mbuf [0]);
	ufree (mbuf);
	proceed (RS_RCMD);

  entry (RS_SDRAM+6)

	ser_outf (RS_SDRAM+6, "Error: %x%x %x -> %x\r\n",
				(word)((i >> 16) & 0xffff),
				(word)( i        & 0xffff),
				mbuf [bp],
				(word) (i + 1));
	ufree (mbuf);
	proceed (RS_RCMD);

#endif	/* SDRAM_PRESENT */

#ifdef	BATTERY_TEST

  entry	(RS_BTS)

	k = battery ();

  entry (RS_BTS+1)

	ser_outf (RS_BTS+1, "Battery status: %u\r\n", k);
	proceed (RS_RCMD);
#endif

  entry (RS_LPM)

	n = 0;
	scan (ibuf + 1, "%u", &n);
	if (n == 0)
		n = 1;
	else if (n1 > 63)
		n = 63;

	powerdown ();
	delay (n * 1024, RS_LPM+1);
	release;

  entry (RS_LPM+1)

	powerup ();

  entry (RS_LPM+2)

	ser_out (RS_LPM+2, "Done\r\n");
	proceed (RS_RCMD);

  entry (RS_FRE)

	n = 0;
	scan (ibuf + 1, "%u", &n);
	if (n == 0)
		n = 1;

	freeze (n);
	proceed (RS_LPM+2);

#endif	/* UART_DRIVER */

  entry (RS_AUTOSTART)
	  
	snd_start (1024);
  	rcv_start ();

  	finish;

endthread
