/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "sysio.h"
#include "ez430_lcd.h"
#include "rtc_cc430.h"
#include "tcvphys.h"
#include "phys_cc1100.h"
#include "cc1100.h"
#include "plug_null.h"
#include "form.h"
#include "sensors.h"
#include "storage.h"
#include "ab.h"

#ifdef BUTTONS
#include "buttons.h"

static byte Buttons;

#endif

#define MAXPLEN			CC1100_MAXPLEN

// ============================================================================

#ifdef	BOARD_CHRONOS_WHITE
// Differentiating sensors in version 1.1 of CHRONOS
#define	ACCEL_BMA250
#define	PRESS_BMP085
#else
// Assume version 1.0
#define	ACCEL_CMA3000
#define	PRESS_SCP1000
#endif

// ============================================================================

static word 	imess,	// Count received
		omess;	// ... and outgoing messages

// ============================================================================

void msg_lcd (const char *txt, word fr, word to) {
//
// Displays characters on the LCD
//
	char c;

	while (1) {
		if ((c = *txt) != '\0') {
			if (c >= 'a' && c <= 'z')
				c = c - 'a' + 10;
			else if (c >= 'A' && c <= 'Z')
				c = c - 'A' + 10;
			else if (c >= '0' && c <= '9')
				c -= '0';
			else
				c = 32;
			ezlcd_item (fr, (word)c | LCD_MODE_SET);
			txt++;
		} else {
			ezlcd_item (fr, LCD_MODE_CLEAR);
		}
		if (fr == to)
			return;
		if (fr > to)
			fr--;
		else
			fr++;
	}
}

void msg_hi (const char *txt) { msg_lcd (txt, LCD_SEG_L1_3, LCD_SEG_L1_0); }
void msg_lo (const char *txt) { msg_lcd (txt, LCD_SEG_L2_4, LCD_SEG_L2_0); }

void msg_nn (word hi, word a) {

	char erm [6];

	if (hi) {
		form (erm, "%u", a % 10000);
		msg_hi (erm);
	} else {
		form (erm, "%u", a);
		msg_lo (erm);
	}
}

void msg_xx (word hi, word a) {

	char erm [6];

	form (erm, "%x", a);

	if (hi) {
		msg_hi (erm);
	} else {
		msg_lo (erm);
	}
}

#ifdef BUTTONS
static void buttons (word but) {

	Buttons |= (1 << but);
	trigger (&Buttons);

}
#endif

// ============================================================================
// Array to return sensor values. For the acceleration sensor:
//
//	aval = event + 2 acceleration coordinates
//
// For the pressure/temperature combo:
//
//	pval [0-1] = lword pressure in Pascals * 4, i.e., divide this by 4 to
//                   get Pascals
//	pval [3]   = temperature in Celsius * 20, i.e., divide this by 20 to
//                   get temperature in Celsius
//
// ============================================================================

word sval_rint;		// Reporting interval

fsm sensor_server {

  word bval, pval [3];
  char aval [4];

  state AS_LOOP:

	if (sval_rint == 0) {
		// Stop
		cma3000_off ();
		when (&sval_rint, AS_LOOP);
		release;
	}

  state AS_RBAT:

	read_sensor (AS_RBAT, SENSOR_BATTERY, &bval);

  state AS_RPRE:

	read_sensor (AS_RPRE, SENSOR_PRESSTEMP, pval);

  state AS_RACC:

	read_sensor (AS_RACC, SENSOR_MOTION, (address)(&aval));

  state AS_SEND:

	ab_outf (AS_SEND, "MO: %d,%d,%d,%d, PR: %lu, TM: %u, BA: %u",
		aval [0],
		aval [1],
		aval [2],
		aval [3],
			((lword*)pval) [0], pval [2], bval);

	msg_nn (1, omess++);

	delay (sval_rint, AS_LOOP);
}

// ============================================================================

#ifdef BUTTONS

// Buttons serviced by a special (debouncing) driver

