/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */


#include "sysio.h"
#include "motion.h"

// heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"

static word cycle;
static char *ibuf;

#define	IBUFLEN		80

thread (monitor)

  entry (0)

	ser_outf (0, "M: %u\r\n", motion_status ());

	delay (cycle, 0);

endthread

#define	RS_INIT		0
#define	RS_PROMPT	1
#define	RS_RCMD		2
#define	RS_ILL		3
#define	RS_START	4
#define	RS_STOP		5

thread (root)

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFLEN);
	ibuf [0] = 0;

  entry (RS_PROMPT)

	ser_out (RS_PROMPT,
		"\r\nCommands:\r\n"
		"s n -> start reports at n msec intervals\r\n"
		"q   -> stop\r\n"
	);

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, IBUFLEN);

	switch (ibuf [0]) {

		case 's' : proceed (RS_START);
		case 'q' : proceed (RS_STOP);
	}

  entry (RS_ILL)

	ser_out (RS_ILL, "Illegal\r\n");
	proceed (RS_PROMPT);

  entry (RS_START)

	cycle = 1024;
	scan (ibuf + 1, "%u", &cycle);

	if (!running (monitor))
		runthread (monitor);

	proceed (RS_RCMD);

  entry (RS_STOP)

	killall (monitor);
	proceed (RS_RCMD);

endthread
