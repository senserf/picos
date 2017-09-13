/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2017                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "sensors.h"

#ifndef	__SMURPH__
#include "board_pins.h"
#endif

// These constants are expected to be defined in board_pins.h (at least for
// this test) and refer to the specific sensor numbers as assigned to the
// sensor. This way we have the sensor type encoded into the constant for 
// any specific kind of handling.

#ifdef	SENSOR_CMA3000
#include "cma3000.h"
#endif

#ifdef	SENSOR_SCA3100
#include "sca3100.h"
#endif

#ifdef	SENSOR_TMP007
#include "tmp007.h"
#endif

#ifdef	SENSOR_MPU9250
#include "mpu9250.h"
#endif

#ifdef	SENSOR_OBMICROPHONE
#include "obmicrophone.h"
#endif

// No actuator interface yet; should be easy to add when needed; in particular,
// the DW1000 test should be reimplemented here

#include "ser.h"
#include "serf.h"
#include "form.h"

#define	IBUFLEN		82
#define	MAXVALLEN	16
#define	MAX_SENSORS	16

// ============================================================================

typedef struct {

	sint sensor;		// Sensor ID (number), can be negative
	word vlength;		// Value length in words
	const char *name;	// Sensor name
	word del, count;	// Reporting delay + remaining count
	aword epid, vpid;	// Event thread + reporting thread
	word value [0];		// The value itself

} sensval_t;

static sensval_t *sensors [MAX_SENSORS];
static word n_sensors = 0;

static Boolean streq (const char *a, const char *b) {

	while (*a == *b) {
		if (*a == '\0')
			return YES;
		*a++;
		*b++;
	}

	return NO;
}

fsm outval (sensval_t *sen) {
//
// Output sensor value
//
	word cnt;

	state INIT:

		ser_outf (INIT, "=== Sensor %s\r\n", sen->name);
		cnt = 0;

	state NEXT:

		if (cnt == sen->vlength)
			// Done
			finish;

		ser_outf (NEXT, "  [%d] = %x [%u] <%d>\r\n", cnt,
			sen->value [cnt], sen->value [cnt], sen->value [cnt]);
		cnt++;

		sameas NEXT;
}

fsm svalues (sensval_t *sen) {

	state SV_REPORT:

		bzero (sen->value, sen->vlength << 1);
		read_sensor (SV_REPORT, sen->sensor, sen->value);
		call outval (sen, SV_DONE);

	state SV_DONE:

		if (sen->count <= 1) {
			sen->vpid = 0;
			finish;
		}

		sen->count--;

		delay (sen->del, SV_REPORT);
}


#if defined(SENSOR_EVENTS) || defined(__SMURPH__)

fsm sevents (sensval_t *sen) {

  state SE_WAIT:

	wait_sensor (sen->sensor, SE_EVENT);

  state SE_EVENT:

	ser_outf (SE_EVENT, "Event on sensor %s (%d)\r\n", sen->name,
		sen->sensor);

  state SE_READ:

	bzero (sen->value, sen->vlength << 1);
	read_sensor (SE_READ, sen->sensor, sen->value);
	call outval (sen, SE_WAIT);
}

#endif

// ============================================================================

static void add_sensor (sint id, const char *name, word vl) {

	sensval_t *p;

	if (n_sensors == MAX_SENSORS) {
		diag ("TOO MANY SENSORS!");
		reset ();
	}

	p = (sensval_t*) umalloc (sizeof (sensval_t) + vl + vl);
	if (p == NULL) {
		diag ("MEMORY!");
		reset ();
	}

	p->sensor = id;
	p->name = name;
	p->vlength = vl;
	p->del = p->count = 0;
	p->epid = p->vpid = 0;

	bzero (p->value, vl + vl);

	sensors [n_sensors++] = p;
}

