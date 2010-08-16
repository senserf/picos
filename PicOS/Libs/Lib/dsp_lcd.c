/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

#include "options.sys"

#if !ECOG_SIM

//+++ "__display.c"

extern address __display_pmem;

procname (__display);

#define	cnt	(*((int*)(__display_pmem + 0)))
#define	len	(*((int*)(__display_pmem + 1)))
#define	pos	(*((int*)(__display_pmem + 2)))
#define	ptr	(*((int*)(__display_pmem + 3)))
#define	shft	(*((int*)(__display_pmem + 4)))
#define	mess	( (char*)(__display_pmem + 5))

int dsp_lcd (const char *m, Boolean kl) {

	if (__display_pmem) {
		if (!kl)
			return -1;
		kill (running (__display));
		ufree (__display_pmem);
		__display_pmem = NULL;
	}
	__display_pmem = umalloc (5 * sizeof (int) + strlen (m) + 1);
	if (__display_pmem != NULL) {
		strcpy (mess, m);
		runthread (__display);
	}
	return 0;
}

#else
void sim_lcd (const char *m);

int dsp_lcd (const char *m, Boolean kl) {
	sim_lcd (m);
	return 0;
}
#endif

