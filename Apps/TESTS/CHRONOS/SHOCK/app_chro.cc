#include "sysio.h"
#include "cc1100.h"
#include "tcvphys.h"
#include "phys_cc1100.h"
#include "plug_null.h"
#include "storage.h"
#include "bma250.h"
#include "bmp085.h"
#include "rtc_cc430.h"

#include "display.h"
#include "rf.h"
#include "watch.h"
#include "netid.h"
#include "ossi.h"

// ============================================================================
// ============================================================================

byte	RadioOn, 	// Just 0 or 1, RadioDelay indicates the RX open time
	AccelOn;

	//	0 - seconds
	//	1 - minutes
	//	2 - AM/PM/24H
	//	3 - hours
	//	4 - day
	//	5 - month
	//	6 - year
byte	WatchBeingSet = BNONE;

lword	AccelEvents, AccelTotal;
word	AccelLast, AccelMax;

// ============================================================================

static lword AccelWhen, AccelUntil;
static lword AccelBuf [MAX_ACC_PACK];
static word AccelInterval, AccelSernum;
static byte AccelPack, AccelFill;
// The default of zero won't modify any registers leaving them at the
// default settings
static bma250_regs_t AccelRegs = { 0x00003017, { 0x03, 0x0f, 0x00, 0x00, 0x07,
						 0x00, 0x00, 0x00, 0x00, 0x00,
						 0x00, 0x00, 0x01, 0x4d, 0x00,
						 0x00, 0x00, 0x00, 0x00, 0x00 }
				 };

// Registers for high-speed collection of raw data
static const bma250_regs_t CollectRegs = {
				   0x000fffff, { 0x03, 0x0f, 0x00, 0x00,
						 0x00, 0x00, 0x00, 0x09,
						 0x30, 0x81, 0x0f, 0xc0,
						 0x00, 0x14, 0x04, 0x0a,
						 0x18, 0x08, 0x08, 0x10 }
				  };

static const item_t *TheItem;
static sint RFC;
static word RadioDelay = RADIO_WOR_IDLE_TIMEOUT;
static byte TheButton, Item, LastRef, LCDOn, AccelRequest, PressRequest;
static Boolean SetMode = NO;

// ============================================================================

static const item_t items [N_ITEMS] = {

	{ watch_display, watch_option, watch_enter_set_mode,
		watch_exit_set_mode, watch_set_next, watch_set_value },

	{ radio_display, NULL, radio_enter_set_mode, radio_exit_set_mode,
		NULL, radio_set_value },

	{ accel_display, accel_option, accel_enter_set_mode,
		accel_exit_set_mode, NULL, accel_set_value },

	{ press_display, NULL, NULL, NULL, NULL, NULL },
};

// ============================================================================

static void display_item (byte n) {

	if ((Item = n) == BNONE) {
		// Zero out the display
		TheItem = NULL;
		display_clear (0);
		display_clear (1);
	} else {
		(TheItem = items + Item)->fn_display ();
	}
}

static void next_item () {

	display_item (Item == BNONE ? 0 : (Item + 1) % N_ITEMS);
}

// ============================================================================

static address new_msg (byte code, word len) {
//
// Tries to acquire a packet for outgoing RF message; len == parameter length
//
	address msg;

	if ((msg = tcv_wnp (WNONE, RFC, len + RFPFRAME + sizeof (oss_hdr_t))) !=
	    NULL) {
		msg [1] = NODE_ID;
		osshdr (msg) -> code = code;
		osshdr (msg) -> ref = LastRef;
	}
	return msg;
}

static void oss_ack (word status) {

	address msg;

	if ((msg = new_msg (0, sizeof (status))) != NULL) {
		osspar (msg) [0] = status;
		tcv_endp (msg);
	}
}

// ============================================================================

static void switch_lcd (byte on) {

	if (LCDOn != on) {
		if (on) {
			ezlcd_on ();
			display_item (ITEM_WATCH);
		} else {
			display_item (BNONE);
			ezlcd_off ();
		}
		LCDOn = on;
	}
}

// ============================================================================

void switch_radio (byte on) {

	word par [2];

	if (RadioOn != on) {
		par [0] = RadioOn = on;
		tcv_control (RFC, PHYSOPT_OFF, par);
	}

	if (RadioOn) {
		par [0] = RadioDelay;
		par [1] = 2048;		// This is just in case
		tcv_control (RFC, PHYSOPT_SETPARAMS, par);
	}

	display_rfstat ();
}

// ============================================================================

