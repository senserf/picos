/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

/*
 * There is no TCV, so all dynamic memory goes to umalloc
 */
heapmem {100};

#if	LEDS_DRIVER
#include "led.h"
#endif

#include "ser.h"
#include "serf.h"

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_ERR		11
#define	RS_DELC		20	// Calibrates delay loops

#define	IBUFSIZE	128

process (root, void)
/* =========================================== */
/* This is the main program of the application */
/* =========================================== */

	static char *ibuf = NULL;
	word n, m;

  entry (RS_INIT)

	if (ibuf == NULL)
		ibuf = (char*) umalloc (IBUFSIZE);

	ser_out (RS_RCMD-1, "\r\n"
		"Commands:\r\n"
 		"  d n m        : run n times m-msec delay\r\n"
		"  r            : reset)\r\n");

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, IBUFSIZE);

	switch (ibuf [0]) {

		case 'd' : proceed (RS_DELC);
		case 'r' : reset ();

	}

  entry (RS_ERR)

	ser_out (RS_ERR, "Illegal command or parameter\r\n");
	proceed (RS_INIT);

  entry (RS_DELC)

	n = m = 0;
	scan (ibuf + 1, "%u %u", &n, &m);

	if (n == 0 || m == 0)
		proceed (RS_ERR);

	diag ("%u times %u msec delay starting %u", n, m, (word) seconds ());

	while (n--)
		mdelay (m);

	proceed (RS_DELC+1);

  entry (RS_DELC+1)

	ser_outf (RS_DELC+1, "Done at %u\r\n", (word) seconds ());
	proceed (RS_RCMD);

endprocess (1)
