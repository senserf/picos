/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "sysio.h"
#include "tcvphys.h"
#include "ser.h"
#include "serf.h"
#include "form.h"
#include "plug_null.h"

#ifdef	BUTTON_LIST
#include "buttons.h"
#endif

#include "pins.h"

#if CC1100
#include "phys_cc1100.h"
#endif

#if CC2420
#include "phys_cc2420.h"
#endif

#if CC1350_RF
#include "phys_cc1350.h"
#endif

#if defined(PIN_LIST) || defined (__SMURPH__)
#define	PIN_OPERATIONS_INCLUDED
#endif

#if defined(SENSOR_LIST) || defined (__SMURPH__)
#define	SENSOR_OPERATIONS_INCLUDED
#endif

#if defined(ACTUATOR_LIST) || defined (__SMURPH__)
#define	ACTUATOR_OPERATIONS_INCLUDED
#endif

#ifdef PIN_OPERATIONS_INCLUDED
#include "pinopts.h"
#endif

#ifdef SENSOR_OPERATIONS_INCLUDED
#include "sensors.h"
#endif

#ifdef ACTUATOR_OPERATIONS_INCLUDED
#include "actuators.h"
#endif

#define	MIN_PACKET_LENGTH	24
#define	MAX_PACKET_LENGTH	46

#define	IBUFLEN		82

#define	PKT_ACK		0x1234
#define	PKT_DAT		0xABCD
#define	PKT_BUN		0xEE00

#define	ACK_LENGTH	12
#define MAXPLEN		(MAX_PACKET_LENGTH + 2)

heapmem {10, 90};

// ============================================================================

int	sfd = -1;
lint	last_snt;
lint	last_rcv;
lint	last_ack;
bool	XMTon;
bool 	RCVon;
bool	rkillflag;
bool	tkillflag;
word	pmode = WNONE;

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
word	xpower;
#endif

// ============================================================================

static void setpm (word pm) {

	if (pm != pmode)
		setpowermode (pmode = pm);
}

static word gen_rnd (word min, word max) {
	return (rnd () % (max - min + 1)) + min;
}

static word gen_packet_length (void) {

#if MIN_PACKET_LENGTH >= MAX_PACKET_LENGTH
	return MIN_PACKET_LENGTH;
#else
	return gen_rnd (MIN_PACKET_LENGTH, MAX_PACKET_LENGTH) & 0xFFE;
#endif

}

static inline void rdelay (word del, word st) {
//
// Randomized delay
//
	if (del > 16)
		del = del - 7 + (rnd () & 0xF);

	delay (del, st);
}

fsm receiver {

    address packet;

    entry RC_TRY:

	if (rkillflag) {
		rkillflag = NO;
		RCVon = NO;
		finish;
	}

	when (&rkillflag, RC_TRY);
	packet = tcv_rnp (RC_TRY, sfd);

	if (packet [1] == PKT_ACK) {
		last_ack = ntowl (((lword*)packet) [1]);
		proceed RC_ACK;
	}

	// Data packet
	last_rcv = ntowl (((lword*)packet) [1]);

    entry RC_DATA:

	highlight_set (0, 0.5, "Rcvd: %1d %1d", last_rcv, tcv_left (packet) -2);
	ser_outf (RC_DATA, "RCV: [%x] %lu (len = %d), pow = %d qua = %d\r\n",
		packet [1],
		last_rcv, tcv_left (packet) - 2,
		((byte*)packet) [tcv_left (packet) - 1],
		((byte*)packet) [tcv_left (packet) - 2]
	);

	emul (1, "Reception: %lu", last_rcv);

	tcv_endp (packet);

	// Acknowledge it

    entry RC_SACK:

	if (XMTon) {
		packet = tcv_wnp (RC_SACK, sfd, ACK_LENGTH);
		packet [0] = 0;
		packet [1] = PKT_ACK;
		((lword*)packet) [1] = wtonl (last_rcv);
		packet [4] = (word) (entropy);
#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
		packet [ACK_LENGTH/2 - 1] = xpower;
#endif
		tcv_endp (packet);
	}
	proceed RC_TRY;

    entry RC_ACK:

	ser_outf (RC_ACK, "ACK: %lu (len = %d)\r\n", last_ack,
		tcv_left (packet));

	tcv_endp (packet);
	trigger (&last_ack);
	proceed RC_TRY;
}

int rcv_start () {

	rkillflag = 0;
	tcv_control (sfd, PHYSOPT_RXON, NULL);
	if (!RCVon) {
		runfsm receiver;
		RCVon = YES;
		return 1;
	}
	return 0;
}