fsm accel_reader {
//
// Here is a thread to safely communicate with the accelerometer
//
	state AR_LOOP:

		if (AccelRequest & ACCRQ_ON) {
			// On or reset
			bma250_on (&AccelRegs);
			AccelOn = 1;
			_BIC (AccelRequest, ACCRQ_ON);
		}

		if (!AccelOn) {
			// This is a precaution
Off:
			AccelRequest = 0;
			finish;
		}

		if (AccelRequest & ACCRQ_VALUE) {
			_BIC (AccelRequest, ACCRQ_VALUE);
			sameas AR_READ;
		}

		if (AccelRequest & ACCRQ_OFF) {
			// The last one to try
			bma250_off ();
			AccelOn = NO;
			goto Off;
		}

		when (&AccelRequest, AR_LOOP);
		if (AccelInterval)
			delay (AccelInterval, AR_REPORT);
		wait_sensor (SENSOR_MOTION, AR_EVENT);

	state AR_READ:

		bma250_data_t val;
		address msg;

		read_sensor (AR_READ, SENSOR_MOTION, (address)(&val));

		if ((msg = new_msg (message_accvalue_code,
		    sizeof (message_accvalue_t))) != NULL) {

			memcpy (osspar (msg), &val,
			    sizeof (message_accvalue_t));

			tcv_endp (msg);
		}

		sameas AR_LOOP;

	state AR_REPORT:

		bma250_data_t val;
		address msg;
		message_accreport_t *pmt;

		read_sensor (AR_REPORT, SENSOR_MOTION, (address)(&val));

		AccelBuf [AccelFill] = (((lword) (val.x & 0x3ff)) << 20) |
				       (((lword) (val.y & 0x3ff)) << 10) |
				       (((lword) (val.z & 0x3ff))      ) ;
		if (++AccelFill == AccelPack) {
			// Expedite the packet
			AccelFill <<= 2;
			if ((msg = new_msg (message_accreport_code,
				8 + sizeof (word) + AccelFill)) != NULL) {

				rtc_get (&RTC);
				pmt = (message_accreport_t*) osspar (msg);
				pmt->time [0] = RTC.year;
				pmt->time [1] = RTC.month;
				pmt->time [2] = RTC.day;
				pmt->time [3] = RTC.hour;
				pmt->time [4] = RTC.minute;
				pmt->time [5] = RTC.second;
				pmt->sernum = AccelSernum;
				pmt->data.size = AccelFill;
				memcpy (pmt->data.content, AccelBuf, AccelFill);
				tcv_endp (msg);
			}
			AccelFill = 0;
			AccelSernum++;
		}

		sameas AR_LOOP;

	state AR_EVENT:

		bma250_data_t val;

		read_sensor (AR_EVENT, SENSOR_MOTION, (address)(&val));

		if (val.x < 0)
			val.x = -val.x;
		if (val.y < 0)
			val.y = -val.y;
		if (val.z < 0)
			val.z = -val.z;

		AccelLast = val.x + val.y + val.z;
		AccelTotal += AccelLast;
		if (AccelLast > AccelMax)
			AccelMax = AccelLast;
		AccelEvents++;

		if (Item == ITEM_ACCEL && !SetMode)
			accel_update_display ();

		sameas AR_LOOP;
}

static word accel_trigger (byte c) {

	switch (c) {

		case ACCRQ_ON:

			if (!running (accel_reader) && (!runfsm accel_reader))
				return 1;
			break;

		case ACCRQ_VALUE:

			if (!AccelOn)
				return 2;
			break;

		default:	// OFF

			if (!AccelOn && !running (accel_reader))
				return 0;
	}

	_BIS (AccelRequest, c);
	trigger (&AccelRequest);

	return 0;
}

void accel_clear () {

	AccelEvents = AccelTotal = 0;
	AccelLast = AccelMax = 0;
}

// ============================================================================

fsm accel_starter (byte on) {
//
// This extra thread is needed primarily to implement the delayed startup of 
// the accelerometer
//
	state AS_INIT:

		if (on)
			sameas AS_TURN_ON;

		// Turning off
		if (AccelOn) {
			accel_trigger (ACCRQ_OFF);
			delay (256, AS_INIT);
			release;
		}

		finish;

	state AS_TURN_ON:

		if (AccelWhen > seconds ()) {
			// Must be off until the target time
			if (AccelOn)
				accel_trigger (ACCRQ_OFF);
			delay (1024, AS_TURN_ON);
			release;
		}

		if (!AccelOn) {
			accel_trigger (ACCRQ_ON);
			delay (256, AS_TURN_ON);
			release;
		}

	state AS_RUNNING:

		if (!AccelOn || AccelUntil == 0)
			// We are not needed any more
			finish;

		if (AccelUntil < seconds ()) {
			// Must be switched off
			accel_trigger (ACCRQ_OFF);
			delay (256, AS_RUNNING);
			release;
		}

		delay (1024, AS_RUNNING);
}

