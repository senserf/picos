/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
{
#if SCHED_PRIO
	static int       maxprio;
	static pcb_t	*maxpriocurrent;
#endif

	zz_systat.evntpn = 0;

	/* Set the return context for release */
	SET_RELEASE_POINT;
Redo:
	/* Timer service */
	zzz_tservice ();

#if SCHED_PRIO

	maxprio = MAX_PRIO;

	for_all_tasks (zz_curr) {
		if (zz_curr->code == NULL)
			// PCB unused
			continue;
		if (!waiting (zz_curr)) {
		        if (zz_curr->prio < maxprio) {
			  	maxprio = zz_curr->prio;
			  	maxpriocurrent = zz_curr;
			}
		}
		if (twaiting (zz_curr) && zz_curr->Timer == 0) {
			zz_curr->Status &= 0xfff0;
			if (zz_curr->prio < maxprio) {
				maxprio = zz_curr->prio;
				maxpriocurrent = zz_curr;
			}
		}
	}

	if (maxprio != MAX_PRIO) {
		zz_curr = maxpriocurrent;
		(zz_curr->code) (tstate (zz_curr), zz_curr->data);
		goto Redo;
	} else {
#else
	//-----default :
	for_all_tasks (zz_curr) {
		if (zz_curr->code == NULL)
			// PCB unused
			continue;
		if (!waiting (zz_curr)) {
			(zz_curr->code) (tstate (zz_curr), zz_curr->data);
			goto Redo;
		}
		if (twaiting (zz_curr) && zz_curr->Timer == 0) {
			zz_curr->Status &= 0xfff0;
			(zz_curr->code) (tstate (zz_curr), zz_curr->data);
			goto Redo;
		}
	}

#endif
	/* No process to run */

#if SPIN_WHEN_HALTED == 0
	SLEEP;
#endif
	goto Redo;

#if SCHED_PRIO
    }
#endif
}
