/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "ez430_lcd.h"
#include "tcvphys.h"
#include "phys_cc1100.h"
#include "plug_null.h"
#include "form.h"
#include "buttons.h"

#define	MIN_PACKET_LENGTH	24
#define	MAX_PACKET_LENGTH	42

#define MAXPLEN			(MAX_PACKET_LENGTH + 2)
#define	SEND_INTERVAL		1000

static int 	sfd = -1, packet_length, rcvl;
static lword	last_sent, last_rcv;
static word	rssi;
static address	packet;

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

void msg_er (char c, word n) {

	char erm [6];

	if (n > 9999)
		n = 9999;

	form (erm, "%c%u", c, n);
	msg_lo (erm);
	while (1);
}

void msg_hx (char c, word a) {

	char erm [6];

	if (c == 0) {
		form (erm, "%x", a);
		msg_hi (erm);
	} else {
		form (erm, "%c%x", c, a);
		msg_lo (erm);
	}
}

// ============================================================================

static void radio_start (word);
static void radio_stop ();

// ============================================================================

static void buttons (word but) {

	switch (but) {

		case BUTTON_M1:

			radio_start (3);
			return;

		case BUTTON_M2:

			radio_stop ();
			return;
	}
}

// ============================================================================
// Copied from TEST/WARSAW ====================================================
// ============================================================================

static word gen_packet_length (void) {

#if MIN_PACKET_LENGTH >= MAX_PACKET_LENGTH
	return MIN_PACKET_LENGTH;
#else
	return ((rnd () % (MAX_PACKET_LENGTH - MIN_PACKET_LENGTH + 1)) +
			MIN_PACKET_LENGTH) & 0xFFE;
#endif

}

// ============================================================================

#define	SN_SEND		0
#define	SN_NEXT		1

char smsg [6];

thread (sender)

  int pl, pp;

  entry (SN_SEND)

	packet_length = gen_packet_length ();

	if (packet_length < 10)
		packet_length = 10;
	else if (packet_length > MAX_PACKET_LENGTH)
		packet_length = MAX_PACKET_LENGTH;

  entry (SN_NEXT)

	packet = tcv_wnp (SN_NEXT, sfd, packet_length + 2);

	packet [0] = 0;
	packet [1] = 0xBABA;

	// In words
	pl = packet_length / 2;
	((lword*)packet)[1] = wtonl (last_sent);

	for (pp = 4; pp < pl; pp++)
		packet [pp] = (word) entropy;

	tcv_endp (packet);

	// Display the number sent modulo 9999
	form (smsg, "%u", (word) (last_sent % 10000));
	msg_hi (smsg);

	last_sent++;
	delay (SEND_INTERVAL, SN_SEND);

endthread;

// ============================================================================

#define	RC_TRY		0

char rmsg [6];

thread (receiver)

  address packet;

  entry (RC_TRY)

	packet = tcv_rnp (RC_TRY, sfd);
	last_rcv = ntowl (((lword*)packet) [1]);
	rcvl = tcv_left (packet) - 2;
	rssi = packet [rcvl >> 1];
	tcv_endp (packet);

	form (rmsg, "%u", (word) (last_rcv % 10000));

	msg_lo (rmsg);

	proceed (RC_TRY);

endthread;

// ============================================================================

static void radio_start (word d) {

	if (sfd < 0)
		return;

	if (d & 1) {
		tcv_control (sfd, PHYSOPT_RXON, NULL);
		if (!running (receiver))
			runthread (receiver);
	}
	if (d & 2) {
		tcv_control (sfd, PHYSOPT_TXON, NULL);
		if (!running (sender))
			runthread (sender);
	}
}

static void radio_stop () {

	if (sfd < 0)
		return;

	killall (sender);
	killall (receiver);

	tcv_control (sfd, PHYSOPT_RXOFF, NULL);
	tcv_control (sfd, PHYSOPT_TXOFF, NULL);
}

// ============================================================================

#define	RS_INIT		0
#define	RS_HEART	1
#define	RS_TOGGLE	2

thread (root)

  entry (RS_INIT)

	ezlcd_init ();
	ezlcd_on ();

	msg_hi ("OLSO");
	msg_lo ("PICOS");
	mdelay (2000);

	// Initialize things for radio
	phys_cc1100 (0, MAXPLEN);

	msg_hi ("CC11");
	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);

	msg_hi ("VNET");

	if (sfd < 0)
		msg_er ('A', 1);

	buttons_action (buttons);

	mdelay (1000);
	msg_hi ("-----");
	msg_lo ("-----");

	// Use buttons
	// radio_start (3);

  entry (RS_HEART)

	ezlcd_item (LCD_ICON_HEART, LCD_MODE_SET);
	delay (512, RS_TOGGLE);
	release;

  entry (RS_TOGGLE)

	ezlcd_item (LCD_ICON_HEART, LCD_MODE_CLEAR);
	delay (512, RS_HEART);

endthread
