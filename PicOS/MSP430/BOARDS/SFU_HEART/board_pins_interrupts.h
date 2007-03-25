/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifdef	P1_INTERRUPT_SERVICE

#include "irq_cc1100.h"

if (heart_rate_counter_int) {

	HeartRateIntervals [HRINext] = HRTimer;
	HRTimer = 0xffff;

	if (HRINext == 0)
		HRINext = N_HEART_RATE_INTERVALS - 1;
	else
		HRINext--;

	clear_heart_rate_counter_int;
}

#endif