int rcv_stop () {

	if (RCVon) {
		tcv_control (sfd, PHYSOPT_RXOFF, NULL);
		rkillflag = YES;
		trigger (&rkillflag);
		return 1;
	}

	return 0;
}

fsm sender (int tdelay) {

    address packet;
    word packet_length;

    entry SN_SEND:

	if (tkillflag) {
Finish:
		tkillflag = 0;
		XMTon = NO;
		finish;
	}
	when (&tkillflag, SN_SEND);

	if (last_ack != last_snt) {
		rdelay (tdelay, SN_NEXT);
		when (&last_ack, SN_SEND);
		release;
	}

	last_snt++;

	packet_length = gen_packet_length ();

	// proceed SN_NEXT;
	rdelay (tdelay, SN_NEXT);
	release;

    entry SN_NEXT:

    	word pl;
    	int  pp;

	if (tkillflag)
		goto Finish;

	when (&tkillflag, SN_SEND);

	packet = tcv_wnp (SN_NEXT, sfd, packet_length + 2);

	packet [0] = 0;
	packet [1] = PKT_DAT;

	// In words
	pl = packet_length / 2;
	((lword*)packet)[1] = wtonl (last_snt);

	for (pp = 4; pp < pl; pp++)
		packet [pp] = (word) (entropy);

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
	packet [pl] = xpower;
#endif
	tcv_endp (packet);

    entry SN_NEXT1:

	highlight_set (1, 0.5, "Sent: %1d %1d", last_snt, packet_length);
	ser_outf (SN_NEXT1, "SND %lu, len = %d\r\n", last_snt,
		packet_length);
	emul (3, "SNT: %1d %1d", last_snt, packet_length);
	proceed SN_SEND;
}

int snd_start (int del) {

	tkillflag = NO;
	last_ack = last_snt;

	if (!XMTon) {
		runfsm sender (del);
		XMTon = 1;
		return 1;
	}

	return 0;
}

int snd_stop () {

	if (XMTon) {
		tkillflag = YES;
		trigger (&tkillflag);
		return 1;
	}
	return 0;
}

#ifdef	PMON_NOTEVENT

fsm pin_monitor {

    lint CNT, CMP;
    word STA;
    const char *MSG;

    entry PM_START:

	if (pmon_pending_not ()) {
		MSG = "NOTIFIER PENDING\r\n";
		proceed PM_OUT;
	}
	if (pmon_pending_cmp ()) {
		MSG = "COUNTER PENDING\r\n";
		proceed PM_OUT;
	}
	when (PMON_NOTEVENT, PM_NOTIFIER);
	when (PMON_CNTEVENT, PM_COUNTER);
	release;

  entry PM_OUT:

	ser_out (PM_OUT, MSG);
	proceed PM_START;

  entry PM_NOTIFIER:

	MSG = "NOTIFIER EVENT\r\n";
	proceed PM_EVENT;

  entry PM_COUNTER:

	MSG = "COUNTER EVENT\r\n";

  entry PM_EVENT:

	ser_out (PM_EVENT, MSG);

	STA = pmon_get_state ();
	CNT = pmon_get_cnt ();
	CMP = pmon_get_cmp ();

  entry PM_EVENT1:

	ser_outf (PM_EVENT1, "STATE: %x, CNT: %lu, CMP: %lu\r\n",
		STA, CNT, CMP);
	pmon_pending_cmp ();
	pmon_pending_not ();

	proceed PM_START;
}

#endif	/* PMON_NOTEVENT */

fsm watchdog {

    entry WA_START:

	watchdog_start ();

    entry WA_WAIT:

	watchdog_clear ();
	delay (300, WA_WAIT);
}

fsm reverter (word n) {
//
// Revert to power mode 0 after n seconds
//

	entry RE_START:

		delay (((n & 0xff) > 60 ? 60 : (n & 0xff)) * 1024, RE_REVERT);
		release;

	entry RE_REVERT:

		setpm (n >> 8);

	entry RE_MESSAGE:

		ser_out (RE_MESSAGE, "REVERTED\r\n");
		finish;
}

#ifdef	BUTTON_LIST

fsm showbuttons (word b) {

	state SHOWTHEM:

		ser_outf (SHOWTHEM, "B: %x\r\n", b);
		finish;
}

void button (word butts) {

	if (pmode > 1)
		setpm (1);

	runfsm showbuttons (butts);
}

#endif

