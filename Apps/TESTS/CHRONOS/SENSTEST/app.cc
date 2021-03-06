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
#include "cc1100.h"
#include "phys_cc1100.h"
#include "plug_null.h"
#include "form.h"
#include "sensors.h"
#include "storage.h"
#include "ab.h"

// ============================================================================

#ifdef	BUTTONS
#include "buttons.h"
#endif

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

#ifdef BOARD_CHRONOS_WHITE
// BMA250 and BMP085
#define	accel_off()	bma250_off (0)


#else
// CMA3000 and SCP1000
#define	accel_off()	cma3000_off ()

#endif

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

#ifdef	BUTTONS
static void buttons (word but) {

	Buttons |= (1 << but);
	trigger (&Buttons);

}
#endif

// ============================================================================

#ifdef	BUTTONS

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

static const byte btable [] = {
	BUTTON_M1, BUTTON_M2, BUTTON_S1, BUTTON_S2, BUTTON_BL
};

fsm button_server {

  char butts [6];

  state BS_LOOP:

	wait_sensor (SENSOR_BUTTONS, BS_PRESSED);

  state BS_PRESSED:

	byte bv [2], i;

	read_sensor (BS_PRESSED, SENSOR_BUTTONS, (address) bv);

	for (i = 0; i < 5; i++)
		butts [i] = (bv [1] & btable [i]) ? 'X' : '-';

	butts [5] = '\0';

  state BS_OUT:

	ab_outf (BS_OUT, "Buttons: %s", butts);
	msg_nn (1, omess++);
	proceed BS_LOOP;
}

#endif /* BUTTONS_SERVER */

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

#ifdef BOARD_CHRONOS_WHITE

  bma250_data_t c;
#define SDATA (address) (&c)
#define	SVALU "[%x] %d <%d,%d,%d>", c.stat, c.temp, c.x, c.y, c.z

#else

  char c [4];
#define SDATA (address) (&c)
#define	SVALU "[%x] <%d,%d,%d>", c [0], c [1], c [2], c [3]

#endif

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
	read_sensor (AT_TIMEOUT, SENSOR_MOTION, SDATA);

  state AT_SEND_PERIODIC:

	ab_outf (AT_SEND_PERIODIC, "P: " SVALU);
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

	read_sensor (AT_READOUT, SENSOR_MOTION, SDATA);

  state AT_SEND_EVENT:

	ab_outf (AT_SEND_EVENT, "E: " SVALU);
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

  word w [11];
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

	w[0] = 0xffff;
	tcv_control (sfd, PHYSOPT_SETSID, w+0);
	radio_on ();

	ab_init (sfd);
	ab_mode (AB_MODE_PASSIVE);
#ifdef	BUTTONS
	buttons_action (buttons);
#endif
	runfsm button_server;
	accel_off ();
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
#ifdef	BOARD_CHRONOS_WHITE
		case 'm' : proceed RS_MODE;
#endif
		case 'D' : proceed RS_DEBREP;
		case 'q' : proceed RS_ASOFF;	// Stop acceleration reports
		case 's' : proceed RS_SETDEL;	// Set delays for acc reports
		case 'h' : proceed RS_HANG;	// Hung RF till accel event
		case 'd' : proceed RS_DISP;	// Display on/off
		case 'r' : proceed RS_RTC;	// Set or get RTC
		case 'b' : proceed RS_BUZZ;
		case 'p' : proceed RS_PRESS;	// Read pressure sensor
		case 'P' : proceed RS_POW;
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

	w[0] = 0;

	scan (ibuf + 1, "%u", w+0);

	if (w[0])
		powerup ();
	else
		powerdown ();

	proceed RS_DONE;

  state RS_ASON:

	if (aon)
		proceed RS_BAD;

	w[0] = 0;
	w[1] = 0;
	w[2] = 0;

#ifdef	BOARD_CHRONOS_WHITE

	scan (ibuf + 1, "%u %u %x", w+0, w+1, w+2);

	switch (w[0]) {
		case 0:  w[0] = BMA250_RANGE_2G; break;
		case 1:  w[0] = BMA250_RANGE_4G; break;
		case 2:  w[0] = BMA250_RANGE_8G; break;
		default: w[0] = BMA250_RANGE_16G;
	}

	if (w[1] > 7)
		w[1] = 7;

	bma250_on ((byte)(w[0]), (byte)(w[1]), (byte)(w[2]));

#else
	scan (ibuf + 1, "%u %u %u", w+0, w+1, w+2);
	cma3000_on (w[0], w[1], w[2]);
#endif

	aon = 1;
Restart:
	killall (accel_thread);
	runfsm accel_thread;

	proceed RS_DONE;

