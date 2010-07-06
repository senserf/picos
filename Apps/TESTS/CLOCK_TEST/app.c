/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"

#define	MIN_PACKET_LENGTH	24
#define	MAX_PACKET_LENGTH	48

#include "ser.h"
#include "serf.h"
#include "form.h"
#include "plug_null.h"
#include "hold.h"

#include "phys_cc1100.h"

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
static	lint	last_snt, last_rcv, last_ack;
static  char 	XMTon = 0, RCVon = 0;
static	word	min_spin_delay       = 0,
		max_spin_delay       = 0,
		min_inter_spin_delay = 0,
		max_inter_spin_delay = 0,
		min_delay            = 0,
		max_delay	     = 0;

static void zero_dstp () {

		min_spin_delay = max_spin_delay =
		min_inter_spin_delay = max_inter_spin_delay =
		min_delay = max_delay = 0;
}

static word rnd_u (word min, word max) {

	return (word)(rnd () % (max - min + 1)) + min;
}

static word gen_packet_length (void) {

	word w;

#if MIN_PACKET_LENGTH >= MAX_PACKET_LENGTH
	w = MIN_PACKET_LENGTH;
#else
	w = rnd_u (MIN_PACKET_LENGTH, MAX_PACKET_LENGTH);
#endif
	return w & ~1;
}

#define	BU_START	0

thread (bursty)

	word w;

	entry (BU_START)

		if (max_spin_delay == 0) {
			when (&max_spin_delay, BU_START);
			release;
		}

		w = rnd_u (min_spin_delay, max_spin_delay);
		// diag ("mdelay %u", w);
		mdelay (w);

		w = rnd_u (min_inter_spin_delay, max_inter_spin_delay);
		// diag ("is delay %u", w);

		delay (w, BU_START);
		when (&max_spin_delay, BU_START);
endthread

#define	DE_START	0

thread (delayer)

	word w;

	entry (DE_START)

		if (max_delay == 0) {
			when (&max_delay, DE_START);
			release;
		}

		w = rnd_u (min_delay, max_delay);
		// diag ("delay %u", w);
		delay (w, DE_START);
endthread

// ============================================================================

thread (receiver)

	static address packet;

  entry (RC_TRY)

	if (rkillflag) {
		rkillflag = 0;
		finish;
	}
	wait (&rkillflag, RC_TRY);
	packet = tcv_rnp (RC_TRY, sfd);
	if (packet [1] == PKT_ACK) {
		last_ack = ntowl (((lword*)packet) [1]);
		proceed (RC_ACK);
	}

	// Data packet
	last_rcv = ntowl (((lword*)packet) [1]);

  entry (RC_DATA)

	ser_outf (RC_DATA,
	    "%lu RCV: [%x] %lu %lu (len = %u), pow = %u qua = %u\r\n",
		seconds (),
		packet [1],
		last_rcv,
		((lword*)packet) [2],
		tcv_left (packet) - 2,
#if LITTLE_ENDIAN
		((byte*)packet) [tcv_left (packet) - 1],
		((byte*)packet) [tcv_left (packet) - 2]
#else
		((byte*)packet) [tcv_left (packet) - 2],
		((byte*)packet) [tcv_left (packet) - 1]
#endif
	);

	tcv_endp (packet);

	// Acknowledge it

  entry (RC_SACK)

	if (XMTon) {
		packet = tcv_wnp (RC_SACK, sfd, ACK_LENGTH);
		packet [0] = 0;
		packet [1] = PKT_ACK;
		((lword*)packet) [1] = wtonl (last_rcv);

		packet [4] = (word) entropy;
		tcv_endp (packet);
	}
	proceed (RC_TRY);

  entry (RC_ACK)

	ser_outf (RC_ACK, "%lu ACK: %lu (len = %u)\r\n", seconds (), last_ack,
		tcv_left (packet));

	tcv_endp (packet);
	trigger (&last_ack);
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
	if (packet_length < 12)
		packet_length = 12;

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

	((lword*)packet)[2] = seconds ();

	for (pp = 6; pp < pl; pp++)
		packet [pp] = (word) entropy;

	tcv_endp (packet);

  entry (SN_NEXT+1)

	ser_outf (SN_NEXT+1,
	    "%lu SND %lu, len = %u\r\n", seconds (), last_snt, packet_length);
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

// ============================================================================

#define	N_DMONS		4
#define	DREP_INT	300

typedef struct {

	lword	millisec, lastsec, count;
	word	lastdel;
	word	elapsed;

} dmon_data_t;

