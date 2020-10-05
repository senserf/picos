/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "sysio.h"
#include "sensors.h"

#ifndef	__SMURPH__
#include "board_pins.h"
#else
#define	sca3100_on() CNOP
#endif

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"

#define	IBUFLEN		82
#define	VBUFSIZE	1000
#define	SENSOR_SCA3100	0

typedef struct { sint xx, yy, zz; } readout_t;

readout_t readouts [VBUFSIZE];

word NR;
word Del;

char ibuf [IBUFLEN];

static void collect (word n) {

	readout_t *p;

	if (n > VBUFSIZE)
		n = VBUFSIZE;

	NR = n;

	for (p = readouts; n; n--, p++) {
		if (Del)
			mdelay (Del);
		read_sensor (WNONE, SENSOR_SCA3100, (address) p);
	}
}

fsm root {

	sint v [3];
	word n;

  state RS_INIT:

	sca3100_on ();

  state RS_BANNER:

	ser_out (RS_BANNER,
		"\r\nCommands:\r\n"
		"r     -> read one sample\r\n"
		"c n d -> collect n samples (<= 1000), d = del ms (<= 1000)\r\n"
		"d     -> dump collected samples\r\n"
		);

  state RS_RCMD:

	ser_out (RS_RCMD, "Ready:\r\n");

  state RS_READ:

	ser_in (RS_READ, ibuf, IBUFLEN);

	switch (ibuf [0]) {

		case 'r' : proceed RS_READONCE;
		case 'c' : proceed RS_COLLECT;
		case 'd' : proceed RS_DUMP;
	}

  state RS_ERR:

	ser_out (RS_ERR, "Illegal command or parameter\r\n");
	proceed RS_BANNER;

  state RS_READONCE:

	read_sensor (WNONE, SENSOR_SCA3100, (address) v);

  state RS_REPORTONCE:

	ser_outf (RS_REPORTONCE, "X=%d, Y=%d, Z=%d\r\n", v [0], v [1], v [2]);
	proceed RS_RCMD;

  state RS_COLLECT:

	NR = 0;
	n = 1000;
	Del = 0;
	scan (ibuf + 1, "%u %u", &n, &Del);
	if (n > 1000 || Del > 1000)
		proceed RS_ERR;

	collect (n);
	proceed RS_RCMD;

  state RS_DUMP:

	ser_out (RS_DUMP, "Number, X, Y, Z\r\n");
	n = 0;

  state RS_NEXTITEM:

	if (n == NR)
		proceed RS_RCMD;

	ser_outf (RS_NEXTITEM, "%u, %d, %d, %d\r\n", n,
		readouts [n] . xx,
		readouts [n] . yy,
		readouts [n] . zz);

	n++;
	sameas RS_NEXTITEM;
}
