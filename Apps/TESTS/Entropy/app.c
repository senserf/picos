/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "form.h"
#include "serf.h"

#define	RS_INIT		0
#define	RS_SHOW		1
#define	RS_READ		2
#define	RS_CMND		3
#define	RS_FORMAT	4
#define	RS_SHOWMEM	5

#define	NSAMPLES	4

lword entsamp [NSAMPLES];
word nsamples = 0;
word nrounds = 0;

#define	RS_INIT		0
#define	RS_START	1
#define	RS_OUT		2

thread (root)

  entry (RS_INIT) 

  	phys_dm2200 (0, 32);

  entry (RS_START)

	entsamp [nsamples++] = entropy;

	if (nsamples < NSAMPLES) {
		delay (1024, RS_START);
		release;
	}

	nsamples = 0;

  entry (RS_OUT)

	ser_outf (RS_OUT,
		nsamples > 9 ? "%d: %lx [%d]\r\n" : " %d: %lx [%d]\r\n",
		nsamples, entsamp [nsamples], nrounds);

	if (++nsamples == NSAMPLES) {
		nsamples = 0;
		nrounds++;
		proceed (RS_START);
	}

	proceed (RS_OUT);

endthread