Boolean accel_start (Boolean on, lword after, lword duration, byte pack,
								word interval) {
	AccelWhen  = after + seconds ();
	AccelUntil = duration ? AccelWhen + duration : 0;
	AccelInterval = pack ? interval : 0;
	AccelPack = pack <= MAX_ACC_PACK ? pack : MAX_ACC_PACK;
	AccelFill = 0;
	AccelSernum = 0;
	killall (accel_starter);
	return runfsm accel_starter (on) != 0;
}

static void accpms (lword res [2]) {
//
// Transforms accelerometer's timing parameters into values to be sent to OSS
//
	lword sec = seconds ();

	if (AccelWhen < sec) {
		res [0] = 0;
		res [1] = AccelUntil < sec ? 0 : AccelUntil - sec;
	} else {
		res [0] = AccelWhen - sec;
		res [1] = AccelUntil < AccelWhen ? 0 : AccelUntil - AccelWhen;
	}
}

// ============================================================================

fsm press_reader {

	state PR_LOOP:

		if (!PressRequest && Item != ITEM_PRESS) {
			// No SetMode (yet)
			finish;
		}

	state PR_READ:

		address msg;
		message_presst_t *pmt;
		bmp085_data_t val;

		read_sensor (PR_READ, SENSOR_PRESSTEMP, (address)(&val));

		if (PressRequest) {
			// Send it out
			if ((msg = new_msg (message_presst_code,
			    sizeof (message_presst_t))) == NULL) {
				delay (256, PR_READ);
				release;
			}
			PressRequest = NO;
			pmt = (message_presst_t*) osspar (msg);
			pmt->press = val.press;
			pmt->temp = val.temp;
			tcv_endp (msg);
		}

		if (Item == ITEM_PRESS)
			press_update_display (val.press, val.temp);

		delay (5 * 1024, PR_LOOP);
		when (&PressRequest, PR_LOOP);
}

Boolean press_trigger (byte c) {

	if (!running (press_reader) && !runfsm press_reader)
		return NO;

	trigger (&PressRequest);

	PressRequest = c;
	return YES;
}

// ============================================================================

fsm status_sender {

	state WAIT_BATTERY:

		word batt;
		address msg;
		message_status_t *pmt;

		read_sensor (WAIT_BATTERY, SENSOR_BATTERY, &batt);

		if ((msg = new_msg (message_status_code,
		    sizeof (message_status_t))) == NULL) {
			delay (256, WAIT_BATTERY);
			release;
		}

		pmt = (message_status_t*) osspar (msg);
		pmt->uptime = seconds ();

		accpms (&(pmt->after));

		pmt->accstat = AccelOn;
		pmt->display = LCDOn;
		pmt->delay = RadioDelay;
		rtc_get (&RTC);
		pmt->time [0] = RTC.year;
		pmt->time [1] = RTC.month;
		pmt->time [2] = RTC.day;
		pmt->time [3] = RTC.hour;
		pmt->time [4] = RTC.minute;
		pmt->time [5] = RTC.second;
		pmt->freemem = memfree (0, &(pmt->minmem));
		pmt->battery = batt;

		tcv_endp (msg);
		finish;
}

static void msg_status () {

	if (running (status_sender)) {
		oss_ack (ACK_BUSY);
		return;
	}

	if (!runfsm status_sender)
		oss_ack (ACK_NORES);
}

static void msg_accsts () {

	address msg;
	message_accstats_t *pmt;

	if ((msg = new_msg (message_accstats_code,
	    sizeof (message_accstats_t))) == NULL)
		return;

	pmt = (message_accstats_t*) osspar (msg);

	accpms (&(pmt->after));

	pmt->nevents = AccelEvents;
	pmt->total = AccelTotal;
	pmt->max = AccelMax;
	pmt->last = AccelLast;
	pmt->on = AccelOn;

	tcv_endp (msg);
}

static void msg_acccnf () {

	address msg;
	message_accregs_t *pmt;

	if ((msg = new_msg (message_accregs_code, sizeof (word) +
	    sizeof (AccelRegs))) == NULL)
		return;

	pmt = (message_accregs_t*) osspar (msg);
	pmt->regs.size = sizeof (AccelRegs);
	memcpy (pmt->regs.content, &AccelRegs, sizeof (AccelRegs));

	tcv_endp (msg);
}

