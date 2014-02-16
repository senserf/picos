#include "sysio.h"
#include "cc1100.h"
#include "tcvphys.h"
#include "phys_cc1100.h"
#include "plug_null.h"
#include "storage.h"

#include "butts.h"
#include "watch.h"
#include "display.h"

#include "netid.h"

// ============================================================================
// ============================================================================

sint RFC;

static void radio_off () { tcv_control (RFC, PHYSOPT_OFF, NULL); }

byte TheButton;
byte WatchBeingSet = BNONE;

// ============================================================================

static void button_press (Boolean lng) {
//
// The present idea is this:
//
//	The top line always shows the minute clock. We may want to change it
//	later (nothing fundamental). The bottom line is switched among these
//	items:
//		- second
//		- calendar
//		- temperature
//		- pressure
//		- shock
//		- radio
//
//	LB switches the roles of the bottom line
//	LT (long) will let us reset [clock and shock params]
//	RT (and RB) will go through options for reset
//	LT (short) in reset mode will exit the mode
//	
	if (WatchBeingSet != BNONE) {
		//
		// The watch is being set, so the buttons have this meaning:
		//
		//	LT - quits the setting and resumes watch + calendar
		//	LB - advances to the next thing to set
		//	RT - pushes the thing up
		//	RB - pushes the thing down
		//
		if (TheButton == BUTTON_LT) {
			WatchBeingSet = BNONE;
			display_setwatch ();
			return;
		}
		if (TheButton == BUTTON_LB) {
			// Advance to the next thing to set
			if (++WatchBeingSet > 6)
				WatchBeingSet = 0;
			display (DISPLAY_MWATCH_SET);
			display (WatchBeingSet ? DISPLAY_CWATCH_SET :
				DISPLAY_SWATCH_SET);
			return;
		}
		// The remaining two buttons are passed to the display
		display_button ();
		return;
	}

	if (TheButton == BUTTON_LB) {
		// Switch the role of the bottom display
		display ((display_content (1) == DISPLAY_SWATCH) ?
			DISPLAY_CWATCH : DISPLAY_SWATCH);
		return;
	}

	if ((TheButton == BUTTON_LT) && lng) {
		// Set the watch
		display_startset ();
		return;
	}

	// Everything else gets passed to the display
	display_button ();
}

fsm root {

	word depcnt;

	state RS_INIT:

		word sid;

		ezlcd_init ();
		ezlcd_on ();
		powerdown ();

		phys_cc1100 (0, CC1100_MAXPLEN);
		tcv_plug (0, &plug_null);

		RFC = tcv_open (NONE, 0, 0);
		sid = NETID;
		tcv_control (RFC, PHYSOPT_SETSID, &sid);

		// Initialize
		watch_start ();

		display (DISPLAY_MWATCH);
		display (DISPLAY_CWATCH);

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

void rms (word c, word v) {

	address pkt;

	if ((pkt = tcv_wnp (WNONE, RFC, 8)) == NULL)
		return;

	pkt [1] = c;
	pkt [2] = v;

	tcv_endp (pkt);
}

#endif
