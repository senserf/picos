/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
{
	// Set the return context for release
	SET_RELEASE_POINT;
Redo:

#ifdef	MONITOR_PIN_SCHED
	_PVS (MONITOR_PIN_SCHED, 1);
#endif
	// Catch up with time
	update_n_wake (MAX_WORD, NO);

	// Run the first ready process
	for_all_tasks (__pi_curr) {
		if (__pi_curr->code != NULL && !waiting (__pi_curr)) {
			// Entry used and process ready
			(__pi_curr->code) (tstate (__pi_curr));
			goto Redo;
		}
	}

	// No process is ready

#ifdef	MONITOR_PIN_SCHED
	_PVS (MONITOR_PIN_SCHED, 0);
#endif

#if SPIN_WHEN_HALTED
	// Keep spinning the CPU (this exotic feature was requested for GENESIS)
#if ENTROPY_COLLECTION
	entropy++;
#endif

#else	/* NOT SPIN_WHEN_HALTED */

	__SLEEP;

#endif	/* SPIN_WHEN_HALTED */

	goto Redo;
}
