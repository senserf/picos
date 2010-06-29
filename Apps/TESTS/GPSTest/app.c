/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

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

thread (minput)

#define	MI_READ		0
#define	MI_WRITE	1

  static char c;

  entry (MI_READ)

	io (MI_READ, UART_B, READ, &c, 1);

  entry (MI_WRITE)

	io (MI_WRITE, UART_A, WRITE, &c, 1);
	proceed (MI_READ);

endthread

thread (root)

#define	RS_INI	0
#define	RS_MEN	1
#define	RS_RCM	2
#define	RS_WRI	3
#define	RS_RAT	4
#define	RS_ENA	5
#define	RS_DIS	6
#define	RS_RAU	8

#define	RS_WRL	9
#define	RS_ERR	10

	static char *ibuf;
	static int n, k, p;

  entry (RS_INI)

	disable ();
	ibuf = (char*) umalloc (IBUFLEN + 2);
	//ion (UART_A, CONTROL, (char*)(&RATE096), UART_CNTRL_SETRATE);
	//ion (UART_B, CONTROL, (char*)(&RATE096), UART_CNTRL_SETRATE);

	runthread (minput);

  entry (RS_MEN)

	ser_out (RS_MEN,
		"\r\nGPS Test\r\n"
		"Commands:\r\n"
		"w string    -> write line to module\r\n"
		"r rate      -> set rate for module\r\n"
		"t rate      -> set rate for UART\r\n"
		"e           -> enable\r\n"
		"d           -> disable\r\n"
	);

  entry (RS_RCM)

	ser_in (RS_RCM, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {

	    case 'w': proceed (RS_WRI);
	    case 'r': proceed (RS_RAT);
	    case 't': proceed (RS_RAU);
	    case 'e': proceed (RS_ENA);
	    case 'd': proceed (RS_DIS);

	}

  entry (RS_ERR)

	ser_out (RS_ERR, "Illegal command or parameter\r\n");
	proceed (RS_MEN);

  entry (RS_WRI)

	for (k = 1; ibuf [k] == ' '; k++);
	n = strlen (ibuf + k);

	ibuf [k + n    ] = '\r';
	ibuf [k + n + 1] = '\n';

	n += 2;
	ibuf [k + n    ] = '\0';

  entry (RS_WRL)

	while (n) {
		p = io (RS_WRL, UART_B, WRITE, ibuf + k, n);
		n -= p;
		k += p;
	}

	proceed (RS_RCM);

  entry (RS_RAT)

	n = 96;
	scan (ibuf + 1, "%d", &n);
	ion (UART_B, CONTROL, (char*)(&n), UART_CNTRL_SETRATE);
	proceed (RS_RCM);

  entry (RS_RAU)

	n = 96;
	scan (ibuf + 1, "%d", &n);
	ion (UART_A, CONTROL, (char*)(&n), UART_CNTRL_SETRATE);
	proceed (RS_RCM);

  entry (RS_ENA)

	enable ();
	proceed (RS_RCM);

  entry (RS_DIS)

	disable ();
	proceed (RS_RCM);

endthread
