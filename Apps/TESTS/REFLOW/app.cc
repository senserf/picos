/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "sensors.h"

#ifndef	__SMURPH__
#include "board_pins.h"
#else
#define	owen_on() CNOP
#define	owen_off() CNOP
#define	max6675_on() CNOP
#define	max6675_off() CNOP
#endif

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"

#define	IBUFLEN		82
#define	THERMOCOUPLE	0

static char ibuf [IBUFLEN];

fsm root {

  word val;

  state RS_BANNER:

	ser_out (RS_BANNER,
		"\r\nCommands:\r\n"
		"on    -> owen on\r\n"
		"off   -> owen off\r\n"
		"son   -> thermocouple sensor on\r\n"
		"soff  -> thermocouple sensor off\r\n"
		"temp  -> read thermocouple sensor\r\n"
	);

  state RS_RCMD:

	ser_out (RS_RCMD, "Ready:\r\n");

  state RS_READ:

	ser_in (RS_READ, ibuf, IBUFLEN);

	switch (ibuf [0]) {

		case 'o' : proceed RS_OWEN;
		case 's' : proceed RS_SENSOR;
		case 't' : proceed RS_TEMP;
	}

  state RS_ERR:

	ser_out (RS_ERR, "Illegal command\r\n");
	proceed RS_BANNER;

  state RS_OWEN:

	if (ibuf [1] == 'n')
		owen_on ();
	else if (ibuf [1] == 'f')
		owen_off ();
	else
		proceed RS_ERR;

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

  state RS_OUTVAL:

	ser_outf (RS_OUTVAL, "Value = %d <%x>\r\n", val, val);
	proceed RS_RCMD;
}
