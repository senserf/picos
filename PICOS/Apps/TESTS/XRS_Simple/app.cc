/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "sysio.h"
#include "form.h"
#include "plug_null.h"
#include "phys_uart.h"
#include "ab.h"

#define	MAXPL		86

fsm root {

  char *ibuf = NULL;
  word w;
  int SFD;

  state RS_INIT:

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

  state RS_LOOP:

	ab_outf (RS_LOOP, "Enter line");

  state RS_INP:

	ibuf = ab_in (RS_INP);

  state RS_OUT:

	ab_outf (RS_OUT, "You have entered: %s", ibuf);

	ufree (ibuf);
	proceed RS_LOOP;
}
