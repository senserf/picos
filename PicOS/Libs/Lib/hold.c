/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

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
		if (del == 1) {
			// Make it extremely fine to catch the second boundary
			delay (1, st);
			release;
		}
		if (del == 2) {
			// Force clockup
			delay (100, st);
			release;
		}
		if (del <= 32) {
			delay (((word)del) << 10, st);
			release;
		}
		delay ((word)32 * 1024, st);
		release;
	}
}