dmon_data_t dmond [N_DMONS];

int dmons [N_DMONS];

// ============================================================================

#define	DM_START	0
#define	DM_DONE		1
#define	DM_UNDER	2
#define	DM_OVER		3

extern word zz_old, zz_new, zz_mintk;

strand (delay_monitor, dmon_data_t)

	word d;

	entry (DM_START)

		data->lastdel = rnd ();
		data->lastsec = seconds ();
		if (data->lastdel > 1200) 
		delay (data->lastdel, DM_DONE);
		release;

	entry (DM_DONE)

		data->count++;
		data->millisec += data->lastdel;

		// Requested delay truncated to seconds
		d = data->lastdel >> 10;

		// Elapsed delay in seconds
		data->elapsed = (word) (seconds () - data->lastsec);

		if (data->elapsed < d)
			proceed (DM_UNDER);

		if (data->elapsed > d + 2)
			proceed (DM_OVER);

		proceed (DM_START);

	entry (DM_UNDER)

		ser_outf (DM_UNDER, "UNDERDELAY%u: %u %u %u %u %u\r\n",
			data - dmond,
			data->lastdel, data->elapsed, zz_old, zz_new, zz_mintk);

		proceed (DM_START);

	entry (DM_OVER)

		ser_outf (DM_OVER, "OVERDELAY%u: %u %u %u %u %u\r\n",
			data - dmond,
			data->lastdel, data->elapsed, zz_old, zz_new, zz_mintk);

		proceed (DM_START);
			
endstrand

// ============================================================================

#define	DR_START	0
#define	DR_LOOP		1
#define	DR_WAIT		2
#define	DR_REPORT	3
#define	DR_OUT		4

thread (delay_reporter)

	static lword reptime = 0;
	static word dm;

	entry (DR_START)

		// Create the processes
		for (dm = 0; dm < N_DMONS; dm++)
			dmons [dm] = runstrand (delay_monitor, dmond + dm);

	entry (DR_LOOP)

		reptime += DREP_INT;

	entry (DR_WAIT)

		hold (DR_WAIT, reptime);
		dm = 0;

	entry (DR_REPORT)

		if (dm == N_DMONS)
			proceed (DR_LOOP);

	entry (DR_OUT)

		ser_outf (DR_OUT,
			"DM%u, s = %lu, ms = %lu, dl = %lu, cn = %lu\r\n",
			dm,
			dmond [dm] . lastsec,
			dmond [dm] . lastsec * 1024,
			dmond [dm] . millisec,
			dmond [dm] . count);

		dm++;
		proceed (DR_REPORT);
endthread

// ============================================================================

#define	WA_START 	0
#define	WA_WAKE		1

thread (watchdog)

  entry (WA_START)

	watchdog_start ();

  entry (WA_WAKE)

	watchdog_clear ();
	delay (300, WA_WAKE);

endthread

// ============================================================================

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_SEC		20
#define	RS_SND		30
#define RS_RCV		40
#define	RS_POW		50
#define	RS_RCP		60
#define	RS_QRCV		70
#define	RS_QXMT		80
#define	RS_QUIT		90
#define	RS_SSID		110
#define	RS_STK		120
#define	RS_LED		130
#define	RS_URS		140
#define	RS_URG		150
#define	RS_LPM		160
#define	RS_HPM		170
#define	RS_DST		180
#define	RS_LOP		190
#define	RS_DON		200
#define	RS_AUTOSTART	300

const static word parm_power = 255;

