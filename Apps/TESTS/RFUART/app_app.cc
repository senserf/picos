/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2014                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"
#include "ser.h"
#include "serf.h"
#include "form.h"
#include "phys_cc1100.h"
#include "plug_null.h"
#include "pins.h"
#include "pinopts.h"
#include "netid.h"

// ============================================================================
// ============================================================================

#ifndef	__SMURPH__

#include "cc1100.h"

#endif

// ============================================================================
// ============================================================================

// Packet format: NetID(w) ser(b) len(b) chars (CRC); +1 is for the sentinel
// byte
#define	FRAME_LENGTH	(2 + 2 + 2)
#define	MAX_LLENGTH	(CC1100_MAXPLEN - FRAME_LENGTH + 1)
#define	MAX_MPINS	8

// ============================================================================

const char *cmds [] = { "accel", "amode", "radio", "pins", NULL };
const char *onof [] = { "on", "off", "wor", NULL };
const char *amod [] = { "motion", "tap", "orient", "flat", "fall",
			       "shock", NULL };
const char *popt [] = { "monitor", "set", "get", "pulse", NULL };

const char *cmde [] = {
	"illegal command",
	"illegal option",
	"number expected",
	"number out of range",
	"already ON",
	"already OFF",
	"already WOR",
	"failed to spawn fsm",
	"duplicate value",
	"too many values",
	"not allowed",
};

// ============================================================================

sint RFD;
byte ISQ, ISQ_N = 1, OSQ, RFON = 1, AON, PINSN [MAX_MPINS], NPINS;
char PINSS [MAX_MPINS];

char GBuf [MAX_LLENGTH];

// ============================================================================

static void send_rf (const char *buf, sint len) {

	sint pln, i;
	address pkt;

	if (RFON != 1)
		return;

	if (len <= 0)
		len = strlen (buf);

	if (len > MAX_LLENGTH - 1)
		len = MAX_LLENGTH - 1;

	if ((pln = len + FRAME_LENGTH) & 1)
		pln++;

	for (i = 0; i < NCOPIES; i++) {
		if ((pkt = tcv_wnp (WNONE, RFD, pln)) == NULL)
			break;
		pkt [1] = OSQ | (len << 8);
		memcpy (pkt + 2, buf, len);
		tcv_endp (pkt);
	}

	if (i)
		OSQ++;
}

fsm uart_reader {

	char UIBuf [MAX_LLENGTH];

	state UR_WAIT:

		sint len;

		len = ser_in (UR_WAIT, UIBuf, MAX_LLENGTH);
		send_rf (UIBuf, len);
		sameas UR_WAIT;
}

fsm accel_thread {

	bma250_data_t c;
	char f;

	state AT_WAIT:

		f = 'V';
		delay (5 * 1024, AT_TIMEOUT);
		wait_sensor (SENSOR_MOTION, AT_EVENT);
		release;

	state AT_EVENT:

		f = 'E';

	state AT_TIMEOUT:

		read_sensor (AT_TIMEOUT, SENSOR_MOTION, (address)(&c));

		if (RFON != 1)
			sameas AT_WAIT;

		form (GBuf, "S%c [%x] %d <%d,%d,%d>", f,
			c.stat, c.temp, c.x, c.y, c.z);

		send_rf (GBuf, 0);
		sameas AT_WAIT;
}

static void radio_status () {

	word st;

	if (RFON == 1) {
		tcv_control (RFD, PHYSOPT_RXON, NULL);
	} else {
		st = (RFON != 0);
		tcv_control (RFD, PHYSOPT_RXOFF, &st);
	}
}

fsm radio_starter (word del) {

	state RS_WAIT:

		delay (del, RS_START);
		release;

	state RS_START:

		RFON = 1;
		radio_status ();
		finish;
}

static byte check_pins () {

	sint i;
	word p;
	char s;
	byte r;

	r = 0;
	for (i = 0; i < NPINS; i++) {
		p = pin_read (PINSN [i]) & 3;
		if (p & 2)
			s = '-';
		else
			s = '0' + (p & 1);
		if (s != PINSS [i]) {
			PINSS [i] = s;
			r++;
		}
	}

	return r;
}

fsm pin_monitor {

	state PM_INIT:

		bzero (PINSS, MAX_MPINS);

	state PM_CHECK:

		sint i;
		char *q;

		if (NPINS == 0)
			finish;

		if (check_pins ()) {
			// Report
			strcpy (GBuf, "Pins: ");
			strncpy (GBuf + strlen (GBuf), PINSS, NPINS);
			send_rf (GBuf, 0);
		}

		delay (256, PM_CHECK);
}

fsm pin_pulsar (word pin) {

	state PU_START:

		pin_write (pin, 1);
		delay (512, PU_OFF);
		release;

	state PU_OFF:

		pin_write (pin, 0);
		finish;
}

// ============================================================================

