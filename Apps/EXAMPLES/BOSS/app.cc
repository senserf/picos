/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "form.h"
#include "plug_boss.h"
#include "phys_uart.h"

#define	MAXPL		86

int SFR, SFD;

// ============================================================================

fsm reliable {

  address pka, pkb;
  word ln;

  state RS_LOOP:

	pka = tcv_wnp (RS_LOOP, SFR, 12);
	tcv_write (pka, "ENTER LINE", 11);
	tcv_endp (pka);

  state RS_INP:

	pka = tcv_rnp (RS_INP, SFR);

	ln = tcv_left (pka) + 6;
	if (ln > MAXPL)
		ln = MAXPL;

  state RS_OUT:

	pkb = tcv_wnp (RS_OUT, SFR, ln);
	tcv_write (pkb, "ECHO: ", 6);
	tcv_write (pkb, (char*)(pka+1), ln - 6);
	tcv_endp (pka);
	tcv_endp (pkb);

	proceed RS_LOOP;
}

// ============================================================================

fsm root {

  address pka, pkb;
  word ln;

  state RS_INIT:

	phys_uart (0, MAXPL, 0);
	tcv_plug (0, &plug_boss);
	SFR = tcv_open (WNONE, 0, 0, 0);	// Reliable
	SFD = tcv_open (WNONE, 0, 0, 1);	// Direct
	if (SFD < 0 || SFR < 0)
		syserror (ENODEVICE, "uart");
	runfsm reliable;

  state RS_LOOP:

	pka = tcv_wnp (RS_LOOP, SFD, 12);
	tcv_write (pka, "Enter line", 11);
	tcv_endp (pka);

  state RS_INP:

	pka = tcv_rnp (RS_INP, SFD);
	ln = tcv_left (pka) + 6;
	if (ln > MAXPL)
		ln = MAXPL;

  state RS_OUT:

	pkb = tcv_wnp (RS_OUT, SFD, ln);
	tcv_write (pkb, "Echo: ", 6);
	tcv_write (pkb, (char*)(pka+1), ln - 6);
	tcv_endp (pka);
	tcv_endp (pkb);

	proceed RS_LOOP;
}