// ============================================================================

static void setclock (command_time_t *pmt) {

	RTC.year   = pmt->time [0];
	RTC.month  = pmt->time [1];
	RTC.day    = pmt->time [2];
	RTC.hour   = pmt->time [3];
	RTC.minute = pmt->time [4];
	RTC.second = pmt->time [5];
	rtc_set (&RTC);
	// Make sure to update the LCD copy
	if (Item == ITEM_WATCH) {
		WatchBeingSet = BNONE;
		SetMode = NO;
		display_item (ITEM_WATCH);
	}
}

static void handle_rf_command (byte code, address par, word pml) {

	word u;

	switch (code) {

		case command_acconfig_code:

			// Accelerator configuration
			if (pml < 2) {
				oss_ack (ACK_FMT);
				return;
			}

#define pmt ((command_acconfig_t*)par)

			if ((u = pmt->regs.size) > pml - 2 ||
			     u > sizeof (AccelRegs)) {
BadLength:
				oss_ack (ACK_LENGTH);
				return;
			}

			memcpy (&AccelRegs, &(pmt->regs.content), u);

			if (AccelOn) {
				if (accel_trigger (ACCRQ_ON)) {
					oss_ack (ACK_NORES);
					return;
				}
			}
OK:
			oss_ack (ACK_OK);
			return;
#undef	pmt
		case command_accturn_code:

			// Switch the accelerometer on or off
			if (pml < sizeof (command_accturn_t))
				goto BadLength;

#define	pmt ((command_accturn_t*)par)

			if (pmt->what & 0x04) {
				// Erase
				accel_clear ();
				pmt->what &= 0x03;
			}

			if (pmt->what) {
				if (!accel_start (pmt->what == 2,
				    pmt->after,
				    pmt->duration,
				    pmt->pack,
				    pmt->interval)) {
					oss_ack (ACK_NORES);
					return;
				}
			}
				
			goto OK;
#undef	pmt
		case command_time_code:

			if (pml < sizeof (command_time_t))
				goto BadLength;

			setclock ((command_time_t*)par);

			goto OK;

		case command_radio_code:

			if (pml < sizeof (command_radio_t))
				goto BadLength;

#define	pmt ((command_radio_t*)par)

			if (pmt->delay == 0) {
				switch_radio (0);
			} else {
				RadioDelay = pmt->delay;
				switch_radio (1);
			}
			goto OK;
#undef	pmt
		case command_display_code:

			if (pml < sizeof (command_display_t))
				goto BadLength;

#define	pmt ((command_display_t*)par)

			switch_lcd (pmt->what != 0);
			goto OK;
#undef	pmt

		case command_getinfo_code:

			if (pml < sizeof (command_getinfo_t))
				goto BadLength;

#define	pmt ((command_getinfo_t*)par)

			switch (pmt->what) {

				case 0: msg_status (); return;
				case 1:
					if (!AccelOn)
						oss_ack (ACK_ISOFF);
					else
						accel_trigger (ACCRQ_VALUE);
					return;
				case 2: msg_accsts (); return;
				case 3: msg_acccnf (); return;
				case 4:
					if (!press_trigger (1))
						oss_ack (ACK_NORES);
					return;
			}

			oss_ack (ACK_PARAM);
			return;

		case command_collect_code:

			if (pml < sizeof (command_time_t))
				goto BadLength;

			setclock ((command_time_t*)par);

			// Set the registers to a nice foolproof config
			memcpy (&CollectRegs, &AccelRegs, sizeof (AccelRegs));

			// Force on
			if (!accel_start (YES, 0, 0, MAX_ACC_PACK, 4)) {
				oss_ack (ACK_NORES);
				return;
			}

			goto OK;

	}

	oss_ack (ACK_COMMAND);
}

fsm radio_receiver {

	state RS_LOOP:

		address pkt;
		oss_hdr_t *osh;

		pkt = tcv_rnp (RS_LOOP, RFC);

		if (tcv_left (pkt) >= OSSMINPL && pkt [1] == NODE_ID &&
		    (osh = osshdr (pkt)) -> ref != LastRef) {

			LastRef = osh -> ref;

			handle_rf_command (
				osh->code,
				osspar (pkt),
				tcv_left (pkt) - OSSFRAME
			);
		}

		tcv_endp (pkt);
		sameas RS_LOOP;
}

