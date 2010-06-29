/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "form.h"
#include "plug_null.h"
#include "phys_uart.h"
#include "ab.h"

#define	MAXPL		86


#define	RS_INIT		0
#define	RS_LOOP		1
#define	RS_INP		2
#define	RS_OUT		3

thread (root)
/* =========================================== */
/* This is the main program of the application */
/* =========================================== */

  static char *ibuf = NULL;
  static word w;
  static int SFD;

  entry (RS_INIT)

	phys_uart (0, MAXPL, 0);
	tcv_plug (0, &plug_null);
	SFD = tcv_open (WNONE, 0, 0);
	if (SFD < 0)
		syserror (ENODEVICE, "uart");
	w = 0xffff;
	tcv_control (SFD, PHYSOPT_SETSID, &w);
	tcv_control (SFD, PHYSOPT_TXON, NULL);
	tcv_control (SFD, PHYSOPT_RXON, NULL);
	ab_init (SFD);

  entry (RS_LOOP)

	ab_outf (RS_LOOP, "Enter line");

  entry (RS_INP)

	ibuf = ab_in (RS_INP);

  entry (RS_OUT)

	ab_outf (RS_OUT, "You have entered: %s", ibuf);

	ufree (ibuf);
	proceed (RS_LOOP);

endthread
