/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"

#include "options.sys"

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