// ============================================================================

static void button_press (Boolean lng) {
//
// ============================================================================
//
// Display units:
//
//	Time, Radio, Accel, Temp/Press
//
//	LB 	switches among the units
//	RB, RT	switch among the options of the unit
//	LT	(long push) selects "setting" for the unit
//
//	In setting mode:
//
//	LB	selects what to set
//	RB, RT	go through the values
//
// Time, options = sec, calendar
//
// Radio, shows status, no options
//
// Accel:
//
//	Shows number of events (top) and (options) last G, max G (bottom)
//	Setting mode: on, off, reset, yes, no; 
//
	if (TheItem == NULL) {
		// Ignore everything, except for long LB
		if (TheButton == BUTTON_LB && lng)
			switch_lcd (1);
		return;
	}

	if (SetMode) {
		if (TheButton == BUTTON_LT) {
			// This ends the set mode
			if (TheItem->fn_exit_set_mode != NULL)
				TheItem->fn_exit_set_mode ();
			SetMode = NO;
			return;
		}
		if (TheButton == BUTTON_LB) {
			// Next thing to set
			if (TheItem->fn_set_next != NULL)
				TheItem->fn_set_next ();
			return;
		}
		if (TheItem->fn_set_value != NULL)
			TheItem->fn_set_value (TheButton == BUTTON_RT ? 1 : -1);
		return;
	}

	if (TheButton == BUTTON_LB) {
		if (lng)
			// Switch the display off
			switch_lcd (0);
		else
			next_item ();
		return;
	}

	if (TheButton == BUTTON_RT || TheButton == BUTTON_RB) {
		if (TheItem->fn_option != NULL)
			TheItem->fn_option (TheButton == BUTTON_RT ? 1 : -1);
		return;
	}

	if (lng) {
		// Long LT
		if (TheItem->fn_enter_set_mode != NULL && 
		    TheItem->fn_enter_set_mode ())
			SetMode = YES;
	}
}
		
fsm root {

	word depcnt;

	state RS_INIT:

		word sid;

		ezlcd_init ();

		LCDOn = 1;
		ezlcd_on ();

		powerdown ();

		phys_cc1100 (0, MAX_PACKET_LENGTH);
		tcv_plug (0, &plug_null);

		RFC = tcv_open (NONE, 0, 0);
		sid = NETID;
		tcv_control (RFC, PHYSOPT_SETSID, &sid);

		// Initialize
		watch_start ();
		display_item (ITEM_WATCH);

		runfsm radio_receiver;

#ifdef RADIO_INITIALLY_ON
		switch_radio (1);
#endif

#ifdef DISPLAY_INITIALLY_OFF

		delay (1024, RS_DOFF);
		release;

	state RS_DOFF:

		switch_lcd (0);
#endif
		// Assume the role of button handler

	state RS_BUTTON_LOOP:

		wait_sensor (SENSOR_BUTTONS, RS_PRESSED);
		release;

	state RS_PRESSED:

		word butt;

		// Find out which button; we only care about one at a time
		// in the priority order LT, LB, RT, RB
		read_sensor (RS_PRESSED, SENSOR_BUTTONS, &butt);

		for (TheButton = 0; TheButton < N_BUTTONS; TheButton++)
			if ((butt >> TheButton) & 1)
				break;

		if (TheButton == N_BUTTONS)
			// Ignore
			sameas RS_BUTTON_LOOP;

		// Detecting long presses
		depcnt = LONG_PRESS_TIME;
DnW:
		delay (DEPRESS_CHECK_INTERVAL, RS_DEPRESS);
		release;

	state RS_DEPRESS:

		word butt;

		read_sensor (RS_DEPRESS, SENSOR_BUTTONS, &butt);

		if ((butt >> TheButton) & 1) {
			// Still pressed
			if (depcnt) {
				if (--depcnt == 0)
					// Long press
					button_press (YES);
			}
			// Wait until depressed
			goto DnW;
		}

		if (depcnt)
			// Short press, signalled at de-press
			button_press (NO);

		sameas RS_BUTTON_LOOP;
}

#ifdef DEBUGGING

void rms (word code, word c, word v) {

	address pkt;

	if ((pkt = tcv_wnp (WNONE, RFC, 12)) == NULL)
		return;

	pkt [1] = NODE_ID;
	pkt [2] = 0xFF | (code << 8);
	pkt [3] = c;
	pkt [4] = v;

	tcv_endp (pkt);
}

#endif

// ============================================================================
// ============================================================================