fsm button_server {

  state BS_LOOP:

        char butts [6];
        word i;

	if (Buttons == 0) {
		when (&Buttons, BS_LOOP);
		release;
	}

	for (i = 0; i < 5; i++)
		butts [i] = (Buttons & (1 << i)) ? 'X' : '-';

	butts [5] = '\0';

	ab_outf (BS_LOOP, "Buttons: %s", butts);
	Buttons = 0;
	msg_nn (1, omess++);
	proceed BS_LOOP;
}

#else

// Buttons implemented as a sensor

fsm button_server {

  char butts [6];

  state BS_LOOP:

	wait_sensor (SENSOR_BUTTONS, BS_PRESSED);

  state BS_PRESSED:

	word bv, i;

	read_sensor (BS_PRESSED, SENSOR_BUTTONS, &bv);

	for (i = 0; i < 5; i++)
		butts [i] = (bv & (1 << i)) ? 'X' : '-';

	butts [5] = '\0';

  state BS_OUT:

	ab_outf (BS_OUT, "Buttons: %s", butts);
	msg_nn (1, omess++);
	proceed BS_LOOP;
}

#endif /* BUTTONS */

// ============================================================================

fsm buzz (word duration) {

	state BUZZ_OFF:

		buzzer_off ();
		finish;

	initial state BUZZ_ON:

		buzzer_on ();
		delay (duration, BUZZ_OFF);
}

// ============================================================================

fsm root {

  int sfd = -1;
  word w0, w1, w2, w3;
  int iarg0;
  char *ibuf = NULL;

  state RS_INIT:

	ezlcd_init ();
	ezlcd_on ();

	msg_hi ("OLSO");
	msg_lo ("PICOS");

	// Initialize things for radio
	phys_cc1100 (0, MAXPLEN);

	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);

	w0 = 0xffff;
	tcv_control (sfd, PHYSOPT_SETSID, &w0);
	tcv_control (sfd, PHYSOPT_TXON, NULL);
	tcv_control (sfd, PHYSOPT_RXON, NULL);

	ab_init (sfd);
	ab_mode (AB_MODE_PASSIVE);
#ifdef BUTTONS
	buttons_action (buttons);
#endif
	runfsm sensor_server;
	runfsm button_server;

  state RS_LOOP:

	if (ibuf != NULL) {
		ufree (ibuf);
		ibuf = NULL;
	}

  state RS_READ:

	ibuf = ab_in (RS_READ);

	msg_nn (0, imess++);

	switch (ibuf [0]) {

		case 's' : proceed RS_ASON;	// Start acceleration reports
		case 'q' : proceed RS_ASOFF;	// Stop acceleration reports
		case 'd' : proceed RS_PDOWN;	// Power down mode
		case 'D' : proceed RS_DDOWN;	// Dumb power down
		case 'r' : proceed RS_RTC;	// Set or get RTC
		case 'b' : proceed RS_BUZZ;

		case 'f' : {			// INFO FLASH operations
			switch (ibuf [1]) {
				case 'r' : proceed IF_READ;
				case 'w' : proceed IF_WRITE;
				case 'e' : proceed IF_ERASE;
			}
		}
	}

  state RS_BAD:

	ab_outf (RS_BAD, "Illegal command");
	proceed RS_LOOP;

  // ==========================================================================

  state RS_DDOWN:

	w0 = 0;
	scan (ibuf + 1, "%u", &w0);
	if (w0 == 0)
		proceed RS_BAD;

	ezlcd_off ();
	powerdown ();
	tcv_control (sfd, PHYSOPT_RXOFF, NULL);
	tcv_control (sfd, PHYSOPT_TXOFF, NULL);

  state RS_DLOOP:

	if (w0) {
		w0--;
		delay (1024, RS_DLOOP);
		release;
	}

	powerup ();
	ezlcd_on ();
	tcv_control (sfd, PHYSOPT_RXON, NULL);
	tcv_control (sfd, PHYSOPT_TXON, NULL);
	proceed RS_DONE;

  state RS_PDOWN:

	w1 = w0 = 0;
	scan (ibuf + 1, "%u %u", &w0, &w1);

	if (w0 == 0 || w1 == 0)
		proceed RS_BAD;

  state RS_HOLD:

	powerdown ();
	tcv_control (sfd, PHYSOPT_RXOFF, NULL);
	tcv_control (sfd, PHYSOPT_TXOFF, NULL);
	delay (w0, RS_ON);
	release;

  state RS_ON:

	powerup ();
	tcv_control (sfd, PHYSOPT_RXON, NULL);
	tcv_control (sfd, PHYSOPT_TXON, NULL);

  state RS_TICK:

	ab_outf (RS_TICK, "Up for a sec");
	if (w1 <= 1) {
		// Exit
		powerup ();
		proceed RS_DONE;
	}
	w1--;
	delay (1000, RS_HOLD);
	release;

  // ==========================================================================

  state RS_ASON:

	// Mode + interval
	w0 = 0;
	scan (ibuf + 1, "%u", &w0);
	sval_rint = (w0 == 0) ? 1024 : w0;
	cma3000_on (0, 1, 3);
