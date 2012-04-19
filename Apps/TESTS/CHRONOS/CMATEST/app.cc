/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "ez430_lcd.h"
#include "rtc_cc430.h"
#include "tcvphys.h"
#include "cc1100.h"
#include "phys_cc1100.h"
#include "plug_null.h"
#include "form.h"
#include "buttons.h"
#include "sensors.h"
#include "storage.h"
#include "ab.h"

#define MAXPLEN	CC1100_MAXPLEN

static word 	imess,	// Count received
		omess;	// ... and outgoing messages

static byte	Buttons;

static int sfd = -1;
static byte ron = 0,	// Radio on
	    aon = 0;	// Accelerometer on
static word rtimer = 0;	// Radio back on timer

static word atimeout = 0,		// Accel event timeout, every that many
					// msec read data even if no event

       	    areadouts = 1,		// After event read this many times ...
	    ainterval = 0;  		// ... at this interval before
					// returning to MD mode

// ============================================================================

static void radio_on () {

	if (!ron) {
		tcv_control (sfd, PHYSOPT_TXON, NULL);
		tcv_control (sfd, PHYSOPT_RXON, NULL);
		ron = 1;
	}
}

static void radio_off () {

	if (ron) {
		tcv_control (sfd, PHYSOPT_RXOFF, NULL);
		tcv_control (sfd, PHYSOPT_TXOFF, NULL);
		ron = 0;
	}
}

// ============================================================================

void msg_lcd (const char *txt, word fr, word to) {
//
// Displays characters on the LCD
//
	char c;

	while (1) {
		if ((c = *txt) != '\0') {
			if (c >= 'a' && c <= 'z')
				c -= ('a' - 'A');
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

static void buttons (word but) {

	Buttons |= (1 << but);
	trigger (&Buttons);

}

// ============================================================================

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

	radio_on ();
	// One-time read
	read_sensor (AT_TIMEOUT, SENSOR_MOTION, (address) c);

  state AT_SEND_PERIODIC:

	ab_outf (AT_SEND_PERIODIC, "P: [%x] <%d,%d,%d>",
		c [0], c [1], c [2], c [3]);
	msg_nn (1, omess++);
Revert:
	if (areadouts == 0)
		// No wait: revert to event mode
		wait_sensor (SENSOR_MOTION, WNONE);
	proceed AT_WAIT;

  state AT_EVENT:

	radio_on ();
	nc = areadouts;

  state AT_READOUT:

	read_sensor (AT_READOUT, SENSOR_MOTION, (address) c);

  state AT_SEND_EVENT:

	ab_outf (AT_SEND_EVENT, "E: [%x] <%d,%d,%d>",
		c [0], c [1], c [2], c [3]);
	msg_nn (1, omess++);

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
  int i0;
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
	radio_on ();

	ab_init (sfd);
	ab_mode (AB_MODE_PASSIVE);
	buttons_action (buttons);
	runfsm button_server;
	cma3000_off ();
	powerdown ();

  state RS_LOOP:

	if (ibuf != NULL) {
		ufree (ibuf);
		ibuf = NULL;
	}

  state RS_READ:

	if (!ron && rtimer)
		delay (rtimer, RS_FRON);
	ibuf = ab_in (RS_READ);
	unwait ();
	rtimer = 0;
	msg_nn (0, imess++);

	switch (ibuf [0]) {

		case 'a' : proceed RS_ASON;	// Start acceleration reports
		case 'D' : proceed RS_DEBREP;
		case 'q' : proceed RS_ASOFF;	// Stop acceleration reports
		case 's' : proceed RS_SETDEL;	// Set delays for acc reports
		case 'h' : proceed RS_HANG;	// Hung RF till accel event
		case 'd' : proceed RS_DISP;	// Display on/off
		case 'r' : proceed RS_RTC;	// Set or get RTC
		case 'b' : proceed RS_BUZZ;
		case 'p' : proceed RS_POW;
		case 'f' : {			// INFO FLASH operations
			switch (ibuf [1]) {
				case 'r' : proceed IF_READ;
				case 'w' : proceed IF_WRITE;
				case 'e' : proceed IF_ERASE;
			}
		}
	}

  state RS_BAD:

	radio_on ();
	ab_outf (RS_BAD, "Illegal command");
	proceed RS_LOOP;

  state RS_DEBREP:

	radio_on ();
/*
	ab_outf (RS_DEBREP, "DBG: %x %x %x", cma3000_debug0,
					     cma3000_debug1,
					     cma3000_debug2);
*/
	proceed RS_LOOP;

  state RS_FRON:

	// Force radio on
	radio_on ();
	proceed RS_LOOP;

  // ==========================================================================

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
	aon = 1;
Restart:
	killall (accel_thread);
	runfsm accel_thread;

	proceed RS_DONE;

  state RS_ASOFF:

	if (!aon)
		proceed RS_BAD;

	killall (accel_thread);
	cma3000_off ();
	aon = 0;

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

  state RS_HANG:

	if (!aon)
		proceed RS_BAD;

	w0 = 0;
	scan (ibuf + 1, "%d", &w0);
	if (w0) {
		if (w0 > 60)
			w0 = 60;
		w0 *= 1024;
	}

	rtimer = w0;
	radio_off ();
	proceed RS_LOOP;

  state RS_DISP:

	w0 = 0;
	scan (ibuf + 1, "%d", &w0);

	if (w0)
		ezlcd_on ();
	else
		ezlcd_off ();
	proceed RS_DONE;

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

	if (scan (ibuf + 2, "%d", &i0) != 1 ||
	    (i0 > 0 && i0 >= IFLASH_SIZE))
		proceed RS_BAD;

	if_erase (i0);
	proceed RS_DONE;

  state RS_DONE:

	ab_outf (RS_DONE, "OK");
	proceed RS_LOOP;

  state RS_FAIL:

	ab_outf (RS_FAIL, "Failed");
	proceed RS_LOOP;
}
