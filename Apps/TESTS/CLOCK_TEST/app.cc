/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"

#define	MIN_PACKET_LENGTH	24
#define	MAX_PACKET_LENGTH	46

#include "ser.h"
#include "serf.h"
#include "form.h"
#include "plug_ack.h"
#include "hold.h"
#include "phys_cc1100.h"

#define	IBUFLEN		82

#define	PKT_DAT		0x01

#define MAXPLEN		(MAX_PACKET_LENGTH + 2)

heapmem {10, 90};

int 	sfd = -1;
word 	tdelay;

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

fsm bursty {

	state BU_START:

		word w;

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
}

fsm delayer {

	lword ovs;
	word ovp [3];

	state DE_START:

		word w;
		address p;

		if (max_delay == 0) {
			when (&max_delay, DE_START);
			release;
		}

		if ((p = tcv_overtime_check (&ovs)) != NULL) {
			ovp [0] = p [1];
			ovp [1] = p [2];
			ovp [2] = p [3];
			proceed DE_REPORT;
		}
Wait:
		w = rnd_u (min_delay, max_delay);
		// diag ("delay %u", w);
		delay (w, DE_START);
		release;

	state DE_REPORT:

		ser_outf (DE_REPORT,
			"TCV TIMER OVERDELAY: (%lu > %lu): %x %x %x\r\n",
				ovs, seconds (), ovp [0], ovp [1], ovp [2]);
		goto Wait;
}

// ============================================================================

fsm receiver {

    address packet;
    word mem [2];

    state RC_TRY:

	packet = tcv_rnp (RC_TRY, sfd);
	mem [0] = memfree (0, mem + 1);

    state RC_DATA:

	ser_outf (RC_DATA, "R: [%x] %lu (%u) RS:%d [%u %u %u]\r\n",
		packet [1], ((lword*)packet) [1],
		tcv_left (packet) - 2,
		((byte*)packet) [tcv_left (packet) - 1],
		mem [0], mem [1], n_free_hooks ()
	);

	tcv_endp (packet);
	proceed RC_TRY;
}

fsm sender {

    lint last_snt = 0;
    address packet;
    word packet_length, mem [2];

    entry SN_SEND:

	packet_length = gen_packet_length ();

	if (packet_length < 10)
		packet_length = 10;
	else if (packet_length > MAX_PACKET_LENGTH)
		packet_length = MAX_PACKET_LENGTH;

    entry SN_NEXT:

    	word pl;
    	int  pp;

	// We control congestion in the transmission queue via a simple
	// handshake with the plugin
	if (n_free_hooks () == 0) {
		// The plugin has run out of hooks, wait until one is freed
		delay (16, SN_NEXT);
		release;
	}

	packet = tcv_wnp (SN_NEXT, sfd, packet_length + 2);

	packet [0] = 0;	// This will be filled in by the PHY
	ptype (packet) = PKT_DAT;
	psernum (packet) = (byte) last_snt;

	// In words
	pl = packet_length / 2;
	((lword*)packet)[1] = last_snt;

	for (pp = 4; pp < pl; pp++)
		packet [pp] = (word) (entropy);

	tcv_endp (packet);
	mem [0] = memfree (0, mem + 1);

    entry SN_OUTM:

	ser_outf (SN_OUTM, "S: %lu (%u) [%u %u %u]\r\n",
		last_snt, packet_length, mem [0], mem [1], n_free_hooks ());
	last_snt++;
	delay (tdelay, SN_SEND);
}

static void radio_switch (Boolean on) {
//
// Note: because of the internal acknowledgments, both components (RX, TX) must
// be on for reception (as well as for transmission)

	if (on) {
		tcv_control (sfd, PHYSOPT_RXON, NULL);
		tcv_control (sfd, PHYSOPT_TXON, NULL);
	} else {
		tcv_control (sfd, PHYSOPT_RXOFF, NULL);
		tcv_control (sfd, PHYSOPT_TXOFF, NULL);
	}
}

int rcv_start () {

	radio_switch (YES);
	if (!running (receiver)) {
		runfsm receiver;
		return 1;
	}
	return 0;
}

int rcv_stop () {

	int res;

	if ((res = running (receiver)))
		killall (receiver);

	if (!running (sender))
		radio_switch (NO);

	return res;
}

int snd_start (int del) {

	radio_switch (YES);
	tdelay = del;
	if (!running (sender)) {
		runfsm sender;
		return 1;
	}

	return 0;
}

int snd_stop () {

	int res;

	if ((res = running (sender)))
		killall (sender);

	if (!running (receiver))
		radio_switch (NO);

	return res;
}

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

extern word __pi_old, __pi_new, __pi_mintk;