static void cmderr (byte error) {

	char *em;

	if (error < sizeof (cmde) / sizeof (const char*)) {
		send_rf (cmde [error], 0);
		return;
	}

	if ((em = (char*) umalloc (16)) == NULL)
		return;

	form (em, "Error: %x", error);

	send_rf (em, 0);

	ufree (em);
}

// ============================================================================

static Boolean isdelim (char c) {

	return isspace (c) || (c == ',');
}

static Boolean istermn (char c) {

	return isdelim (c) || (c == '\0');
}

static void skdelim (char **s) {

	while (isdelim (**s))
		(*s)++;
}

static sint ckwd (char **s, const char *t []) {

	sint i;
	char *p;
	const char *q;

	skdelim (s);

	for (i = 0; (q = t [i]) != NULL; i++) {
		p = *s;
		if (*p != *q)
			continue;
		do {
			p++;
			q++;
			if (*q == '\0')
				goto EOK;
			if (istermn (*p))
				goto EOS;

		} while (*p == *q);

		continue;
EOK:
		// OK, skip the source to delimiter
		while (!istermn (*p))
			p++;
EOS:
		// Skip delimiters
		skdelim (&p);
		*s = p;

		return i;
	}

	return -1;
}

static byte parse_number (char **s, word *w, word n) {

	char *p;
	word res;

	while (n--) {

		skdelim (s);

		p = *s;

		if (!isdigit (*p))
			return 2;

		res = 0;

		if (*p == '0' && (*(p+1) == 'x' || *(p+1) == 'X')) {
			// Hex mode
			p += 2;
			if (!isxdigit (*p))
				return 2;
			while (isxdigit (*p)) {
				if (res & 0xf000)
					return 3;
				res = (res << 4) | hexcode (*p);
				p++;
			}
		} else {
			// Decimal
			while (isdigit (*p)) {
				res = (res * 10) + ((*p) - '0');
				p++;
			}
		}

		*w++ = res;
		*s = p;
	}
	return 0;
}

static byte command_accel (char *src) {

	word w [3];
	byte res;

	if ((res = ckwd (&src, (const char**)onof)) < 0 || res > 1)
		return 1;

	if (res) {
		// Off
		if (!AON)
			return 5;
		if ((res = parse_number (&src, w, 1)) == 0) {
			// Number present
			if (w [0] > 12)
				return 3;
		} else if (res != 2) {
			return res;
		} else {
			w [0] = 0;
		}
		bma250_off ((byte)(w [0]));
		if (w [0] == 0) {
			// Complete off
			killall (accel_thread);
			AON = 0;
		}
		return 0;
	}

	// On
	if (AON)
		return 4;

	if ((res = parse_number (&src, w, 3)))
		return res;

	if (w [0] > 3 || w [1] > 7 || w [2] > 0xff)
		return 3;

	switch (w[0]) {
		case 0:  w[0] = BMA250_RANGE_2G; break;
		case 1:  w[0] = BMA250_RANGE_4G; break;
		case 2:  w[0] = BMA250_RANGE_8G; break;
		default: w[0] = BMA250_RANGE_16G;
	}

	bma250_on ((byte)(w[0]), (byte)(w[1]), (byte)(w[2]));

	AON = 1;

	killall (accel_thread);
	runfsm accel_thread;

	return 0;
}

static byte command_amode (char *src) {

	word w [4];
	byte res;

	if ((res = ckwd (&src, (const char**)amod)) < 0)
		return 1;

	switch (res) {

	    case 0:	// Motion

		if ((res = parse_number (&src, w, 2)))
			return res;

		bma250_move ((byte)(w[0]), (byte)(w[1]));
		break;

	    case 1:	// Tap

		if ((res = parse_number (&src, w, 4)))
			return res;

		if (w [0] > 3)
			return 3;

		bma250_tap ((byte)(w[0] << 6), (byte)(w[1]), (byte)(w[2]),
			(byte)(w[3]));
		break;
			
	    case 2:	// Orient

		if ((res = parse_number (&src, w, 4)))
			return res;

		if (w [0] > 3 || w [1] > 3 || w [2] > 63 || w [4] > 7)
			return 3;

		bma250_orient ((byte)(w[0] << 2), (byte)(w[1]), (byte)(w[2]),
			(byte)(w[3]));
		break;
		
	    case 3:	// Flat

		if ((res = parse_number (&src, w, 2)))
			return res;

		bma250_flat ((byte)(w[0]), (byte)(w[1]));
		break;

	    case 4:	// Fall

		if ((res = parse_number (&src, w, 4)))
			return res;

		if (w [0] > 1)
			return 3;

		if (w [0])
			w [0] = BMA250_LOWG_MODSUM;

		bma250_lowg ((byte)(w[0]), (byte)(w[1]), (byte)(w[2]),
			(byte)(w[3]));
		break;	

	    case 5:	// Shock

		if ((res = parse_number (&src, w, 3)))
			return res;

		bma250_highg ((byte)(w[0]), (byte)(w[1]), (byte)(w[2]));
		break;	

	    default:

		return 1;
	}

	return 0;
}

