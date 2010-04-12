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
	update_n_wake (MAX_UINT);

	// Run the first ready process
	for_all_tasks (zz_curr) {
		if (zz_curr->code != NULL && !waiting (zz_curr)) {
			// Entry used and process ready
			(zz_curr->code) (tstate (zz_curr), zz_curr->data);
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

	SLEEP;

#endif	/* SPIN_WHEN_HALTED */

	goto Redo;
}
