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
#define	max6675_on() CNOP
#define	max6675_off() CNOP
#endif

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"

#define	IBUFLEN		82
#define	THERMOCOUPLE	0
#define	OVEN		0

typedef	struct {

	word Value, Reading;

} calentry_t;

const calentry_t Cal [] = {
	{        0,      21     }, // Added (PG) as a crude lower bound
	{	24,	111	},
	{	25,	113	},
	{	26,	120	},
	{	29,	128	},
	{	34,	151	},
	{	38,	168	},
	{	42,	185	},
	{	48,	212	},
	{	52,	227	},
	{	56,	247	},
	{	60,	269	},
	{	65,	284	},
	{	73,	318	},
	{	78,	340	},
	{	82,	355	},
	{	86,	373	},
	{	91,	393	},
	{	94,	406	},
	{	97,	422	},
	{	100,	432	},
	{	103,	445	},
	{	106,	462	},
	{	109,	472	},
	{	113,	489	},
	{	120,	517	},
	{	124,	524	},
	{	126,	542	},
	{	129,	550	},
	{	131,	559	},
	{	133,	568	},
	{	135,	576	},
	{	137,	583	},
	{	139,	590	},
	{	143,	602	},
	{	145,	615	},
	{	146,	620	},
	{	149,	627	},
	{	152,	640	},
	{	155,	648	},
	{	158,	660	},
	{	161,	688	},
	{	162,	673	},
	{	165,	681	},
	{	167,	685	},
	{	170,	696	},
	{	172,	701	},
	{	175,	715	},
	{	177,	722	},
	{	179,	731	},
	{	180,	735	},
	{	182,	743	},
	{	186,	757	},
	{	189,	765	},
	{	190,	773	},
	{	192,	782	},
	{	194,	786	},
	{	258,   1023	}, // Added (PG) as a crude upper bound
};

#define	CalLength	(sizeof (Cal) / sizeof (calentry_t))

// ============================================================================

word temp (word r) {
//
// Converts the sensor reading to temperature in deg C * 10
//
	sint i;
	word v, v0, v1, r0, r1;

	for (i = 0; i < CalLength; i++)
		if (Cal [i] . Reading > r)
			break;
	if (i == 0)
		return Cal [0].Value * 10;

	if (i == CalLength)
		return Cal [CalLength - 1].Value * 10;

	r0 = Cal [i - 1] . Reading;
	r1 = Cal [i    ] . Reading;

	v0 = Cal [i - 1] . Value * 100;
	v1 = Cal [i    ] . Value * 100;

	v = v0 + (word)(((lword)(v1 - v0) * (r - r0)) / (r1 - r0));

	return (v + 5) / 10;
}

// ============================================================================

fsm scanner {

  lword last_second, first_second;
  word val, tmp;

  state INIT:

	// Synchronize to the nearest second boundary
	last_second = seconds ();

  state SYNCHRONIZE:

	if (last_second == seconds ()) {
		delay (1, SYNCHRONIZE);
		release;
	}

	first_second = last_second = seconds ();

  state MEASURE:

	read_sensor (MEASURE, THERMOCOUPLE, &val);
	tmp = temp (val);

  state OUTTEMP:

	ser_outf (OUTTEMP, "%d, %d.%d, %d\r\n",
		(word)(seconds () - first_second),
		tmp / 10, tmp % 10, val);

  state ADVANCE:

	if (last_second == seconds ()) {
		delay (1, ADVANCE);
		release;
	}

	last_second = seconds ();
	sameas MEASURE;
}

static char ibuf [IBUFLEN];

fsm root {

  word val, tmp;

  state RS_BANNER:

	ser_out (RS_BANNER,
		"\r\nCommands:\r\n"
		"on [v]    -> oven on\r\n"
		"off       -> oven off\r\n"
		"son       -> thermocouple sensor on\r\n"
		"soff      -> thermocouple sensor off\r\n"
		"temp      -> read thermocouple sensor\r\n"
		"run       -> start measurement, stop on any input\r\n"
	);

  state RS_RCMD:

	ser_out (RS_RCMD, "Ready:\r\n");

  state RS_READ:

	ser_in (RS_READ, ibuf, IBUFLEN);

	// In case scan was running
	killall (scanner);

	switch (ibuf [0]) {

		case 'o' : proceed RS_OVEN;
		case 's' : proceed RS_SENSOR;
		case 't' : proceed RS_TEMP;
		case 'r' : proceed RS_RUN;
	}

  state RS_ERR:

	ser_out (RS_ERR, "Illegal command\r\n");
	proceed RS_BANNER;

  state RS_OVEN:

	if (ibuf [1] == 'n') {
		val = 1024;
		scan (ibuf + 2, "%u", &val);
		if (val > 1024)
			val = 1024;
	} else if (ibuf [1] == 'f') {
		val = 0;
	} else
		proceed RS_ERR;

	write_actuator (WNONE, OVEN, &val);
	proceed RS_RCMD;

  state RS_SENSOR:

	if (ibuf [2] == 'n')
		max6675_on ();
	else if (ibuf [2] == 'f')
		max6675_off ();
	else
		proceed RS_ERR;

	proceed RS_RCMD;

  state RS_TEMP:

	read_sensor (RS_TEMP, THERMOCOUPLE, &val);
	tmp = temp (val);

  state RS_OUTVAL:

	ser_outf (RS_OUTVAL, "Temp = %d.%d, %d <%x>\r\n", 
		tmp / 10, tmp % 10, val, val);
	proceed RS_RCMD;

  state RS_RUN:

	runfsm scanner;
	proceed RS_RCMD;
}