fsm delay_monitor (dmon_data_t*) {

    lword ovs;
    word ovp [3];

    state DM_START:

	data->lastdel = rnd ();
	data->lastsec = seconds ();
	if (data->lastdel > 1200) 
	delay (data->lastdel, DM_DONE);
	release;

    state DM_DONE:

        word d;
	address p;

	data->count++;
	data->millisec += data->lastdel;

	// Requested delay truncated to seconds
	d = data->lastdel >> 10;

	// Elapsed delay in seconds
	data->elapsed = (word) (seconds () - data->lastsec);

	if (data->elapsed < d)
		proceed DM_UNDER;

	if (data->elapsed > d + 2)
		proceed DM_OVER;
T_check:

	if ((p = tcv_overtime_check (&ovs)) != NULL) {
		ovp [0] = p [1];
		ovp [1] = p [2];
		ovp [2] = p [3];
		proceed DM_TCV_OVER;
	}

	proceed DM_START;

    state DM_UNDER:

	ser_outf (DM_UNDER, "UNDERDELAY%u: %u %u %u %u %u\r\n",
		data - dmond,
		data->lastdel, data->elapsed,
		__pi_old, __pi_new, __pi_mintk);
	goto T_check;

    state DM_OVER:

	ser_outf (DM_OVER, "OVERDELAY%u: %u %u %u %u %u\r\n",
		data - dmond,
		data->lastdel, data->elapsed,
		__pi_old, __pi_new, __pi_mintk);
	goto T_check;

    state DM_TCV_OVER:

	ser_outf (DM_TCV_OVER, "TCV TIMER OVERDELAY: (%lu > %lu): %x %x %x\r\n",
		ovs, seconds (), ovp [0], ovp [1], ovp [2]);
	proceed DM_START;
}

// ============================================================================

fsm delay_reporter {

    lword reptime = 0;
    word dm;

    state DR_START:

	// Create the processes
	for (dm = 0; dm < N_DMONS; dm++)
		dmons [dm] = runfsm delay_monitor (dmond + dm);

    state DR_LOOP:

	reptime += DREP_INT;

    state DR_WAIT:

	hold (DR_WAIT, reptime);
	dm = 0;

    state DR_REPORT:

	if (dm == N_DMONS)
		proceed DR_LOOP;

    state DR_OUT:

	ser_outf (DR_OUT,
		"DM%u, s = %lu, ms = %lu, dl = %lu, cn = %lu, ho = %u\r\n",
		dm,
		dmond [dm] . lastsec,
		dmond [dm] . lastsec * 1024,
		dmond [dm] . millisec,
		dmond [dm] . count,
		n_free_hooks ());

	dm++;
	proceed DR_REPORT;
}

// ============================================================================

fsm watchdog {

  state WA_START:

	watchdog_start ();

  state WA_WAKE:

	watchdog_clear ();
	delay (300, WA_WAKE);
}

// ============================================================================

trueconst static word parm_power = 255;

