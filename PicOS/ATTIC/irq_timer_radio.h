#ifndef __pg_irq_timer_radio_h
#define __pg_irq_timer_radio_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/*
 * This is legacy stuff, which most likely will never be used again
 */

#if	RADIO_TYPE != RADIO_XEMICS

		if (rcvhigh && !(zzz_radiostat & 1)) {
			if (zzr_xwait
#if	RADIO_INTERRUPTS > 1
			    && zzz_last_sense < RADIO_INTERRUPTS
#endif
				/* Receiver waiting */
				zzr_xwait -> Status = zzr_xstate << 4;
				zzr_xwait = NULL;
				zzz_last_sense = 0;
				/* Wake up the scheduler */
				RISE_N_SHINE;
				RTNI;
			}
			zzz_last_sense = 0;
		} else {
			if (zzz_last_sense != MAX_INT)
				zzz_last_sense++;
		}
#else	/* XEMICS */
		if (zzz_last_sense != MAX_INT)
			zzz_last_sense++;
#endif	/* XEMICS */


#endif
