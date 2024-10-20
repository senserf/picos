/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"

/* =================== */
/* LCD updater process */
/* =================== */

#define	UPD_INIT	00
#define	UPD_POS		10
#define	UPD_WRITE	20

address __dupdater_pmem = NULL;

#define	changed		(*((lword*)(__dupdater_pmem + 0)))
#define	dupw		(*((lword*)(__dupdater_pmem + 2)))
#define	pos		(*((int*)(__dupdater_pmem + 4)))
#define	len		(*((int*)(__dupdater_pmem + 5)))
#define	updpid		(*((int*)(__dupdater_pmem + 6)))
#define	lastend		(*((int*)(__dupdater_pmem + 7)))
#define	sbuf		( (char*)(__dupdater_pmem + 8))

thread (__dupdater)

  byte k;

  entry (UPD_INIT)

	if (changed == 0) {
		updpid = 0;
		finish;
	}

	while (1) {
		if ((changed & dupw) != 0)
			break;
		if (++pos == 32) {
			pos = 0;
			dupw = 1;
		} else {
			dupw <<= 1;
		}
	}

	/* First different */

  entry (UPD_POS)

	k = (byte) pos;
	io (UPD_POS, LCD, CONTROL, (char*)&k, LCD_CNTRL_POS);

	/* Find the length */
	len = 0;
	do {
		changed &= ~dupw;
		len++;
		if ((dupw <<= 1) == 0) {
			dupw = 1;
			break;
		}
	} while ((changed & dupw) != 0);

  entry (UPD_WRITE)

	do {
		k = io (UPD_WRITE, LCD, WRITE, sbuf + pos, len);
		len -= k;
		pos += k;
	} while (len);

	if (pos == 32)
		pos = 0;

	proceed (UPD_INIT);

endthread
