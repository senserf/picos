/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "sensors.h"

#ifndef	__SMURPH__
#include "board_pins.h"
#endif

#ifdef	cma3000_bring_up
#include "cma3000.h"
#define	CMA3000
#endif

#ifdef	sca3100_bring_up
#include "sca3100.h"
#define	SCA3100
#endif

#if defined(SCA3100) && defined(CMA3000)
#error "Only one of CMA3000, SCA3100 can be present at the same time"
#endif

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"

#define	IBUFLEN		82
#define	MAXVALLEN	16

static word vl;			// Sensor value length
static word sen [MAXVALLEN];	// Sensor value

fsm outval {
//
// Write sensor value
//
	word cnt;

	state INIT:

		if (vl == 1) {
			ser_outf (INIT, "Value: %x [%u] <%d>\r\n",
				sen [0], sen [0], sen [0]);
			finish;
		}

		ser_outf (INIT, "========\r\n");
		cnt = 0;

	state NEXT:

		if (cnt >= vl)
			// Done
			finish;

		ser_outf (NEXT, "Value [%d] = %x [%u] <%d>\r\n", cnt,
					sen [cnt], sen [cnt], sen [cnt]);
		cnt++;
		sameas NEXT;
}

#if defined(SENSOR_EVENTS) || defined(__SMURPH__)

fsm sevents (sint sn) {

  state SE_WAIT:

	wait_sensor (sn, SE_EVENT);

  state SE_EVENT:

	ser_outf (SE_EVENT, "Event on sensor %d\r\n", sn);
	sameas SE_WAIT;
}

#endif
		
fsm root {

	char *ibuf;
	sint v; word x, y, z;

  state RS_INIT:

	ibuf = (char*) umalloc (IBUFLEN);
	vl = 1;

  state RS_BANNER:

	ser_out (RS_BANNER,
		"\r\nSensor Test\r\n"
		"Commands:\r\n"
#ifdef CMA3000
		"C m t u  -> CMA3000 on, mo th tm\r\n"
		"F        -> CMA3000 off\r\n"
#endif
#ifdef SCA3100
		"C        -> SCA3100 on\r\n"
		"F        -> SCA3100 off\r\n"
#endif
		"s n      -> set sensor value length (words)\r\n"
		"r s      -> read value of sensor s\r\n"
		"c s d    -> read value of sensor s continually at d int\r\n"

#if defined(SENSOR_EVENTS) || defined(__SMURPH__)
		"v s      -> report events on sensor s"
#endif

#if 0
		"x        -> turn reference on\r\n"
		"t        -> test on\r\n"
		"e        -> test off\r\n"
#endif
		);

  state RS_RCMD:

	ser_in (RS_RCMD, ibuf, IBUFLEN);

	switch (ibuf [0]) {
#if defined(CMA3000) || defined(SCA3100)
		case 'C' : proceed RS_CMO;
		case 'F' : proceed RS_CMF;
#endif
		case 's' : proceed RS_SVAL;
		case 'r' : proceed RS_GSEN;
		case 'c' : proceed RS_CSEN;

#if defined(SENSOR_EVENTS) || defined(__SMURPH__)
		case 'v' : proceed RS_SWAIT;
#endif

#if 0
		case 'x' : proceed RS_CSET;
		case 't' : proceed RS_CTST;
		case 'e' : proceed RS_CTSE;
#endif
	}

  state RS_ERR:

	ser_out (RS_ERR, "Illegal command or parameter\r\n");
	proceed RS_BANNER;

#if defined(SENSOR_EVENTS) || defined(__SMURPH__)

  state RS_SWAIT:

	v = 0;
	scan (ibuf + 1, "%d", &v);
	runfsm sevents (v);
	proceed RS_RCMD;
#endif

  state RS_SVAL:

	vl = 0;
	scan (ibuf + 1, "%u", &vl);
	if (vl == 0)
		vl = 1;
	else if (vl > 16)
		vl = 16;
	proceed RS_RCMD;

  state RS_GSEN:

	v = 0;
	scan (ibuf + 1, "%d", &v);
	bzero (sen, sizeof (sen));

  state RS_GSEN_READ:

	read_sensor (RS_GSEN_READ, v, sen);
	join (runfsm outval, RS_RCMD);
	release;

  state RS_CSEN:

	v = 0;
	x = 1024;
	scan (ibuf + 1, "%d %u", &v, &x);
	bzero (sen, sizeof (sen));

  state RS_CSEN_READ:

	read_sensor (RS_CSEN_READ, v, sen);
	join (runfsm outval, RS_CSEN_NEXT);
	release;

  state RS_CSEN_NEXT:

	delay (x, RS_CSEN_READ);
	release;

#if 0
  state RS_CSET:

	EREF_ON;
	proceed RS_RCMD;

  state RS_CTST:

	SENSOR_TEST;
	proceed RS_RCMD;

  state RS_CTSE:

	SENSOR_TEST_END;
	proceed RS_RCMD;

#endif

#if defined(CMA3000) || defined(SCA3100)

  state RS_CMO:

#ifdef CMA3000
	x = 0;
	y = 1;
	z = 3;
	scan (ibuf + 1, "%u %u %u", &x, &y, &z);
	cma3000_on (x, y, z);
#else
	sca3100_on ();
#endif
	proceed RS_RCMD;

  state RS_CMF:

#ifdef CMA3000
	cma3000_off ();
#else
	sca3100_off ();
#endif
	proceed RS_RCMD;

#endif

}
