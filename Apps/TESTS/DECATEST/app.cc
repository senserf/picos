#include "sysio.h"
#include "tcvphys.h"
#include "ser.h"
#include "serf.h"
#include "form.h"
#include "cc1100.h"
#include "phys_cc1100.h"
#include "plug_null.h"

#include "dw1000.h"
#include "sensors.h"
#include "actuators.h"

#include "buttons.h"

#define	DEFAULT_ROLE	0
#define	DEFAULT_MODE	0
#define	PAN		0xC0C0

// ============================================================================

#define	IBUFLEN	72

// ============================================================================

sint	sfd, thefsm;

dw1000_locdata_t location;

// ============================================================================

void dumpbytes (byte *buf, word len) {

	byte is;
	word ix;

	diag_disable_int (a, is);

	for (ix = 0; ix < len; ix++) {

		if ((ix & 0xF) == 0) {
			diag_wait (a); diag_wchar ('\r', a);
			diag_wait (a); diag_wchar ('\n', a);
		}

		diag_wait (a); diag_wchar (' ', a);
		diag_wait (a); diag_wchar (
			__pi_hex_enc_table [((*buf) >> 4) & 0xf], a);
		diag_wait (a); diag_wchar (
			__pi_hex_enc_table [((*buf)     ) & 0xf], a);
		buf++;
	}

	diag_wait (a); diag_wchar ('\r', a);
	diag_wait (a); diag_wchar ('\n', a);

	diag_enable_int (a, is);
}

// ============================================================================

fsm blink_off {

    state SWITCH_ON:

	delay (1024, SWITCH_OFF);
	release;

    state SWITCH_OFF:

	leds (0, 0);
	finish;
}

static void blink () {

	leds (0, 1);

	if (!running (blink_off))
		runfsm blink_off;
}

// ============================================================================

fsm peg_thread {

    word cnt;

    state EVENT:

	read_sensor (EVENT, SENSOR_RANGE, (address)&location);
	if (location.tag == 0) {
		wait_sensor (SENSOR_RANGE, EVENT);
		release;
	}

    state SHOW:

	ser_outf (SHOW, "r %u %u\r\n", location.tag, location.seq);
	cnt = 0;

    state SHOWTS:

	lword lw;

	if (cnt >= 6 * DW1000_TSTAMP_LEN)
		sameas EVENT;

	memcpy (&lw, location.tst + cnt, 4);

#if DW1000_TSTAMP_LEN == 4
	ser_outf (SHOWTS, "%lx\r\n", lw);
#else
	ser_outf (SHOWTS, "%x%lx\r\n", location.tst [cnt + 4], lw);
#endif

	cnt += DW1000_TSTAMP_LEN;
	sameas SHOWTS;
}

static void startup (byte m, byte r) {

	if (r) {
		thefsm = runfsm peg_thread;
		powerup ();
	}

	dw1000_start (m, r, PAN);
}

static void range (word st) {

	byte seq;

	if (thefsm)
		// Illegal in Peg mode
		return;

	seq = (byte) seconds ();
	write_actuator (st, ACTUATOR_RANGE, (address)&seq);
	blink ();
}

// ============================================================================

static void button (word but) {

	trigger (&location);

}

// ============================================================================

fsm root {

    char *ibuf;

    state INIT:

	ibuf = (char*) umalloc (IBUFLEN);

	// We also test the coexistence of CC1100 with DecaWave
	phys_cc1100 (0, CC1100_MAXPLEN);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (WNONE, 0, 0);

	if (sfd < 0) {
		diag ("fail cc1100");
		halt ();
	}

	buttons_action (button);

	blink ();

#ifdef	DEFAULT_ROLE
	// Start up in the default mode
	startup (DEFAULT_MODE, DEFAULT_ROLE);
#endif
	delay (1024, BANNER);
	release;

    state BANNER:

	ser_outf (BANNER,
		"\r\nDECA TEST, NId = %u\r\n"
		"d 0 - off\r\n"
		"d 1 m r - on\r\n"
		"s - poll\r\n"
#if (DW1000_OPTIONS & 0x0001)
		"p msec - pull cs\r\n"
		"r r o n - read\r\n"
		"w r o n v - write\r\n"
#endif
		"q - reset\r\n"
			, (word)host_id
	);

    state COMMAND:

	ibuf [0] = 0;
	when (&location, POLL);
	ser_in (COMMAND, ibuf, IBUFLEN);

	switch (ibuf [0]) {

		case 'd': proceed DECAONOFF;
		case 's': proceed POLL;
#if (DW1000_OPTIONS & 0x0001)
		case 'p': proceed PULLCS;
		case 'r': proceed REG_READ;
		case 'w': proceed REG_WRITE;
#endif
		case 'q': {
				DW1000_RESET;
				proceed WOK;
		}

	}

    state ILLEGAL:

	ser_out (ILLEGAL, "Illegal!\r\n");
	proceed BANNER;

    state DECAONOFF:

	sint on, mode, role;

	on = mode = role = -1;
	scan (ibuf + 1, "%u %u %u", &on, &mode, &role);

	if (on < 0)
		proceed ILLEGAL;

	// Always off before doing anything else
	dw1000_stop ();
	if (thefsm) {
		kill (thefsm);
		thefsm = 0;
	}
	powerdown ();

	if (on) {

		if (mode < 0 || mode > 7)
			proceed ILLEGAL;

		if (role != 0 && role != 1)
			proceed ILLEGAL;

		startup ((byte)mode, (byte)role);
	}

    state WOK:

	ser_out (WOK, "OK\r\n");
	proceed COMMAND;

    state POLL:

	range (POLL);
	proceed COMMAND;

#if (DW1000_OPTIONS & 0x0001)
    state PULLCS:	// Test CS pull

	word tm;

	tm = 1;
	scan (ibuf + 1, "%u", &tm);

	DW1000_SPI_START;
	delay (tm, PULLCS_DONE);
	release;

    state PULLCS_DONE:

	DW1000_SPI_STOP;
	proceed WOK;

    state REG_READ:

	{
		word i, reg, ind, len; byte *buf;

		reg = WNONE;
		ind = 0;
		len = 1;

		scan (ibuf + 1, "%x %u %u", &reg, &ind, &len);
		if (reg == WNONE)
			proceed ILLEGAL;
		if (len < 1 || len > 64)
			proceed ILLEGAL;

		if ((buf = (byte*)umalloc (len)) == NULL)
			proceed ILLEGAL;

		chip_read ((byte)reg, ind, len, buf);

		dumpbytes (buf, len);
		ufree (buf);
	}

	proceed WOK;

    state REG_WRITE:

	{
		word i, reg, ind, len; lword val; byte *buf;

		reg = WNONE;
		ind = 0;
		len = 1;

		scan (ibuf + 1, "%x %u %u %lx", &reg, &ind, &len, &val);
		if (reg == WNONE)
			proceed ILLEGAL;
		if (len != 1 && len != 2 && len != 4)
			proceed ILLEGAL;
		chip_write ((byte)reg, ind, len, (byte*)&val);
	}

	proceed WOK;
#endif
}