void parse (char **c, char **t) {

	char *cc = *c, *tt;

	while (isspace (*cc)) cc++;

	*c = tt = cc;

	if (*cc == '\0') {
		*t = cc;
		return;
	}

	while (!isspace (*tt) && *tt != '\0') tt++;

	if (*tt == '\0') {
		*t = tt;
		return;
	}

	*tt++ = '\0';

	while (isspace (*tt)) tt++;

	*t = tt;
}

sensval_t *find_sensor (const char *str) {

	const char *cp;
	wint sid;
	word sn;

	sid = -999;
	scan (str, "%d", &sid);

	cp = (sid == -999) ? str : NULL;

	for (sn = 0; sn < n_sensors; sn++)
		if (cp != NULL && streq (cp, sensors [sn]->name) ||
		    cp == NULL && sensors [sn]->sensor == sid)
			// found
			break;
	if (sn == n_sensors)
		return NULL;

	return sensors [sn];
}

// ============================================================================
		
fsm root {

	char *ibuf, *curr, *tail;
	word wa;

	state RS_INIT:

		// Create sensor descriptions
#ifdef	SENSOR_TEMP
		add_sensor (SENSOR_TEMP, "temp", 1);
#endif
#ifdef	SENSOR_VOLTAGE
		add_sensor (SENSOR_VOLTAGE, "volt", 1);
#endif
#ifdef	SENSOR_PIN
		add_sensor (SENSOR_PIN, "pin", 1);
#endif
#ifdef	SENSOR_CMA3000
		add_sensor (SENSOR_CMA3000, "cma3000", 2);
#endif
#ifdef	SENSOR_SCA3100
		add_sensor (SENSOR_SCA3100, "sca3100", 3);
#endif
#ifdef	SENSOR_TMP007
		add_sensor (SENSOR_TMP007, "tmp007", 2);
#endif
#ifdef	SENSOR_MPU9250
		add_sensor (SENSOR_MPU9250, "mpu9250", 10);
#endif
#ifdef	SENSOR_OBMICROPHONE
		add_sensor (SENSOR_OBMICROPHONE, "obmicrophone", 4);
#endif
		// ... add more as needed

		if ((ibuf = (char*) umalloc (IBUFLEN)) == NULL) {
			diag ("MEMORY!");
			reset ();
		}

	state RS_BANNER:

		ser_out (RS_BANNER,
			"\r\nSensor Test\r\n"
		// Sensors that need explicit commands to start/stop
#ifdef SENSOR_CMA3000
	"cma3000 [on mo th tm | off]\r\n"
#endif
#ifdef SENSOR_SCA3100
	"sca3100 [on | off]\r\n"
#endif
#ifdef SENSOR_TMP007
	"tmp007 [on mo en | off | sl oh ol lh ll | rr r | wr r v]\r\n"
#endif
#ifdef SENSOR_MPU9250
	"mpu9250 [on op th | off | r[a|c] r | w[a|c] r v]\r\n"
#endif
#ifdef SENSOR_OBMICROPHONE
	"obmic [on rate | off | reset]\r\n"
#endif
		"r sen [times [intv]]\r\n"
#if defined(SENSOR_EVENTS) || defined(__SMURPH__)
		"e sen\r\n"
#endif
		"stop\r\n"
		"list\r\n"
		"reset\r\n"
	);

	state RS_RCMD:

		leds (0, 1);
		ser_in (RS_RCMD, ibuf, IBUFLEN);
		leds (0, 0);
		curr = ibuf;
		parse (&curr, &tail);

		if (*curr == '\0')
			sameas RS_ERROR;

#ifdef SENSOR_CMA3000
		if (streq (curr, "cma3000"))
			sameas RS_CMA3000;
#endif
#ifdef SENSOR_SCA3100
		if (streq (curr, "sca3100"))
			sameas RS_SCA3100;
#endif
#ifdef SENSOR_TMP007
		if (streq (curr, "tmp007"))
			sameas RS_TMP007;
#endif
#ifdef SENSOR_MPU9250
		if (streq (curr, "mpu9250"))
			sameas RS_MPU9250;
#endif
#ifdef SENSOR_OBMICROPHONE
		if (streq (curr, "obmic"))
			sameas RS_OBMIC;
#endif
		if (streq (curr, "r"))
			sameas RS_R;
#if defined(SENSOR_EVENTS) || defined(__SMURPH__)
		if (streq (curr, "e"))
			sameas RS_E;
#endif
		if (streq (curr, "stop"))
			sameas RS_STOP;

		if (streq (curr, "list"))
			sameas RS_LIST;

		if (streq (curr, "reset"))
			reset ();

	state RS_ERROR:

		ser_out (RS_ERROR, "Illegal command or parameter\r\n");
		sameas RS_BANNER;

	state RS_OK:

		ser_out (RS_OK, "OK\r\n");
		sameas RS_RCMD;

	state RS_SHOWWA:

		ser_outf (RS_SHOWWA, "Value = %x [%u, %d]\r\n", wa, wa, wa);
		sameas RS_RCMD;

#ifdef SENSOR_CMA3000

	state RS_CMA3000:

		word a, b, c;

		curr = tail;
		parse (&curr, &tail);

		if (streq (curr, "on")) {
			a = 0; b = 1; c = 3;
			scan (tail, "%u %u %u", &a, &b, &c);
			cma3000_on (a, b, c);
			sameas RS_OK;
		}
		if (streq (curr, "off")) {
			cma3000_off ();
			sameas RS_OK;
		}
		sameas RS_ERROR;
#endif

#ifdef SENSOR_SCA3100

	state RS_SCA3100:

		curr = tail;
		parse (&curr, &tail);

		if (streq (curr, "on")) {
			sca3100_on ();
			sameas RS_OK;
		}
		if (streq (curr, "off")) {
			sca3100_off ();
			sameas RS_OK;
		}
		sameas RS_ERROR;
#endif

#ifdef SENSOR_TMP007

	state RS_TMP007:

		curr = tail;
		parse (&curr, &tail);

		if (streq (curr, "on")) {
			word mo, en;
			mo = 0x1440;
			en = 0;
			scan (tail, "%x %x", &mo, &en);
			tmp007_on (mo, en);
			sameas RS_OK;
		}
		if (streq (curr, "off")) {
			tmp007_off ();
			sameas RS_OK;
		}
		if (streq (curr, "sl")) {
			wint oh, ol, lh, ll;
			oh = ol = lh = ll = 0;
			scan (tail, "%d %d %d %d", &oh, &ol, &lh, &ll);
			tmp007_setlimits (oh, ol, lh, ll);
			sameas RS_OK;
		}
		if (streq (curr, "rr")) {
			word rn;
			rn = 0;
			scan (tail, "%x", &rn);
			wa = tmp007_rreg ((byte)rn);
			sameas RS_SHOWWA;
		}
		if (streq (curr, "wr")) {
			word rn, va;
			rn = va = 0;
			scan (tail, "%x %x", &rn, &va);
			tmp007_wreg ((byte)rn, va);
			sameas RS_OK;
		}
		sameas RS_ERROR;
#endif

#ifdef SENSOR_MPU9250

	state RS_MPU9250:

		curr = tail;
		parse (&curr, &tail);

		if (streq (curr, "on")) {
			word op, th;
			// Default set for LP motion detection:
			//	lpf = 1, odr = 5, A only, ar = 0, gr = 0, md
			// op = 1 00 00 0001 0101 001
			//	1000 0000 1010 1001
			op = 	MPU9250_LP_MOTION_DETECT	+
				MPU9250_SEN_ACCEL		+
				MPU9250_ACCEL_RANGE_2		+
				MPU9250_LPF_188			+
				MPU9250_LPA_4;
			th = 32;
			scan (tail, "%x %u", &op, &th);
			mpu9250_on (op, (byte)th);
			sameas RS_OK;
		}
		if (streq (curr, "off")) {
			mpu9250_off ();
			sameas RS_OK;
		}
		if (streq (curr, "ra")) {
			word r;
			byte v;
			r = 0;
			scan (tail, "%x", &r);
			mpu9250_rregan ((byte)r, &v, 1);
			wa = v;
			sameas RS_SHOWWA;
		}
		if (streq (curr, "rc")) {
			word r;
			byte v;
			r = 0;
			scan (tail, "%x", &r);
			mpu9250_rregcn ((byte)r, &v, 1);
			wa = v;
			sameas RS_SHOWWA;
		}
		if (streq (curr, "wa")) {
			word r, v;
			r = 0; v = 0;
			scan (tail, "%x %x", &r, &v);
			mpu9250_wrega ((byte)r, (byte)v);
			sameas RS_OK;
		}
		if (streq (curr, "wc")) {
			word r, v;
			r = 0; v = 0;
			scan (tail, "%x %x", &r, &v);
			mpu9250_wregc ((byte)r, (byte)v);
			sameas RS_OK;
		}

		sameas RS_ERROR;
#endif

#ifdef SENSOR_OBMICROPHONE

	state RS_OBMIC:

		curr = tail;
		parse (&curr, &tail);

		if (streq (curr, "on")) {
			// The default is 1.2M (is this a tall order?)
			word ra = 12000;
			scan (tail, "%u", &ra);
			obmicrophone_on (ra);
			sameas RS_OK;
		}
		if (streq (curr, "off")) {
			obmicrophone_off ();
			sameas RS_OK;
		}
		if (streq (curr, "reset")) {
			obmicrophone_reset ();
			sameas RS_OK;
		}

	sameas RS_ERROR;
#endif

	state RS_R:

		sensval_t *sen;
		word del, ntm;
byte aa[8];

		curr = tail;
		parse (&curr, &tail);

		if ((sen = find_sensor (curr)) == NULL || sen->vpid != 0)
			sameas RS_ERROR;

		ntm = 0;
		del = 0;

		scan (tail, "%d %d", &ntm, &del);
		if (ntm == 0)
			ntm = 1;

		sen->del = del;
		sen->count = ntm;
		sen->vpid = runfsm svalues (sen);

		sameas RS_OK;

#if defined(SENSOR_EVENTS) || defined(__SMURPH__)

	state RS_E:

		sensval_t *sen;

		curr = tail;
		parse (&curr, &tail);

		if ((sen = find_sensor (curr)) == NULL || sen->epid != 0)
			sameas RS_ERROR;

		sen->epid = runfsm sevents (sen);
		sameas RS_OK;
#endif

	state RS_STOP:

		word i;
		sensval_t *sen;

		curr = tail;
		parse (&curr, &tail);

		if ((sen = find_sensor (curr)) != NULL) {
			// A single sensor
			if (sen->epid) {
				kill (sen->epid);
				sen->epid = 0;
			}
			if (sen->vpid) {
				kill (sen->vpid);
				sen->vpid = 0;
			}
			sameas RS_OK;
		}

		// All sensors

		for (i = 0; i < n_sensors; i++) {
			sen = sensors [i];
			if (sen->epid) {
				kill (sen->epid);
				sen->epid = 0;
			}
			if (sen->vpid) {
				kill (sen->vpid);
				sen->vpid = 0;
			}
		}
		sameas RS_OK;

	state RS_LIST:

		wa = 0;

	state RS_LIST_N:

		sensval_t *sen;

		if (wa == n_sensors)
			sameas RS_OK;

		sen = sensors [wa];

		ser_outf (RS_LIST_N, "Index=%d, vl=%d, nm=%s, de=%d, co=%d, "
		    "ep=%c, vp=%c\r\n",
			sen->sensor, sen->vlength, sen->name, sen->del,
				sen->count, sen->epid ? 'y' : 'n',
					sen->vpid ? 'y' : 'n');

		wa++;
		sameas RS_LIST_N;
}
