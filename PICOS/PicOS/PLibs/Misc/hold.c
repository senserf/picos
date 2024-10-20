/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "hold.h"

void hold (word st, lword sec) {
/*
 * Holds the process until the wall clock reaches the specified second
 *
 *      lword del;
 *      ...
 *      ...
 *      entry (HOLD_ME);
 *        hold (HOLD_ME, del);
 *        ...
 */
	lword del;

	if ((del = seconds ()) < sec) {
		del = sec - del;
		if (del <= HOLD_STEP_DELAY_SEC) {
			// Make it as fine as it gets to catch the second
			// boundary
			delay (1, st);
			release;
		}
#ifdef	TRIPLE_CLOCK
#if	TRIPLE_CLOCK == 0
		// Not sure if this is gonna work any more with 
		// TRIPLE_CLOCK == 0, let me leave this legacy code as a
		// comment
		if (del == 2) {
			// Force clockup
			delay (100, st);
			release;
		}
#endif
#endif
		// Allow for roundups on millisecond waits, so we don't miss
		// the beginning of the target second
		del -= HOLD_STEP_DELAY_SEC;
		if (del <= HOLD_BREAK_DELAY_SEC) {
			delay (((word)del) << 10, st);
			release;
		}
		delay ((word)HOLD_BREAK_DELAY_SEC * 1024, st);
		release;
	}
}
