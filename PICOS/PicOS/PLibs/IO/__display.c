/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"

/* =================== */
/* LCD display process */
/* =================== */

#define	DS_INIT		0
#define	DS_EASY		1
#define	DS_SCROLL	2
#define	DS_ADVANCE	3
#define	DS_WRITE	4

/* ================================ */
/* Interface to the display process */
/* ================================ */
address __display_pmem = NULL;

#define	cnt	(*((int*)(__display_pmem + 0)))
#define	len	(*((int*)(__display_pmem + 1)))
#define	pos	(*((int*)(__display_pmem + 2)))
#define	ptr	(*((int*)(__display_pmem + 3)))
#define	shft	(*((int*)(__display_pmem + 4)))
#define	mess	( (char*)(__display_pmem + 5))

thread (__display)
/* =================================== */
/* Writes a message to the LCD display */
/* =================================== */

	char c;

  entry (DS_INIT)

	io (DS_INIT, LCD, CONTROL, (char*) &cnt, LCD_CNTRL_ERASE);

	len = strlen (mess);
	/* Reset the display */
	if (len > 32) {
		/* Have to scroll */
		shft = 0;
		proceed (DS_SCROLL);
	}
	/* Do it the easy way */
	ptr = len;
	pos = 0;

  entry (DS_EASY)

	cnt = io (DS_EASY, LCD, WRITE, mess + pos, ptr);
	if (cnt != ptr) {
		ptr -= cnt;
		pos += cnt;
		proceed (DS_EASY);
	}
	/* We can disappear from the scene */
	ufree (__display_pmem);
	__display_pmem = NULL;
	finish;

  entry (DS_SCROLL)

	/* Move to the front */
	c = 0;
	io (DS_SCROLL, LCD, CONTROL, &c, LCD_CNTRL_POS);
	delay (JIFFIES/2, DS_ADVANCE);
	release;

  entry (DS_ADVANCE)

	ptr = 16;
	pos = shft;

  entry (DS_WRITE)

	cnt = io (DS_WRITE, LCD, WRITE, mess + pos, ptr);
	if (cnt == ptr) {
		/* We are done with this turn */
		if (shft == len - 16) {
			shft = 0;
		} else {
			shft ++;
		}
		proceed (DS_SCROLL);
	}
	ptr -= cnt;
	pos += cnt;
	proceed (DS_WRITE);

endthread
