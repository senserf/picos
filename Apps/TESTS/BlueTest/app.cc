/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "pinopts.h"

#include "ser.h"
#include "serf.h"
#include "form.h"

#define	IBUFLEN	82

fsm minput {

  char c;

  entry MI_READ:

	io (MI_READ, UART_B, READ, &c, 1);

  entry MI_WRITE:

	io (MI_WRITE, UART_A, WRITE, &c, 1);

	proceed MI_READ;
}

fsm root {

	char *ibuf;
	int n, k, p;

  entry RS_INI:

	ibuf = (char*) umalloc (IBUFLEN + 2);
	runfsm minput;

  entry RS_MEN:

	ser_out (RS_MEN,
		"\r\nBlueTooth Test\r\n"
		"Commands:\r\n"
		"w string    -> write line to module\r\n"
		"r rate      -> set rate for module\r\n"
		"t rate      -> set rate for UART\r\n"
		"e           -> escape\r\n"
		"a           -> reset\r\n"
		"s           -> view status flag\r\n"
		"p [0|1]     -> power (down|up)\r\n"
	);

  entry RS_RCM:

	ser_in (RS_RCM, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {

	    case 'w': proceed RS_WRI;
	    case 'r': proceed RS_RAT;
	    case 't': proceed RS_RAU;
	    case 'e': proceed RS_ESC;
	    case 'a': proceed RS_RES;
	    case 's': proceed RS_ATT;
	    case 'p': proceed RS_POW;

	}

  entry RS_ERR:

	ser_out (RS_ERR, "Illegal command or parameter\r\n");
	proceed RS_MEN;

  entry RS_WRI:

	for (k = 1; ibuf [k] == ' '; k++);
	n = strlen (ibuf + k);

	ibuf [k + n] = '\r';
	n++;

#ifndef LINKMATIC
	if (blue_status == 0) {
#endif
		// Connected, send NL (otherwise, it messes up commands)
		ibuf [k + n] = '\n';
		n++;
#ifndef LINKMATIC
	}
#endif
	ibuf [k + n] = '\0';

  entry RS_WRL:

	while (n) {
		p = io (RS_WRL, UART_B, WRITE, ibuf + k, n);
		n -= p;
		k += p;
	}

	proceed RS_RCM;

  entry RS_RAT:

	n = 96;
	scan (ibuf + 1, "%d", &n);
	ion (UART_B, CONTROL, (char*)(&n), UART_CNTRL_SETRATE);
	proceed RS_RCM;

  entry RS_RAU:

	n = 96;
	scan (ibuf + 1, "%d", &n);
	ion (UART_A, CONTROL, (char*)(&n), UART_CNTRL_SETRATE);
	proceed RS_RCM;

  entry RS_ESC:

	blue_escape_set;
	mdelay (10);
	blue_escape_clear;
	proceed RS_RCM;

  entry RS_RES:

	blue_reset_set;
	mdelay (10);
	blue_reset_clear;
	proceed RS_RCM;

  entry RS_POW:

	n = 1;
	scan (ibuf + 1, "%d", &n);
	if (n)
		blue_power_up;
	else
		blue_power_down;
	proceed RS_RCM;

  entry RS_ATT:

	ser_outf (RS_ATT, "STATUS: %d\r\n", blue_status != 0);
	proceed RS_RCM;
}
