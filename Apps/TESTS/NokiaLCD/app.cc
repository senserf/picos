/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "lcdg_n6100p.h"

/*
 * There is no TCV, so all dynamic memory goes to umalloc
 */
heapmem {100};

#define	MIN_CON	30
#define	MAX_CON 70

byte con = MIN_CON;

fsm root {

  state RS_INIT:

	lcdg_on (con);
	lcdg_set (0, 0, 129, 129);
	lcdg_setc (COLOR_BLACK, COLOR_WHITE);
	lcdg_clear ();
	lcdg_set (32, 32, 97, 97);
	lcdg_setc (COLOR_RED, COLOR_YELLOW);
	lcdg_clear ();

	if (con == MAX_CON)
		con = MIN_CON;
	else
		con++;

	delay (2048, RS_INIT);
}
