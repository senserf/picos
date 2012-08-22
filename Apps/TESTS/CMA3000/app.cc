/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// This is an extract from CHRONOS/CMATEST for a UART-equipped board to test
// CMA3000, e.g., as interfaced to CC430W

#include "sysio.h"
#include "sensors.h"
#include "ser.h"
#include "serf.h"

#define	IBUFLEN		82

static word atimeout = 0,               // Accel event timeout, every that many
                                        // msec read data even if no event

            areadouts = 1,              // After event read this many times ...
            ainterval = 0;              // ... at this interval before

static Boolean aon = NO;

static char ibuf [IBUFLEN];

fsm accel_thread {

  char c [4];
  word nc;

  state AT_WAIT:

	if (atimeout)
		delay (atimeout, AT_TIMEOUT);

	if (areadouts)
		wait_sensor (SENSOR_MOTION, AT_EVENT);

	release;

  state AT_TIMEOUT:

	// One-time read
	read_sensor (AT_TIMEOUT, SENSOR_MOTION, (address) c);

  state AT_SEND_PERIODIC:

	ser_outf (AT_SEND_PERIODIC, "P: [%x] <%d,%d,%d>\r\n",
		c [0], c [1], c [2], c [3]);
Revert:
	if (areadouts == 0)
		// No wait: revert to event mode
		wait_sensor (SENSOR_MOTION, WNONE);

	proceed AT_WAIT;

  state AT_EVENT:

	nc = areadouts;

  state AT_READOUT:

	read_sensor (AT_READOUT, SENSOR_MOTION, (address) c);

  state AT_SEND_EVENT:

	ser_outf (AT_SEND_EVENT, "E: [%x] <%d,%d,%d>\r\n",
		c [0], c [1], c [2], c [3]);

	if (nc <= 1)
		// Done
		proceed AT_WAIT;

	nc--;
	delay (ainterval, AT_READOUT);
	release;
}
	
// ============================================================================

fsm root {

  word w0, w1, w2;

  state RS_INIT:

	cma3000_off ();
	powerdown ();

  state RS_MENU:

	ser_out (RS_MENU,
		"\r\nCMA3000 test\r\n"
		"Commands:\r\n"
		"a mo th tm - start acc reports\r\n"
		"q          - stop\r\n"
		"s at ar ai - set invls for acc reps\r\n"
		"p 0|1      - power\r\n"
	);

  state RS_LOOP:

	if (ser_in (RS_LOOP, ibuf, IBUFLEN) <= 0)
		proceed RS_MENU;

	switch (ibuf [0]) {
		case 'a' : proceed RS_ASON;	// Start acceleration reports
		case 'q' : proceed RS_ASOFF;	// Stop acceleration reports
		case 's' : proceed RS_SETDEL;	// Set delays for acc reports
		case 'p' : proceed RS_POW;
	}

  state RS_BAD:

	ser_out (RS_BAD, "Illegal command\r\n");
	proceed RS_LOOP;

  state RS_POW:

	w0 = 0;
	scan (ibuf + 1, "%u", &w0);

	if (w0)
		powerup ();
	else
		powerdown ();

	proceed RS_DONE;

  state RS_ASON:

	if (aon)
		proceed RS_BAD;

	w0 = 0;
	w1 = 0;
	w2 = 3;

	scan (ibuf + 1, "%u %u %u", &w0, &w1, &w2);
	cma3000_on (w0, w1, w2);
	aon = YES;

Restart:
	killall (accel_thread);
	runfsm accel_thread;

	proceed RS_DONE;

  state RS_ASOFF:

	if (!aon)
		proceed RS_BAD;

	killall (accel_thread);
	cma3000_off ();
	aon = NO;

	proceed RS_DONE;

  state RS_SETDEL:

	w0 = w1 = w2 = 0;
	scan (ibuf + 1, "%u %u %u", &w0, &w1, &w2);
	if (w0 > 60)
		w0 = 60;

	if (w0 == 0 && w1 == 0)
		proceed RS_BAD;

	atimeout = w0 * 1024;
	areadouts = w1;
	ainterval = w2;

	if (aon)
		goto Restart;

	proceed RS_DONE;

  state RS_DONE:

	ser_out (RS_DONE, "OK\r\n");
	proceed RS_LOOP;
}
