/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "buttons.h"

static void (*baction) (word) = NULL;
static word pbutton;
const word button_list [] = BUTTON_LIST;

#define	BU_WAIT		0
#define	BU_OFF		1
#define	BU_LOOP		2

#ifndef	BUTTON_DEBOUNCE_DELAY
#define	BUTTON_DEBOUNCE_DELAY	64
#endif

#ifndef	BUTTON_REPEAT_DELAY
#define	BUTTON_REPEAT_DELAY	750
#endif

#ifndef	BUTTON_REPEAT_INTERVAL
#define	BUTTON_REPEAT_INTERVAL	256
#endif

thread (zz_buttons)

  entry (BU_WAIT)

	if (baction == NULL) {
Term:
		// This means we are not needed
		finish;
	}

	// Find the button
	for (pbutton = 0; pbutton < sizeof (button_list) / sizeof (word); 
	    pbutton++) {
		// This is the priority order, in case more than one button
		// is pressed at the same time
		if (button_pressed (button_list [pbutton])) {
			// Do the action
			(*baction) (pbutton);
			// Basic debounce delay: wait for this much before
			// looking again at any button
Debounce:
			delay (BUTTON_DEBOUNCE_DELAY, BU_OFF);
			release;
		}
	}

	// Not found
Done:
	when (BUTTON_PRESSED_EVENT, BU_WAIT);
	buttons_enable ();
	release;
		
  entry (BU_OFF)

	if (!button_still_pressed (button_list [pbutton]))
		goto Done;

	if (BUTTON_REPEAT (button_list [pbutton])) {
		// Wait for repeat
		delay (BUTTON_REPEAT_DELAY, BU_LOOP);
		release;
	} else
		goto Debounce;

  entry (BU_LOOP)

	if (!button_still_pressed (button_list [pbutton]))
		goto Done;

	if (baction == NULL)
		goto Term;

	(*baction) (pbutton);

	delay (BUTTON_REPEAT_INTERVAL, BU_LOOP);

endthread

// ============================================================================

void buttons_action (void (*act)(word)) {
/*
 * Initialize button action
 */
	if ((baction = act) == NULL) {
		killall (zz_buttons);
		return;
	}

	if (!running (zz_buttons)) {
		buttons_init ();
		runthread (zz_buttons);
	}
}
