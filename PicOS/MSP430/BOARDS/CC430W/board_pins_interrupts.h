/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifdef	P1_INTERRUPT_SERVICE

if (__pi_cma_3000_int) {

	__pi_cma_3000_disable;
	__pi_cma_3000_clear;
	if (__pi_cma_3000_event_thread)
		ptrigger (__pi_cma_3000_event_thread,
			&__pi_cma_3000_event_thread);
		
}

#endif

#ifdef	P2_INTERRUPT_SERVICE
#endif
