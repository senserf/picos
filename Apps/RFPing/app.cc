/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"

//#define	PIN_RAW_TEST
//#define	PIN_OPS_TEST

#ifdef	PIN_OPS_TEST
#include "pinopts.h"
#endif

#ifdef	PIN_RAW_TEST
#include "portnames.h"
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

#if CC2420
#include "phys_cc2420.h"
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

fsm receiver {

  address packet;

  entry RC_TRY:

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

  entry RC_DATA:

#if UART_DRIVER

#if     CC1000 || DM2100 || CC1100 || DM2200 || CC2420

	ser_outf (RC_DATA, "RCV: [%x] %lu (len = %u), pow = %u qua = %x\r\n",
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

  entry RC_SACK:

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

  entry RC_ACK:

#if UART_DRIVER
	ser_outf (RC_ACK, "ACK: %lu (len = %u)\r\n", last_ack,
		tcv_left (packet));
#endif

	tcv_endp (packet);
	trigger (&last_ack);
	lcd_update ();
	proceed (RC_TRY);
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

  entry SN_SEND:

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

  entry SN_NEXT:

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

#if ENCRYPT
	encrypt (packet + 1, pl-1, secret);
#endif
	tcv_endp (packet);

  entry SN_NEXT_1:

#if UART_DRIVER
	// packet_length is useful info (payload)
	ser_outf (SN_NEXT_1, "SND: %lu, len = %u\r\n", last_snt, packet_length);
#endif
	lcd_update ();
	proceed (SN_SEND);
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


fsm root {

#if UART_DRIVER
	char *ibuf;
	int k, n1;
	char *fmt, obuf [32];
	word p [4];
	word n;
#if SDRAM_PRESENT
	word *mbuf, m, bp;
	lword nw, i, j;
#endif
#endif

#if CC1000 || CC1100 || CC2420
	const word parm_power = 255;
#endif

  entry RS_INIT:

#if LCD_DRIVER
	dsp_lcd ("PicOS ready     RF PING", YES);
#endif
  
#if UART_DRIVER
	ibuf = (char*) umalloc (IBUFLEN);
	ibuf [0] = 0xff;
#endif

#if 1
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

#if CC2420
	phys_cc2420 (0, MAXPLEN);
#endif

#if RF24G
	phys_rf24g (0, 5, 2);
#endif

#if RF24L01
	phys_rf24l01 (0, MAXPLEN);
#endif

	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

#if CC1000 || CC1100 || CC2420
	tcv_control (sfd, PHYSOPT_SETPOWER, (address) &parm_power);
#endif

  entry RS_RCMDm2:

#if UART_DRIVER
	ser_out (RS_RCMDm2,
		"\r\nRF Ping Test\r\n"
		"Commands:\r\n"
		"s intvl  -> start/reset sending interval (2 secs default)\r\n"
		"r        -> start receiver\r\n"
#if DM2100 == 0
		"p v      -> set transmit power\r\n"
#endif
		"g        -> get received power\r\n"
		"o        -> stop receiver\r\n"
		"t        -> stop transmitter\r\n"
		"q        -> stop both\r\n"
		"i        -> set station Id\r\n"
		"l n s    -> led n [012]\r\n"
#ifdef PIN_OPS_TEST
		"x p r    -> read ADC p ref r\r\n"
		"y p v    -> set p to v\r\n"
		"z p      -> show p\r\n"
#endif

#ifdef PIN_RAW_TEST
		"Y p v    -> set p [0,1,2 = in, 3 = special]\r\n"
		"Z p      -> show p\r\n"
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
#if GLACIER
		"F n      -> freeze for n secs\r\n"
#endif
		);
#endif

#if UART_DRIVER

  entry RS_RCMDm1:

	if ((unsigned char) ibuf [0] == 0xff)
		ser_out (RS_RCMDm1,
			"No command in 10 seconds -> start s 1024, r\r\n"
			);
#endif

  entry RS_RCMD:

#if UART_DRIVER
	if ((unsigned char) ibuf [0] == 0xff)
		delay (1024*10, RS_AUTOSTART);
  
	k = ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {

		case 's': proceed (RS_SND);
		case 'r': proceed (RS_RCV);
		case 'l': proceed (RS_LED);
#ifdef PIN_OPS_TEST
		case 'x': proceed (RS_GADC);
		case 'y': proceed (RS_SPIN);
		case 'z': proceed (RS_GPIN);
#endif

#ifdef PIN_RAW_TEST
		case 'Y': proceed (RS_RPIS);
		case 'Z': proceed (RS_RPIP);
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
#if GLACIER
		case 'F': proceed (RS_FRE);
#endif
	}
#else
	delay (1024, RS_AUTOSTART);
	release;
#endif

#if UART_DRIVER
/* ========= */

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

  entry RS_RCMD_1:

	ser_out (RS_RCMD_1, "Illegal command or parameter\r\n");
	proceed (RS_RCMDm2);

  entry RS_SND:

	/* Default */
	n1 = 2048;
	scan (ibuf + 1, "%u", &n1);
	if (n1 < 16)
		n1 = 16;
	snd_start (n1);

  entry RS_SND_1:

	ser_outf (RS_SND_1, "Sender rate: %u\r\n", n1);
	proceed (RS_RCMD);

  entry RS_RCV:

	rcv_start ();
	proceed (RS_RCMD);

#if DM2100 == 0
  entry RS_POW:

	/* Default */
	n1 = 0;
	scan (ibuf + 1, "%u", &n1);
	tcv_control (sfd, PHYSOPT_SETPOWER, (address)&n1);

  entry RS_POW_1:

	ser_outf (RS_POW_1,
		"Transmitter power set to %u\r\n", n1);

	proceed (RS_RCMD);
#endif /* ~DM 2100 */

  entry RS_RCP:

	n1 = tcv_control (sfd, PHYSOPT_GETPOWER, NULL);

  entry RS_RCP_1:

	ser_outf (RS_RCP_1,
		"Received power is %u\r\n", n1);

	proceed (RS_RCMD);

  entry RS_QRCV:

	rcv_stop ();
	proceed (RS_RCMD);

  entry RS_QXMT:

	snd_stop ();
	proceed (RS_RCMD);

  entry RS_QUIT:

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

  entry RS_QUIT_1:

	ser_outf (RS_QUIT_1, fmt, obuf);

	proceed (RS_RCMD);

  entry RS_SSID:

	n1 = 0;
	scan (ibuf + 1, "%u", &n1);
	tcv_control (sfd, PHYSOPT_SETSID, (address) &n1);
	proceed (RS_RCMD);

#if STACK_GUARD

  entry RS_STK:

	ser_outf (RS_STK, "Free stack space = %u words\r\n", stackfree ());
	proceed (RS_RCMD);
#endif

  entry RS_LED:

	p [0] = 0;
	p [1] = 0;
	scan (ibuf + 1, "%u %u", p+0, p+1);
	leds (p [0], p [1]);
	proceed (RS_RCMD);

#ifdef PIN_OPS_TEST

  entry RS_GADC:

	p [0] = 0;
	p [1] = 0;
	scan (ibuf + 1, "%u %u", p+0, p+1);

  entry RS_GADC_1:

	p [0] = pin_read_adc (RS_GADC_1, p [0], p [1], 4);

  entry RS_GADC_2:

	ser_outf (RS_GADC_2, "Value: %u\r\n", p [0]);
	proceed (RS_RCMD);

  entry RS_SPIN:

	p [1] = 0;
	p [1] = 0;
	scan (ibuf + 1, "%u %u", p+0, p+1);
	pin_write (p [0], p [1]);
	proceed (RS_RCMD);

  entry RS_GPIN:

	p [0] = 1;
	scan (ibuf + 1, "%u", p+0);
	p [0] = pin_read (p [0]);

  entry RS_GPIN_1:

	ser_outf (RS_GADC_1, "Value: %u\r\n", p [0]);
	proceed (RS_RCMD);

#endif

#ifdef PIN_RAW_TEST

  entry RS_RPIS:

	p [0] = WNONE;
	p [1] = 0;
	scan (ibuf + 1, "%u %u", p+0, p+1);

	if (p [1] < 2) {
		_PFS (p [0], 0);
		_PDS (p [0], 1);
		_PVS (p [0], p [1]);
	} else if (p [1] == 2) {
		// Set input
		_PFS (p [0], 0);
		_PDS (p [0], 0);
	} else {
		// Special function
		_PFS (p [0], 1);
	}
	proceed (RS_LPM_2);

  entry RS_RPIP:

	p [0] = WNONE;
	scan (ibuf + 1, "%u", p+0);

	p [1] = _PV (p [0]);
	p [2] = _PD (p [0]);
	p [3] = _PF (p [0]);

  entry RS_RPIP_1:

	ser_outf (RS_RPIP_1, "Pin %u = val %u, dir %u, fun %u\r\n",
		p [0], p [1], p [2], p [3]);
	proceed (RS_RCMD);

#endif	/* PIN_RAW_TEST */

#if UART_RATE_SETTABLE

  entry RS_URS:

	scan (ibuf + 1, "%u", &p [0]);
	ion (UART, CONTROL, (char*) p, UART_CNTRL_SETRATE);
	proceed (RS_RCMD);

  entry RS_URG:

	ion (UART, CONTROL, (char*) p, UART_CNTRL_GETRATE);

  entry RS_URG_1:
	
	ser_outf (RS_URG_1, "Rate: %u[00]\r\n", p [0]);
	proceed (RS_RCMD);
#endif

#if SDRAM_PRESENT

  entry RS_SDRAM:

	if (scan (ibuf + 1, "%u %u", &m, &n) != 2)
		proceed (RS_RCMD_1);

	if (m == 0 || n == 0)
		proceed (RS_RCMD_1);

	mbuf = umalloc (m + m);

  entry RS_SDRAM_1:

	ser_outf (RS_SDRAM_1, "Testing %u Kwords using a %u word buffer\r\n",
		n, m);

	delay (512, RS_SDRAM_2);
	release;

  entry RS_SDRAM_2:;

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

  entry RS_SDRAM_3:

	ser_out (RS_SDRAM_3, "Writing complete\r\n");

	delay (512, RS_SDRAM_4);
	release;

  entry RS_SDRAM_4:

	for (i = 0; i < nw; ) {
		bp = (nw - i > m) ? m : (word) (nw - i);
		ramget (mbuf, i, bp);
		j = i + bp;
		bp = 0;
		while (i < j) {
			if (mbuf [bp] != (word) (i + 1))
				proceed (RS_SDRAM_6);
			bp++;
			i++;
		}
	}

  entry RS_SDRAM_5:

	ser_outf (RS_SDRAM_5, "Test OK %x\r\n", mbuf [0]);
	ufree (mbuf);
	proceed (RS_RCMD);

  entry RS_SDRAM_6:

	ser_outf (RS_SDRAM_6, "Error: %x%x %x -> %x\r\n",
				(word)((i >> 16) & 0xffff),
				(word)( i        & 0xffff),
				mbuf [bp],
				(word) (i + 1));
	ufree (mbuf);
	proceed (RS_RCMD);

#endif	/* SDRAM_PRESENT */

#ifdef	BATTERY_TEST

  entry	RS_BTS:

	k = battery ();

  entry RS_BTS_1:

	ser_outf (RS_BTS_1, "Battery status: %u\r\n", k);
	proceed (RS_RCMD);
#endif

  entry RS_LPM:

	n = 0;
	scan (ibuf + 1, "%u", &n);

	if (n > 63)
		n = 63;

	powerdown ();

	if (n == 0)
		proceed (RS_LPM_2);

	delay (n * 1024, RS_LPM_1);
	release;

  entry RS_LPM_1:

	powerup ();

  entry RS_LPM_2:

	ser_out (RS_LPM_2, "Done\r\n");
	proceed (RS_RCMD);

#if GLACIER
  entry RS_FRE:

	n = 0;
	scan (ibuf + 1, "%u", &n);
	if (n == 0)
		n = 1;

	freeze (n);
	proceed (RS_LPM_2);
#endif

#endif	/* UART_DRIVER */

  entry RS_AUTOSTART:
	  
	snd_start (1024);
  	rcv_start ();

  	finish;
}
