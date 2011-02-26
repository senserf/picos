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

trueconst word	RATE115	= 1152,
		RATE096 = 96;

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
	//ion (UART_A, CONTROL, (char*)(&RATE096), UART_CNTRL_SETRATE);
	//ion (UART_B, CONTROL, (char*)(&RATE096), UART_CNTRL_SETRATE);
	runfsm minput;

  entry RS_MEN:

	ser_out (RS_MEN,
		"\r\nBlueTooth Test\r\n"
		"Commands:\r\n"
		"w string    -> write line to module\r\n"
		"r rate      -> set rate for module\r\n"
		"t rate      -> set rate for UART\r\n"
		"e [0|1]     -> escape\r\n"
		"s           -> reset module\r\n"
		"a           -> view attention flag\r\n"
	);

  entry RS_RCM:

	ser_in (RS_RCM, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {

	    case 'w': proceed RS_WRI;
	    case 'r': proceed RS_RAT;
	    case 't': proceed RS_RAU;
	    case 'e': proceed RS_ESC;
	    case 's': proceed RS_RES;
	    case 'a': proceed RS_ATT;

	}

  entry RS_ERR:

	ser_out (RS_ERR, "Illegal command or parameter\r\n");
	proceed RS_MEN;

  entry RS_WRI:

	for (k = 1; ibuf [k] == ' '; k++);
	n = strlen (ibuf + k);

	ibuf [k + n    ] = '\r';
	ibuf [k + n + 1] = '\n';

	n += 2;
	ibuf [k + n    ] = '\0';

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

	n = 0;
	scan (ibuf + 1, "%d", &n);
	if (n)
		blue_cmdmode;
	else
		blue_datamode;
	proceed RS_RCM;

  entry RS_RES:

	blue_reset;
	proceed RS_RCM;

  entry RS_ATT:

	ser_outf (RS_ATT, "ATT: %d\r\n", blue_attention != 0);
	proceed RS_RCM;
}
