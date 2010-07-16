/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifdef	P1_INTERRUPT_SERVICE

#if CC1100
#include "irq_cc1100.h"
#endif

if (P1IFG & P1_PINS_INTERRUPT_MASK) {
	buttons_disable ();
	i_trigger (BUTTON_PRESSED_EVENT);
	RISE_N_SHINE;
}

#endif

#ifdef	P2_INTERRUPT_SERVICE

if (P2IFG & P2_PINS_INTERRUPT_MASK) {
	buttons_disable ();
	i_trigger (BUTTON_PRESSED_EVENT);
	RISE_N_SHINE;
}

#endif