fsm root {

  lword lw;
  char *ibuf;
  int k, n1;
  char *fmt, obuf [32];
  word p [2];
  word n;

  state RS_INIT:

	ibuf = (char*) umalloc (IBUFLEN);
	ibuf [0] = 0;

	phys_cc1100 (0, MAXPLEN);

	// runfsm watchdog;
	runfsm bursty;
	runfsm delayer;
	runfsm delay_reporter;

	tcv_plug (0, &plug_ack);
	sfd = tcv_open (NONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

	tcv_control (sfd, PHYSOPT_SETPOWER, (address) &parm_power);

  state RS_RCMDM2:

	ser_out (RS_RCMDM2,
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
		"k        -> show hooks\r\n"

#if UART_RATE_SETTABLE
		"S r      -> set UART rate\r\n"
		"G        -> get UART rate\r\n"
#endif
		"D        -> power down mode\r\n"
		"U        -> power up mode\r\n"
		"L n      -> loop for n msec\r\n"
		"M mis mas mii mai mid mad\r\n"
	);

  state RS_RCMDM1:

	if ((unsigned char) ibuf [0] == 0xff)
		ser_out (RS_RCMDM1,
			"No command in 10 seconds -> start s 1024, r\r\n"
			);
  state RS_RCMD:

	if ((unsigned char) ibuf [0] == 0xff)
		delay (1024*10, RS_AUTOSTART);
  
	k = ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {

		case 'c': proceed RS_SEC;
		case 's': proceed RS_SND;
		case 'r': proceed RS_RCV;
		case 'l': proceed RS_LED;
#if STACK_GUARD
		case 'v': proceed RS_STK;
#endif

#if UART_RATE_SETTABLE
		case 'S': proceed RS_URS;
		case 'G': proceed RS_URG;
#endif

		case 'D': proceed RS_LPM;
		case 'U': proceed RS_HPM;
		case 'M': proceed RS_DST;
		case 'L': proceed RS_LOP;
		case 'p': proceed RS_POW;
		case 'g': proceed RS_RCP;
		case 'q': proceed RS_QUIT;
		case 'o': proceed RS_QRCV;
		case 't': proceed RS_QXMT;
		case 'i': proceed RS_SSID;
		case 'k': tcv_check_hooks (); proceed RS_RCMD;
	}

  state RS_RCMDP1:

	ser_out (RS_RCMDP1, "Illegal command or parameter\r\n");
	proceed (RS_RCMDM2);

  state RS_SEC:

	lw = LWNONE;
	scan (ibuf + 1, "%lu", &lw);
	if (lw != LWNONE) {
		setseconds (lw);
		proceed (RS_RCMD);
	}

  state RS_SECP1:

	ser_outf (RS_SECP1, "Time: %lu\r\n", seconds ());
	proceed (RS_RCMD);

  state RS_SND:

	/* Default */
	n1 = 2048;
	scan (ibuf + 1, "%u", &n1);
	if (n1 < 16)
		n1 = 16;
	snd_start (n1);

  state RS_SNDP1:

	ser_outf (RS_SNDP1, "Sender rate: %u\r\n", n1);
	proceed (RS_RCMD);

  state RS_RCV:

	rcv_start ();
	proceed (RS_RCMD);

  state RS_POW:

	/* Default */
	n1 = 0;
	scan (ibuf + 1, "%u", &n1);
	tcv_control (sfd, PHYSOPT_SETPOWER, (address)&n1);

  state RS_POWP1:

	ser_outf (RS_POWP1,
		"Transmitter power set to %u\r\n", n1);

	proceed (RS_RCMD);

  state RS_RCP:

	n1 = tcv_control (sfd, PHYSOPT_GETPOWER, NULL);

  state RS_RCPP1:

	ser_outf (RS_RCPP1,
		"Received power is %u\r\n", n1);

	proceed (RS_RCMD);

  state RS_QRCV:

	rcv_stop ();
	proceed (RS_RCMD);

  state RS_QXMT:

	snd_stop ();
	proceed (RS_RCMD);

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
	proceed (RS_RCMD);

  state RS_SSID:

	n1 = 0;
	scan (ibuf + 1, "%u", &n1);
	tcv_control (sfd, PHYSOPT_SETSID, (address) &n1);
	proceed (RS_RCMD);

#if STACK_GUARD

  state RS_STK:

	ser_outf (RS_STK, "Free stack space = %u words\r\n", stackfree ());
	proceed (RS_RCMD);
#endif

  state RS_LED:

	p [0] = 0;
	p [1] = 0;
	scan (ibuf + 1, "%u %u", p+0, p+1);
	leds (p [0], p [1]);
	proceed (RS_RCMD);

#if UART_RATE_SETTABLE

  state RS_URS:

	scan (ibuf + 1, "%u", &p [0]);
	ion (UART, CONTROL, (char*) p, UART_CNTRL_SETRATE);
	proceed (RS_RCMD);

  state RS_URG:

	ion (UART, CONTROL, (char*) p, UART_CNTRL_GETRATE);

  state RS_URGP1:
	
	ser_outf (RS_URGP1, "Rate: %u[00]\r\n", p [0]);
	proceed (RS_RCMD);
#endif

  state RS_LPM:

	powerdown ();

  state RS_LPMP1:

	ser_out (RS_LPMP1, "Power-down mode\r\n");
	proceed (RS_RCMD);

  state RS_HPM:

	powerup ();

  state RS_HPMP1:

	ser_out (RS_HPMP1, "Power-up mode\r\n");
	proceed (RS_RCMD);

  state RS_DST:

	zero_dstp ();
	scan (ibuf + 1, "%u %u %u %u %u %u",
		&min_spin_delay, &max_spin_delay, 
		&min_inter_spin_delay, &max_inter_spin_delay,
		&min_delay, &max_delay);

	if (max_spin_delay != 0 && max_spin_delay < min_spin_delay) {
DST_err:
		zero_dstp ();
		proceed (RS_RCMDP1);
	}

	if (max_inter_spin_delay != 0 &&
	    max_inter_spin_delay < min_inter_spin_delay)
		goto DST_err;

	if (max_delay != 0 && max_delay < min_delay)
		goto DST_err;

	trigger (&max_spin_delay);
	trigger (&max_delay);

  state RS_DSTP1:

	ser_outf (RS_DSTP1, "Params: %u %u %u %u %u %u\r\n",
		min_spin_delay, max_spin_delay, 
		min_inter_spin_delay, max_inter_spin_delay,
		min_delay, max_delay);

	proceed (RS_RCMD);

  state RS_LOP:

	n1 = 1;
	scan (ibuf + 1, "%u", &n1);
	mdelay (n1);

  state RS_DON:

	ser_out (RS_DON, "Done\r\n");
	proceed (RS_RCMD);

  state RS_AUTOSTART:
	  
	snd_start (1024);
  	rcv_start ();

  	finish;
}