STrig:
	trigger (&sval_rint);

  state RS_DONE:

	ab_outf (RS_DONE, "OK");
	proceed RS_LOOP;

  state RS_FAIL:

	ab_outf (RS_FAIL, "Failed");
	proceed RS_LOOP;

  state RS_ASOFF:

	sval_rint = 0;
	goto STrig;

  state RS_RTC:

  	rtc_time_t r;

	if (scan (ibuf + 1, "%u %u %u %u %u %u %u",
		// Note: this won't overflow
		((address)ibuf)+0,
		((address)ibuf)+1,
		((address)ibuf)+2,
		((address)ibuf)+3,
		((address)ibuf)+4,
		((address)ibuf)+5,
		((address)ibuf)+6) != 7)
			proceed RS_SHOW;
	// Set
	r.year   = (byte) ((address)ibuf) [0];
	r.month  = (byte) ((address)ibuf) [1];
	r.day    = (byte) ((address)ibuf) [2];
	r.dow    = (byte) ((address)ibuf) [3];
	r.hour   = (byte) ((address)ibuf) [4];
	r.minute = (byte) ((address)ibuf) [5];
	r.second = (byte) ((address)ibuf) [6];

	rtc_set (&r);
	proceed RS_DONE;

  state RS_SHOW:

  	rtc_time_t r;

	rtc_get (&r);
	ab_outf (RS_SHOW, "TIME: %u %u %u %u %u %u %u",
		r.year,
		r.month,
		r.day,
		r.dow,
		r.hour,
		r.minute,
		r.second);

	proceed RS_LOOP;

  state RS_BUZZ:

	w0 = 0;
	scan (ibuf + 1, "%u", &w0);
	if (w0 == 0)
		proceed RS_BAD;

	if (!running (buzz)) {
		runfsm buzz (w0);
		proceed RS_DONE;
	}

  state RS_BUZZBUSY:

	ab_outf (RS_BUZZBUSY, "Buzzer already buzzing");
	proceed RS_DONE;

  state IF_READ:

	if (scan (ibuf + 2, "%u", &w0) != 1 || w0 >= IFLASH_SIZE)
		proceed RS_BAD;

	w1 = IFLASH [w0];

  state IF_READ_OUT:

	ab_outf (IF_READ_OUT, "IF [%u] = %x (%u, %d)", w0, w1, w1, w1);
	proceed RS_LOOP;

  state IF_WRITE:

	if (scan (ibuf + 2, "%u %u", &w0, &w1) != 2 ||
	    w0 >= IFLASH_SIZE)
		proceed RS_BAD;

	if (if_write (w0, w1))
		proceed RS_FAIL;

	proceed RS_DONE;

  state IF_ERASE:

	if (scan (ibuf + 2, "%d", &iarg0) != 1 ||
	    (iarg0 > 0 && iarg0 >= IFLASH_SIZE))
		proceed RS_BAD;

	if_erase (iarg0);
	proceed RS_DONE;
}