thread (root)

	static lword lw;
	static char *ibuf;
	static int k, n1;
	static char *fmt, obuf [32];
	static word p [2];
	static word n;

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFLEN);
	ibuf [0] = 0;

	phys_cc1100 (0, MAXPLEN);

	// runthread (watchdog);
	runthread (bursty);
	runthread (delayer);
	runthread (delay_reporter);

	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

	tcv_control (sfd, PHYSOPT_SETPOWER, (address) &parm_power);

  entry (RS_RCMD-2)

	ser_out (RS_RCMD-2,
		"\r\nRF Ping/Clock Test\r\n"
		"Commands:\r\n"
		"c val    -> set seconds clock to val\r\n"
		"s intvl  -> start/reset sending interval (2 secs default)\r\n"
		"r        -> start receiver\r\n"
		"d i v    -> change phys parameter i to v\r\n"
		"c btime  -> recalibrate the transceiver\r\n"
		"p v      -> set transmit power\r\n"
		"g        -> get received power\r\n"
		"o        -> stop receiver\r\n"
		"t        -> stop transmitter\r\n"
		"q        -> stop both\r\n"
		"i        -> set station Id\r\n"
		"l n s    -> led n [012]\r\n"
#if STACK_GUARD
		"v        -> show unused stack space\r\n"
#endif

#if UART_RATE_SETTABLE
		"S r      -> set UART rate\r\n"
		"G        -> get UART rate\r\n"
#endif
		"D        -> power down mode\r\n"
		"U        -> power up mode\r\n"
		"L n      -> loop for n msec\r\n"
		"M mis mas mii mai mid mad\r\n"
		);

  entry (RS_RCMD-1)

	if ((unsigned char) ibuf [0] == 0xff)
		ser_out (RS_RCMD-1,
			"No command in 10 seconds -> start s 1024, r\r\n"
			);
  entry (RS_RCMD)

	if ((unsigned char) ibuf [0] == 0xff)
		delay (1024*10, RS_AUTOSTART);
  
	k = ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {

		case 'c': proceed (RS_SEC);
		case 's': proceed (RS_SND);
		case 'r': proceed (RS_RCV);
		case 'l': proceed (RS_LED);
#if STACK_GUARD
		case 'v': proceed (RS_STK);
#endif

#if UART_RATE_SETTABLE
		case 'S': proceed (RS_URS);
		case 'G': proceed (RS_URG);
#endif

		case 'D': proceed (RS_LPM);
		case 'U': proceed (RS_HPM);
		case 'M': proceed (RS_DST);
		case 'L': proceed (RS_LOP);
				
	}

	if (ibuf [0] == 'p')
		proceed (RS_POW);

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

  entry (RS_SEC)

	lw = LWNONE;
	scan (ibuf + 1, "%lu", &lw);
	if (lw != LWNONE) {
		setseconds (lw);
		proceed (RS_RCMD);
	}

  entry (RS_SEC+1)

	ser_outf (RS_SEC+1, "Time: %lu\r\n", seconds ());
	proceed (RS_RCMD);

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

  entry (RS_POW)

	/* Default */
	n1 = 0;
	scan (ibuf + 1, "%u", &n1);
	tcv_control (sfd, PHYSOPT_SETPOWER, (address)&n1);

  entry (RS_POW+1)

	ser_outf (RS_POW+1,
		"Transmitter power set to %u\r\n", n1);

	proceed (RS_RCMD);

  entry (RS_RCP)

	n1 = tcv_control (sfd, PHYSOPT_GETPOWER, NULL);

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

  entry (RS_SSID)

	n1 = 0;
	scan (ibuf + 1, "%u", &n1);
	tcv_control (sfd, PHYSOPT_SETSID, (address) &n1);
	proceed (RS_RCMD);

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

  entry (RS_LPM)

	powerdown ();

  entry (RS_LPM+1)

	ser_out (RS_LPM+1, "Power-down mode\r\n");
	proceed (RS_RCMD);

  entry (RS_HPM)

	powerup ();

  entry (RS_HPM+1)

	ser_out (RS_HPM+1, "Power-up mode\r\n");
	proceed (RS_RCMD);

  entry (RS_DST)

	zero_dstp ();
	scan (ibuf + 1, "%u %u %u %u %u %u",
		&min_spin_delay, &max_spin_delay, 
		&min_inter_spin_delay, &max_inter_spin_delay,
		&min_delay, &max_delay);

	if (max_spin_delay != 0 && max_spin_delay < min_spin_delay) {
DST_err:
		zero_dstp ();
		proceed (RS_RCMD+1);
	}

	if (max_inter_spin_delay != 0 &&
	    max_inter_spin_delay < min_inter_spin_delay)
		goto DST_err;

	if (max_delay != 0 && max_delay < min_delay)
		goto DST_err;

	trigger (&max_spin_delay);
	trigger (&max_delay);

  entry (RS_DST+1)

	ser_outf (RS_DST+1, "Params: %u %u %u %u %u %u\r\n",
		min_spin_delay, max_spin_delay, 
		min_inter_spin_delay, max_inter_spin_delay,
		min_delay, max_delay);

	proceed (RS_RCMD);

  entry (RS_LOP)

	n1 = 1;
	scan (ibuf + 1, "%u", &n1);
	mdelay (n1);

  entry (RS_DON)

	ser_out (RS_DON, "Done\r\n");
	proceed (RS_RCMD);

  entry (RS_AUTOSTART)
	  
	snd_start (1024);
  	rcv_start ();

  	finish;

endthread
