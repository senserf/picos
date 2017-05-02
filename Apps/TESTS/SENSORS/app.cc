/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2017                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "sensors.h"

#ifndef	__SMURPH__
#include "board_pins.h"
#endif

#ifdef	cma3000_enable
#include "cma3000.h"
#define	CMA3000
#endif

#ifdef	sca3100_data
#include "sca3100.h"
#define	SCA3100
#endif

#ifdef	tmp007_enable
#include "tmp007.h"
#define	TMP007
#endif

#include "ser.h"
#include "serf.h"
#include "form.h"

#define	IBUFLEN		82
#define	MAXVALLEN	16

static word vl = 1;		// Sensor value length
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

  state SE_READ:

	read_sensor (SE_READ, sn, sen);
	join (runfsm outval, SE_WAIT);
	// delay (1024, SE_WAIT);
	// sameas SE_WAIT;
}

#endif
		
fsm root {

	char *ibuf;
	sint v, a, b, c; word x, y, z;

  state RS_INIT:

	ibuf = (char*) umalloc (IBUFLEN);

  state RS_BANNER:

	ser_out (RS_BANNER,
		"\r\nSensor Test\r\n"
		"Commands:\r\n"
#ifdef CMA3000
		"C m t u   -> CMA3000 on, mo th tm\r\n"
		"F         -> CMA3000 off\r\n"
#endif
#ifdef SCA3100
		"C         -> SCA3100 on\r\n"
		"F         -> SCA3100 off\r\n"
#endif
#ifdef TMP007
		"C m e     -> TMP007 on, mo en\r\n"
		"F         -> TMP007 off\r\n"
		"S h l h l -> TMP007 setlimits, oh ol lh ll\r\n"
		"R r       -> TMP007 rreg, rn\r\n"
		"W r v     -> TMP007 wreg, rn v\r\n"
#endif
		"s n      -> set sensor value length (words)\r\n"
		"r s      -> read value of sensor s\r\n"
		"c s d    -> read value of sensor s continually at d int\r\n"

#if defined(SENSOR_EVENTS) || defined(__SMURPH__)
		"v s      -> report events on sensor s\r\n"
#endif

#if defined(ACTUATOR_LIST) || defined(__SMURPH__)
		"a s v    -> set actuator s to v (hex)\r\n"
#endif
		);

  state RS_RCMD:

	ser_in (RS_RCMD, ibuf, IBUFLEN);

	switch (ibuf [0]) {
#if defined(CMA3000) || defined(SCA3100) || defined(TMP007)
		case 'C' : proceed RS_CMO;
		case 'F' : proceed RS_CMF;
#endif
#if defined(TMP007)
		case 'S' : proceed RS_SETL;
		case 'R' : proceed RS_RREG;
		case 'W' : proceed RS_WREG;
#endif
		case 's' : proceed RS_SVAL;
		case 'r' : proceed RS_GSEN;
		case 'c' : proceed RS_CSEN;

#if defined(SENSOR_EVENTS) || defined(__SMURPH__)
		case 'v' : proceed RS_SWAIT;
#endif
#if defined(ACTUATOR_LIST) || defined(__SMURPH__)
		case 'a' : proceed RS_SACT;
#endif
	}

  state RS_ERR:

	ser_out (RS_ERR, "Illegal command or parameter\r\n");
	proceed RS_BANNER;

#if defined(SENSOR_EVENTS) || defined(__SMURPH__)

  state RS_SWAIT:

	v = -1;
	scan (ibuf + 1, __sfsi, &v);
	if (v < 0) {
		killall (sevents);
	} else {
		runfsm sevents (v);
	}
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
	scan (ibuf + 1, __sfsi, &v);
	bzero (sen, sizeof (sen));

  state RS_GSEN_READ:

	read_sensor (RS_GSEN_READ, v, sen);
	join (runfsm outval, RS_RCMD);
	release;

  state RS_CSEN:

	v = 0;
	x = 1024;
	scan (ibuf + 1, __sfsi "%u", &v, &x);
	bzero (sen, sizeof (sen));

  state RS_CSEN_READ:

	read_sensor (RS_CSEN_READ, v, sen);
	join (runfsm outval, RS_CSEN_NEXT);
	release;

  state RS_CSEN_NEXT:

	delay (x, RS_CSEN_READ);
	release;

#if defined(ACTUATOR_LIST) || defined(__SMURPH__)

  state RS_SACT:

	v = 0;
	x = 0;

	scan (ibuf + 1, __sfsi "%x", &v, &x);

  state RS_SACT_SET:

	write_actuator (RS_SACT_SET, v, &x);
	proceed RS_RCMD;
#endif

#if defined(CMA3000) || defined(SCA3100) || defined(TMP007)

  state RS_CMO:

#ifdef CMA3000
	x = 0;
	y = 1;
	z = 3;
	scan (ibuf + 1, "%u %u %u", &x, &y, &z);
	cma3000_on (x, y, z);
#endif

#ifdef	SCA3100
	sca3100_on ();
#endif

#ifdef	TMP007
	x = 0x1440;
	y = 0;
	scan (ibuf + 1, "%x %x", &x, &y);
	tmp007_on (x, y);
#endif
	proceed RS_RCMD;

  state RS_CMF:

#ifdef CMA3000
	cma3000_off ();
#endif
#ifdef	SCA3100
	sca3100_off ();
#endif
#ifdef	TMP007
	tmp007_off ();
#endif
	proceed RS_RCMD;

#endif

#ifdef TMP007

  state RS_SETL:

	v = a = b = c = 0;
	scan (ibuf + 1, __sfsi __sfsi __sfsi __sfsi, &v, &a, &b, &c);
	tmp007_setlimits (v, a, b, c);
	proceed RS_RCMD;

  state RS_RREG:

	x = 0;
	scan (ibuf + 1, "%x", &x);
	y = tmp007_rreg ((byte)x);

  state RS_RREG_W:

	ser_outf (RS_RREG_W, "[%x] = %x\r\n", x, y);
	proceed RS_RCMD;

  state RS_WREG:

	x = y = 0;
	scan (ibuf + 1, "%x %x", &x, &y);
	tmp007_wreg ((byte)x, y);
	proceed RS_RCMD;

#endif

}
