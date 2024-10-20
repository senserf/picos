/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "sysio.h"
#include "pinopts.h"

#include "ser.h"
#include "serf.h"
#include "form.h"

#define	IBUFLEN	82

const	word	RATE115	= 1152,
		RATE096 = 96;

static void disable () {

	// Disable the module: note that the preinit macros in mach.h are
	// broken as they account for the stupid interconnections of the
	// UART pins that we inherited from RFM
	_BIS (P3DIR, 0x08);
	_BIC (P3OUT, 0x08);
}

static void enable () {

	_BIS (P3OUT, 0x08);
}

fsm minput {

  char c;

  state MI_READ:

	io (MI_READ, UART_B, READ, &c, 1);

  state MI_WRITE:

	io (MI_WRITE, UART_A, WRITE, &c, 1);
	proceed MI_READ;
}

fsm root {

  char *ibuf;
  int n, k, p;

  state RS_INI:

	disable ();
	ibuf = (char*) umalloc (IBUFLEN + 2);
	//ion (UART_A, CONTROL, (char*)(&RATE096), UART_CNTRL_SETRATE);
	//ion (UART_B, CONTROL, (char*)(&RATE096), UART_CNTRL_SETRATE);

	runfsm minput;

  state RS_MEN:

	ser_out (RS_MEN,
		"\r\nGPS Test\r\n"
		"Commands:\r\n"
		"w string    -> write line to module\r\n"
		"r rate      -> set rate for module\r\n"
		"t rate      -> set rate for UART\r\n"
		"e           -> enable\r\n"
		"d           -> disable\r\n"
	);

  state RS_RCM:

	ser_in (RS_RCM, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {

	    case 'w': proceed RS_WRI;
	    case 'r': proceed RS_RAT;
	    case 't': proceed RS_RAU;
	    case 'e': proceed RS_ENA;
	    case 'd': proceed RS_DIS;

	}

  state RS_ERR:

	ser_out (RS_ERR, "Illegal command or parameter\r\n");
	proceed RS_MEN;

  state RS_WRI:

	for (k = 1; ibuf [k] == ' '; k++);
	n = strlen (ibuf + k);

	ibuf [k + n    ] = '\r';
	ibuf [k + n + 1] = '\n';

	n += 2;
	ibuf [k + n    ] = '\0';

  state RS_WRL:

	while (n) {
		p = io (RS_WRL, UART_B, WRITE, ibuf + k, n);
		n -= p;
		k += p;
	}

	proceed RS_RCM;

  state RS_RAT:

	n = 96;
	scan (ibuf + 1, "%d", &n);
	ion (UART_B, CONTROL, (char*)(&n), UART_CNTRL_SETRATE);
	proceed RS_RCM;

  state RS_RAU:

	n = 96;
	scan (ibuf + 1, "%d", &n);
	ion (UART_A, CONTROL, (char*)(&n), UART_CNTRL_SETRATE);
	proceed RS_RCM;

  state RS_ENA:

	enable ();
	proceed RS_RCM;

  state RS_DIS:

	disable ();
	proceed RS_RCM;
}