fsm root {

    char *ibuf;
    sint k, n1;
    const char *fmt;
    char obuf [32];
    word p [3];
    lword lp [3];

    entry RS_INIT:

	setpm (0);
	ibuf = (char*) umalloc (IBUFLEN);
	ibuf [0] = 0;

	// runfsm watchdog;

#if CC1100
	phys_cc1100 (0, MAXPLEN);
#endif
#if CC2420
	phys_cc2420 (0, MAXPLEN);
#endif
#if CC1350_RF
	phys_cc1350 (0, MAXPLEN);
#endif
	// WARNING: the SMURPH model assumes that the plugin is static, i.e.,
	// all nodes use the same plugin. This is easy to change later, but
	// ... for now ...
	// Thus, only the first tcv_plug operation is effective, and the
	// subsequent ones (with the same ordinal) will be ignored. It is
	// checked, however, whether the plugin is the same in all their
	// instances.
	tcv_plug (0, &plug_null);
	sfd = tcv_open (WNONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

#ifdef	BUTTON_LIST
	buttons_action (button);
#endif

    entry RS_RCMDM2:

	ser_out (RS_RCMDM2,
		"\r\nRF Ping Test\r\n"
		"Commands:\r\n"
		"s intvl  -> snd int\r\n"
		"b n      -> bunch pkts\r\n"
		"r        -> rcv\r\n"
		"d q v    -> physopt\r\n"
		"x p      -> xmt pwr\r\n"
		"y        -> get xmt pwr\r\n"
		"o        -> stop rcv\r\n"
		"t        -> stop xmt\r\n"
		"q        -> stop rf\r\n"
		"i        -> set sid\r\n"
		"z        -> reset\r\n"
		"m n d r  -> power mode\r\n"
		"c n d d  -> seconds\r\n"
#ifdef PIN_OPERATIONS_INCLUDED
		"p n      -> read pin\r\n"
		"u n v    -> set pin\r\n"
		"a n r d  -> read ADC pin\r\n"
		"w n v r  -> write DAC pin\r\n"
#ifdef PMON_NOTEVENT
		"C c e    -> start cnt\r\n"
		"P c      -> set cmp\r\n"
		"G        -> get cnt\r\n"
		"D        -> stop cnt\r\n"
		"N e      -> start ntf\r\n"
		"M        -> stop ntf\r\n"
		"X        -> start mtr\r\n"
		"Y        -> stop mtr\r\n"
#endif
#endif
#ifdef SENSOR_OPERATIONS_INCLUDED
		"S n      -> read n-th sensor\r\n"
#endif
#ifdef ACTUATOR_OPERATIONS_INCLUDED
		"A n v    -> set n-th actuator\r\n"
#endif
		"K        -> force watchdog reset\r\n"
	);

    entry RS_RCMDM1:

	if ((unsigned char) ibuf [0] == 0xff)
		ser_out (RS_RCMDM1,
			"No command in 10 seconds -> start s 1024, r\r\n"
				);
    entry RS_RCMD:

	if ((unsigned char) ibuf [0] == 0xff)
		delay (1024*10, RS_AUTOSTART);

	k = ser_in (RS_RCMD, ibuf, IBUFLEN);

	switch (ibuf [0]) {
	    case 's': proceed RS_SND;
	    case 'b': proceed RS_BUNCH;
	    case 'r': proceed RS_RCV;
	    case 'd': proceed RS_PAR;
	    case 'x': proceed RS_SETP;
	    case 'y': proceed RS_GETP;
	    case 'o': proceed RS_QRCV;
	    case 't': proceed RS_QXMT;
	    case 'q': proceed RS_QUIT;
	    case 'i': proceed RS_SSID;
	    case 'z': proceed RS_RES;
	    case 'm': proceed RS_PDM;
	    case 'c': proceed RS_CLK;
#ifdef __CC1350__
	    case 'e': proceed RS_GEC;
#endif
#ifdef PIN_OPERATIONS_INCLUDED
	    case 'p': proceed RS_RPIN;
	    case 'u': proceed RS_SPIN;
	    case 'a': proceed RS_RANA;
	    case 'w': proceed RS_WANA;
#ifdef PMON_NOTEVENT
	    case 'C': proceed RS_PSCN;
	    case 'P': proceed RS_PSCM;
	    case 'G': proceed RS_PGCN;
	    case 'D': proceed RS_PQCN;
	    case 'N': proceed RS_PSNT;
	    case 'M': proceed RS_PQNT;
	    case 'X': proceed RS_PSMT;
	    case 'Y': proceed RS_PQMT;
#endif
#endif
#ifdef SENSOR_OPERATIONS_INCLUDED
	    case 'S': proceed RS_GETS;
#endif
#ifdef ACTUATOR_OPERATIONS_INCLUDED
	    case 'A': proceed RS_SETA;
#endif
	    case 'K': {
			killall (watchdog);
			proceed RS_RCMD;
		      }
	}

    entry RS_RCMD1:

	ser_out (RS_RCMD1, "Illegal command or parameter\r\n");
	proceed RS_RCMDM2;

    entry RS_SND:

	/* Default */
	n1 = 2048;
	scan (ibuf + 1, "%d", &n1);
	if (n1 < 16)
		n1 = 16;
	snd_start (n1);

    entry RS_SND1:

	ser_outf (RS_SND1, "Sender rate: %d\r\n", n1);
	proceed RS_RCMD;

    entry RS_CLK:

	// Second clock test
	lp [2] = 0;
	p [0] = p [1] = p [2] = 0;
	scan (ibuf + 1, "%lu %u %u", lp+2, p+0, p+1, p+2);
	if (lp [2] == 0)
		// One shot
		proceed RS_CLK_OS;

	lp [0] = 0;

    entry RS_CLK_LP:

	lp [1] = seconds ();
	if (lp [1] < lp [0])
		proceed RS_CLK_ERROR;
	lp [0] = lp [1];
	if (lp [0] >= lp [2])
		proceed RS_DONE;
	if (p [0] == 0 && p [1] == 0)
		// Continuous, silent
		proceed RS_CLK_LP;
	if (++p[2] == 100) {
		p [2] = 0;
		proceed RS_CLK_SHOW;
	}

    entry RS_CLK_CONT:
	delay (gen_rnd (p [0], p [1]), RS_CLK_LP);
	release;

    entry RS_CLK_SHOW:

	ser_outf (RS_CLK_SHOW, "Time: %ld\r\n", lp [0]);
	proceed RS_CLK_CONT;

    entry RS_CLK_ERROR:

	ser_outf (RS_CLK_ERROR, "ERROR: %ld %ld\r\n", lp [0], lp [1]);
	proceed RS_RCMD;

    entry RS_CLK_OS:

	ser_outf (RS_CLK_OS, "Time: %ld\r\n", seconds ());
	proceed RS_RCMD;

    entry RS_CLK_DONE:




    entry RS_BUNCH:

	address pkt;
	byte pc;

	n1 = 8;
	scan (ibuf + 1, "%d", &n1);
	if (n1 < 1)
		n1 = 1;

	pc = 0;

	while (n1) {

		k = gen_packet_length ();
		if ((pkt = tcv_wnp (WNONE, sfd, k + 2)) == NULL) {
			break;
		}
		pkt [0] = 0;
		pkt [1] = PKT_BUN | pc;

		n1--;
		pc++;

		while (k > 3)
			((byte*)pkt) [k--] = 0xAA;

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
		pkt [k/2] = xpower;
#endif
		tcv_endp (pkt);
	}

    entry RS_BUNCH1:

	ser_outf (RS_BUNCH1, "%d left\r\n", n1);
	proceed RS_RCMD;

    entry RS_RCV:

	rcv_start ();
	proceed RS_RCMD;

    entry RS_QRCV:

	rcv_stop ();
	proceed RS_RCMD;

    entry RS_QXMT:

	snd_stop ();
	proceed RS_RCMD;

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

    entry RS_QUIT1:

	ser_outf (RS_QUIT1, fmt, obuf);
	proceed RS_RCMD;

    entry RS_PAR:

	p [1] = 0;
	if (scan (ibuf + 1, "%u %u", p+0, p+1) < 1)
		proceed RS_RCMD1;

	k = tcv_control (sfd, *(p+0), p+1);

    entry RS_PAR1:

	ser_outf (RS_PAR1, "Completed: %d %u\r\n", k, p [1]);
	proceed RS_RCMD;

    entry RS_SSID:

	n1 = 0;
	scan (ibuf + 1, "%d", &n1);
	tcv_control (sfd, PHYSOPT_SETSID, (address) &n1);
	proceed RS_RCMD;

    entry RS_AUTOSTART:
	  
	snd_start (1024);
  	rcv_start ();
	finish;

    entry RS_RES:

	reset ();
	// We should be killed past this

    entry RS_PDM:

	// Power down mode
	p [0] = 0;
	p [1] = 0;
	p [2] = 0;
	scan (ibuf + 1, "%u %u %u", p+0, p+1, p+2);
	setpm (p [0]);
	if (p [1])
		runfsm reverter (p [1] | (p [2] << 8));
	proceed RS_RCMD;

#ifdef __CC1350__
	// Event count
    entry RS_GEC:

	ser_outf (RS_GEC, "EC = %lu\r\n", system_event_count);
	proceed RS_RCMD;
#endif

#ifdef PIN_OPERATIONS_INCLUDED
    entry RS_RPIN:

	if (scan (ibuf + 1, "%u", p+0) < 1)
		proceed RS_RCMD1;

	p [1] = pin_read (p [0]);

    entry RS_RPIN1:

	ser_outf (RS_RPIN1, "P[%u] = %u\r\n", p [0], p [1]);
	proceed RS_RCMD;

    entry RS_SPIN:

	if (scan (ibuf + 1, "%u %u", p+0, p+1) < 2)
		proceed RS_RCMD1;

	pin_write (p [0], p [1]);
	proceed RS_RCMD;

    entry RS_RANA:

	if (scan (ibuf + 1, "%u %u %u", p+0, &k, p+1) < 3)
		proceed RS_RCMD1;

    entry RS_RANA1:

	n1 = pin_read_adc (RS_RANA1, p [0], k, p [1]);

    entry RS_RANA2:

	ser_outf (RS_RANA2, "A[%u] = %d\r\n", p [0], n1);
	proceed RS_RCMD;

    entry RS_WANA:

	if (scan (ibuf + 1, "%u %u %u", p+0, &k, p+1) < 3)
		proceed RS_RCMD1;

	pin_write_dac (p [0], k, p [1]);
	proceed RS_RCMD;

#ifdef PMON_NOTEVENT

    entry RS_PSCN:

	k = 0;
	lp [0] = 0;
	scan (ibuf + 1, "%lu %u", lp, &k);
	pmon_start_cnt (lp [0], (Boolean) k);
	proceed RS_RCMD;

    entry RS_PSCM:

	if (scan (ibuf + 1, "%lu", lp) < 1)
		proceed RS_RCMD1;

	pmon_set_cmp (lp [0]);
	proceed RS_RCMD;

    entry RS_PGCN:

	lp [0] = pmon_get_cnt ();

    entry RS_PGCN1:

	ser_outf (RS_PGCN1, "Counter = %lu\r\n", lp [0]);
	proceed RS_RCMD;

    entry RS_PQCN:

	pmon_stop_cnt ();
	proceed RS_RCMD;

    entry RS_PSNT:

	k = 0;
	scan (ibuf + 1, "%u", &k);
	pmon_start_not ((Boolean) k);
	proceed RS_RCMD;

    entry RS_PQNT:

	pmon_stop_not ();
	proceed RS_RCMD;

    entry RS_PSMT:

	if (running (pin_monitor))
		proceed RS_PSMT1;

	runfsm pin_monitor;
	proceed RS_RCMD;

    entry RS_PSMT1:

	ser_out (RS_PSMT1, "Already running!\r\n");
	proceed RS_RCMD;

    entry RS_PQMT:

	killall (pin_monitor);
	proceed RS_RCMD;

#endif /* PMON_NOTEVENT */
#endif /* PIN_OPERATIONS_INCLUDED */

    entry RS_SETP:

	if (scan (ibuf + 1, "%u", p+0) < 1)
		proceed RS_RCMD1;

	if (p [0] > 7)
		proceed RS_RCMD1;

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
	xpower = p [0] << 12;
#else
	tcv_control (sfd, PHYSOPT_SETPOWER, p);
#endif
	proceed RS_RCMD;

    entry RS_GETP:

	ser_outf (RS_GETP, "P = %d\r\n",
		tcv_control (sfd, PHYSOPT_GETPOWER, NULL));
	proceed RS_RCMD;

#ifdef SENSOR_OPERATIONS_INCLUDED
    entry RS_GETS:

	if (scan (ibuf + 1, "%u", p+0) < 1)
		proceed RS_RCMD1;

    entry RS_GETS1:
	// Assume these sensors handle 2-byte values
	read_sensor (RS_GETS1, p [0], p + 1);

    entry RS_GETS2:

	ser_outf (RS_GETS2, "V = %u\r\n", p [1]);
	proceed RS_RCMD;
#endif

#ifdef ACTUATOR_OPERATIONS_INCLUDED
    entry RS_SETA:

	if (scan (ibuf + 1, "%u %u", p+0, p+1) < 1)
		proceed RS_RCMD1;

    entry RS_SETA1:

	write_actuator (RS_SETA1, p [0], p + 1);
	proceed RS_RCMD;
#endif

    entry RS_DONE:

	ser_out (RS_DONE, "Done\r\n");
	proceed RS_RCMD;

}
