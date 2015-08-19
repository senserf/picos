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

	if (cnt >= 30)
		sameas EVENT;

	memcpy (&lw, location.tst + cnt, 4);

	ser_outf (SHOWTS, "%x%lx\r\n", location.tst [cnt + 4], lw);

	cnt += 5;
	sameas SHOWTS;
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

	leds (0, 1);

    state BANNER:

	ser_outf (BANNER,
		"\r\nDECA TEST, NId = %u\r\n"
		"d 0 - off\r\n"
		"d 1 m r - on\r\n"
		"s - poll\r\n"
		"p msec - pull cs\r\n"
		"r r o n - read\r\n"
		"w r o n v - write\r\n"
		"q - reset\r\n"
			, (word)host_id
	);

    state COMMAND:

	ibuf [0] = 0;
	ser_in (COMMAND, ibuf, IBUFLEN);

	switch (ibuf [0]) {

		case 'd': proceed DECAONOFF;
		case 's': proceed POLL;
		case 'p': proceed PULLCS;
		case 'r': proceed REG_READ;
		case 'w': proceed REG_WRITE;
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

	if (on == 0) {
		dw1000_stop ();
		if (thefsm) {
			kill (thefsm);
			thefsm = 0;
		}
		powerdown ();
		leds (0, 0);
		proceed WOK;
	}

	if (mode < 0 || mode > 7)
		proceed ILLEGAL;

	if (role != 0 && role != 1)
		proceed ILLEGAL;

	if (thefsm)
		proceed ILLEGAL;

	if (role)
		thefsm = runfsm peg_thread;

	powerup ();
	leds (0, 1);
	dw1000_start ((byte)mode, (byte)role, 0xC0C0);

    state WOK:

	ser_out (WOK, "OK\r\n");
	proceed COMMAND;

    state POLL:

	if (thefsm)
		proceed ILLEGAL;

	location.seq = (byte) seconds ();
	write_actuator (POLL, ACTUATOR_RANGE, (address)&(location.seq));
	proceed COMMAND;

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
}