#ifdef	BOARD_CHRONOS_WHITE

  state RS_MODE:

	w[0] = w[1] = w[2] = w[3] = w[4] = 0;

	scan (ibuf + 1, "%c %u %u %u %u", w+0, w+1, w+2, w+3, w+4);

	switch (w[0]) {
		// Mode select
		case 'm':
			// Movement
			bma250_move ((byte)(w[1]), (byte)(w[2]));
			break;

		case 't':
			// Tap
			if (w[1] > 3)
				w[1] = 3;
			bma250_tap ((byte)(w[1] << 6), (byte)(w[2]),
				(byte)(w[3]), (byte)(w[4]));
			break;

		case 'o':
			// Orient
			if (w[1] > 3)
				w[1] = 3;
			if (w[2] > 3)
				w[2] = 3;
			if (w[3] > 63)
				w[3] = 63;
			if (w[4] > 7)
				w[4] = 7;
			bma250_orient ((byte)(w[1] << 2), (byte)(w[2]),
				(byte)(w[3]), (byte)(w[4]));
			break;

		case 'f':
			// Flat
			bma250_flat ((byte)(w[1]), (byte)(w[2]));
			break;

		case 'l':
			// Lowg
			if (w[1])
				w[1] = BMA250_LOWG_MODSUM;
			bma250_lowg ((byte)(w[1]), (byte)(w[2]), (byte)(w[3]),
				(byte)(w[4]));
			break;

		case 'h':
			// Highg
			bma250_highg ((byte)(w[1]), (byte)(w[2]), (byte)(w[3]));
			break;

		default:
			proceed RS_BAD;
	}

	proceed RS_DONE;
#endif

  state RS_ASOFF:

	if (!aon)
		proceed RS_BAD;

#ifdef	BOARD_CHRONOS_WHITE

	w[0] = 0;

	scan (ibuf + 1, "%u", w+0);

	if (w[0] > 12)
		w[0] = 12;

	bma250_off ((byte)(w[0]));

	if (w[0] == 0) {
		killall (accel_thread);
		aon = 0;
	}
#else
	killall (accel_thread);
	accel_off ();
	aon = 0;
#endif
	proceed RS_DONE;

  state RS_SETDEL:

	w[0] = w[1] = w[2] = 0;
	scan (ibuf + 1, "%u %u %u", w+0, w+1, w+2);
	if (w[0] > 60)
		w[0] = 60;

	if (w[0] == 0 && w[1] == 0)
		proceed RS_BAD;

	atimeout = w[0] * 1024;
	areadouts = w[1];
	ainterval = w[2];

	if (aon)
		goto Restart;

	proceed RS_DONE;

  state RS_HANG:

	if (!aon)
		proceed RS_BAD;

	w[0] = 0;
	scan (ibuf + 1, "%d", w+0);
	if (w[0]) {
		if (w[0] > 60)
			w[0] = 60;
		w[0] *= 1024;
	}

	rtimer = w[0];
	radio_off ();
	proceed RS_LOOP;

  state RS_DISP:

	w[0] = 0;
	scan (ibuf + 1, "%d", w+0);

	if (w[0])
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

	w[0] = 0;
	scan (ibuf + 1, "%u", w+0);
	if (w[0] == 0)
		proceed RS_BAD;

	if (!running (buzz)) {
		runfsm buzz (w[0]);
		proceed RS_DONE;
	}

  state RS_BUZZBUSY:

	ab_outf (RS_BUZZBUSY, "Buzzer already buzzing");
	proceed RS_DONE;

  state IF_READ:

	if (scan (ibuf + 2, "%u", w+0) != 1 || w[0] >= IFLASH_SIZE)
		proceed RS_BAD;

	w[1] = IFLASH [w[0]];

  state IF_READ_OUT:

	ab_outf (IF_READ_OUT, "IF [%u] = %x (%u, %d)", w[0], w[1], w[1], w[1]);
	proceed RS_LOOP;

  state IF_WRITE:

	if (scan (ibuf + 2, "%u %u", w+0, w+1) != 2 ||
	    w[0] >= IFLASH_SIZE)
		proceed RS_BAD;

	if (if_write (w[0], w[1]))
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

  state RS_PRESS:

#ifdef BOARD_CHRONOS_WHITE

#ifdef	BMP085_AUTO_CALIBRATE
#define	PVALU "%lu, %u", *((lword*)w), w [2]
#else
#define	PVALU "%u, %u", w [1], w [0]
	if (ibuf [1] == 'c')
		// Read calibration data
		proceed RS_PRESS_CALIB;
#endif
	w[0] = WNONE;
	scan (ibuf + 1, "%d", w+0);
	if (w[0] != WNONE)
		bmp085_oversample ((byte)(w[0]));


#else
#define	PVALU "%lu, %u", *((lword*)w), w [2]
#endif

  state RS_PRESS_READ:

	read_sensor (RS_PRESS_READ, SENSOR_PRESSTEMP, w);

  state RS_PRESS_SEND:

	ab_outf (RS_PRESS_SEND, "PR: " PVALU);
	proceed RS_LOOP;

#if defined(BOARD_CHRONOS_WHITE) && !defined(BMP085_AUTO_CALIBRATE)

  state RS_PRESS_CALIB:

	read_sensor (RS_PRESS_CALIB, SENSOR_PRESSTEMP_CALIB, w);
	i0 = 0;

  state RS_PRESS_CALIB_OUT:

	ab_outf (RS_PRESS_CALIB_OUT, "PP [%d]: %x %u", i0, w[i0], w[i0]);

	if (i0 == 10)
		proceed RS_LOOP;

	i0++;
	proceed RS_PRESS_CALIB_OUT;
#endif

}