static byte command_radio (char *src) {

	word w;
	byte res;

	if ((res = ckwd (&src, (const char**)onof)) < 0)
		return 1;

	if (res == 0) {
		// On
		if (RFON == 1)
			return 4;
		RFON = 1;
		radio_status ();
		return 0;
	}

	if (res == 2) {
		// WOR
		if (RFON == 2)
			return 6;
		RFON = 2;
		radio_status ();
		return 0;
	}

	// OFF
	if ((res = parse_number (&src, &w, 1)) == 2) {
		w = 0;
	} else if (res != 0) {
		return res;
	}

	if (w > 60)
		// This is in seconds
		return 3;

	if (w) {
		w *= 1024;
		killall (radio_starter);
		if (!runfsm radio_starter (w))
			return 7;
	}

	RFON = 0;
	radio_status ();

	return 0;
}

static byte command_pins (char *src) {

	byte res;
	word w [2];
	sint i;

	if ((res = ckwd (&src, (const char**)popt)) < 0)
		return 1;

	if (res == 0) {
		// Monitor
		NPINS = 0;
		while (1) {
			if ((res = parse_number (&src, w, 1)) == 0) {
				// Number present
				if (w [0] > 48)
					return 3;
				// Check if already specified
				for (i = 0; i < NPINS; i++)
					if (PINSN [i] == (byte)(w [0]))
						return 8;
				if (NPINS == MAX_MPINS)
					return 9;
				PINSN [NPINS++] = (byte)(w [0]);
			} else if (res == 2)
				break;
			else
				return res;
		}
		if (NPINS) {
			if (!running (pin_monitor)) {
				send_rf ("Starting", 0);
				runfsm pin_monitor;
			}
		} else {
			send_rf ("Off", 0);
		}
		return 0;
		// Note that "no pins" means off
	}

	if (res == 1) {
		// Set 
		if ((res = parse_number (&src, w, 2)))
			return res;
		if (w [0] > 48 || w [1] > 1)
			return 3;
		if ((pin_read (w [0]) && 0xE) != 0x2)
			// Not output
			return 10;
		pin_write (w [0], w [1]);
		return 0;
	}

	if (res == 2) {
		// Get
		if ((res = parse_number (&src, w, 1)))
			return res;
		if (w [0] > 48)
			return 3;
		form (GBuf, "Pin %d = %x", w [0], pin_read (w [0]));
		send_rf (GBuf, 0);
		return 0;
	}

	// Pulse
	if ((res = parse_number (&src, w, 1)))
		return res;
	if (w [0] > 48)
		return 3;

	if ((pin_read (w [0]) && 0xE) != 0x2)
		// Not output
		return 10;

	runfsm pin_pulsar (w [0]);
	return 0;
}

fsm root {

	char RIBuf [MAX_LLENGTH];

	state RO_INIT:

		word netid;

		tcv_plug (0, &plug_null);
		phys_cc1100 (0, CC1100_MAXPLEN);
		if ((RFD = tcv_open (WNONE, 0, 0)) < 0)
			syserror (EHARDWARE, "RF");
		netid = NETID;
		tcv_control (RFD, PHYSOPT_SETSID, &netid);
		RFON = 1;
		radio_status ();
		runfsm uart_reader;
		powerdown ();

	state RO_WAITRF:

		char *src;
		address pkt;
		sint len;
		byte sq, ln;

		pkt = tcv_rnp (RO_WAITRF, RFD);
		len = tcv_left (pkt);

		if (len < 8) {
Drop:
			tcv_endp (pkt);
			sameas RO_WAITRF;
		}

		sq = pkt [1] & 0xFF;
		if (!ISQ_N && sq == ISQ)
			// Duplicate
			goto Drop;

		ln = (pkt [1] >> 8) & 0xFF;
		if (ln > len - 6)
			// Length error
			goto Drop;

		ISQ_N = 0;
		ISQ = sq;

		memcpy (RIBuf, pkt + 2, ln);
		RIBuf [ln] = '\0';

		tcv_endp (pkt);

		if (RIBuf [0] == ':')
			// The rest goes to the UART
			sameas WUART;

		src = RIBuf;

		switch (ckwd (&src, (const char**)cmds)) {

		    case 0:	sq = command_accel (src); break;
		    case 1:	sq = command_amode (src); break;
		    case 2:	sq = command_radio (src); break;
		    case 3:	sq = command_pins  (src); break;

		    default:
				cmderr (0);
				sameas RO_WAITRF;

		}

		if (sq)
			cmderr (sq);
		else
			send_rf ("ok", 0);

		sameas RO_WAITRF;

	state WUART:

		ser_outf (WUART, "%s\r\n", RIBuf + 1);
		sameas RO_WAITRF;

}
