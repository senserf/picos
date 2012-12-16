/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "form.h"
#include "plug_boss.h"
#include "phys_uart.h"

#define	MAXCM		80
#define	MAXPL		(MAXCM+6)

#define	cnt(p)		((char*)((p) + 1))

sint SFD;

word reldel, unrdel;
word relcnt, unrcnt;

// ============================================================================

fsm relpcs {

  address pka;

  state RS_LOOP:

	pka = tcv_wnp (RS_LOOP, SFD, 30);
	form (cnt (pka), "RELIABLE MESSAGE NUMBER %u", relcnt);
	relcnt++;
	tcv_endp (pka);

	delay (reldel, RS_LOOP);
}

fsm unrpcs {

  address pka;

  state RS_LOOP:

	pka = tcv_wnpu (RS_LOOP, SFD, 32);
	form (cnt (pka), "URELIABLE MESSAGE NUMBER %u", unrcnt);
	unrcnt++;
	tcv_endp (pka);

	delay (unrdel, RS_LOOP);
}

// ============================================================================

static Boolean reliable (address p) {
//
// Returns YES is the (received) packet p is reliable
//
	return ((byte*)(p)) [0] == 0xAD;
}

static char	cmd [MAXCM];
Boolean 	ptp;

static sint do_command () {

	word c;
	char k;

	c = 0;
	k = 0;

	if (!scan (cmd, "%c %u", &k, &c))
		return 0;

	if (c <= 256)
		c = 256;

	if (k == 'r') {
		reldel = c;
		relcnt = 0;
		killall (relpcs);
		runfsm relpcs;
		return 1;
	}

	if (k == 'u') {
		unrdel = c;
		unrcnt = 0;
		killall (unrpcs);
		runfsm unrpcs;
		return 1;
	}

	if (k == 's') {
		killall (relpcs);
		killall (unrpcs);
		return 1;
	}

	if (k == 'q')
		reset ();

	return 0;
}

fsm root {

  address pka;
  word pkl;

  state RS_INIT:

	phys_uart (0, MAXPL, 0);
	tcv_plug (0, &plug_boss);
	if ((SFD = tcv_open (WNONE, 0, 0)) < 0)
		syserror (ENODEVICE, "uart");

  state RS_LOOP:

	pka = tcv_wnp (RS_LOOP, SFD, 10);
	// Prompt written reliably
	tcv_write (pka, "Command?", 9);
	tcv_endp (pka);

  state RS_INP:

	pka = tcv_rnp (RS_INP, SFD);
	pkl = tcv_left (pka);

	if (pkl >= MAXCM)
		pkl = MAXCM-1;

	memcpy (cmd, cnt (pka), pkl);
	cmd [pkl] = '\0';
	ptp = reliable (pka);

	tcv_endp (pka);

	if (do_command ())
		proceed RS_ACMD;

	// This isn't a command
	pkl = strlen (cmd);
	if (pkl > MAXCM - 10)
		pkl = MAXCM - 10;

  state RS_ECHO:

	sint n;

	pka = tcv_wnp (RS_ECHO, SFD, pkl + 10);
	tcv_write (pka, "ECHO<", 5);
	tcv_write (pka, ptp ? "r" : "u", 1);
	tcv_write (pka, ">: ", 3);
	tcv_write (pka, cmd, pkl);
	tcv_write (pka, "\0", 1);
	tcv_endp (pka);

	proceed RS_LOOP;

  state RS_ACMD:

	pka = tcv_wnp (RS_ACMD, SFD, 4);
	tcv_write (pka, "OK", 3);
	tcv_endp (pka);
	proceed RS_LOOP;
}
