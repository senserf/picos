/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "ez430_lcd.h"
#include "rtc_cc430.h"
#include "tcvphys.h"
#include "phys_cc1100.h"
#include "plug_null.h"
#include "form.h"
#include "buttons.h"
#include "sensors.h"
#include "ab.h"

#define MAXPLEN			60

static word 	imess,	// Count received
		omess;	// ... and outgoing messages

static byte	Buttons;

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
		form (erm, "%u", a % 9999);
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
// Array to return sensor values. For the acceleration sensor:
//
//	aval [0] = the time stamp as the number of seconds ago (up to 0xffff)
//	aval [1] = the total number of movement events since last take (zeroed
//	           after collection; if this is zero, the time stamp is zero
//		   as well
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

  word aval [2], pval [3];

  state AS_LOOP:

	if (sval_rint == 0) {
		// Stop
		cma_3000_off ();
		when (&sval_rint, AS_LOOP);
		release;
	}

  state AS_RPRE:

	read_sensor (AS_RPRE, PRESSURE_SENSOR, pval);

  state AS_RACC:

	read_sensor (AS_RACC, MOTION_SENSOR, aval);

  state AS_SEND:

	ab_outf (AS_SEND, "MO: %u %u, PR: %lu, TM: %u",
		aval [0], aval [1], ((lword*)pval) [0], pval [2]);

	msg_nn (1, omess++);

	delay (sval_rint, AS_LOOP);
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

fsm root {

  int sfd = -1;
  word pddelay, pdcount;
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

	pddelay = 0xffff;
	tcv_control (sfd, PHYSOPT_SETSID, &pddelay);
	tcv_control (sfd, PHYSOPT_TXON, NULL);
	tcv_control (sfd, PHYSOPT_RXON, NULL);

	ab_init (sfd);
	ab_mode (AB_MODE_PASSIVE);
	buttons_action (buttons);
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
	}

  state RS_BAD:

	ab_outf (RS_BAD, "Illegal command");
	proceed RS_LOOP;

  // ==========================================================================

  state RS_DDOWN:

	pddelay = 0;
	scan (ibuf + 1, "%u", &pddelay);
	if (pddelay == 0)
		proceed RS_BAD;

	ezlcd_off ();
	powerdown ();
	tcv_control (sfd, PHYSOPT_RXOFF, NULL);
	tcv_control (sfd, PHYSOPT_TXOFF, NULL);

  state RS_DLOOP:

	if (pddelay) {
		pddelay--;
		delay (1024, RS_DLOOP);
		release;
	}

	powerup ();
	ezlcd_on ();
	tcv_control (sfd, PHYSOPT_RXON, NULL);
	tcv_control (sfd, PHYSOPT_TXON, NULL);
	proceed RS_DONE;

  state RS_PDOWN:

	pdcount = pddelay = 0;
	scan (ibuf + 1, "%u %u", &pddelay, &pdcount);

	if (pddelay == 0 || pdcount == 0)
		proceed RS_BAD;

  state RS_HOLD:

	powerdown ();
	tcv_control (sfd, PHYSOPT_RXOFF, NULL);
	tcv_control (sfd, PHYSOPT_TXOFF, NULL);
	delay (pddelay, RS_ON);
	release;

  state RS_ON:

	powerup ();
	tcv_control (sfd, PHYSOPT_RXON, NULL);
	tcv_control (sfd, PHYSOPT_TXON, NULL);

  state RS_TICK:

	ab_outf (RS_TICK, "Up for a sec");
	if (pdcount <= 1) {
		// Exit
		powerup ();
		proceed RS_DONE;
	}
	pdcount--;
	delay (1000, RS_HOLD);
	release;

  // ==========================================================================

  state RS_ASON:

	// Default interval
	pddelay = 0;
	scan (ibuf + 1, "%u", &pddelay);
	sval_rint = (pddelay == 0) ? 1024 : pddelay;
	cma_3000_on ();
STrig:
	trigger (&sval_rint);

  state RS_DONE:

	ab_outf (RS_DONE, "OK");
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

	pddelay = 0;
	scan (ibuf + 1, "%u", &pddelay);
	if (pddelay == 0)
		proceed RS_BAD;

	buzzer_on ();
	delay (pddelay, RS_BUZZF);
	release;

  state RS_BUZZF:

	buzzer_off ();
	proceed RS_DONE;
}
