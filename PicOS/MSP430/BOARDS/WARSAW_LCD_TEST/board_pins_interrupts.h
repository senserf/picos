/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifdef	P1_INTERRUPT_SERVICE

#if CC1100
#include "irq_cc1100.h"
#endif

if (P1IFG & P1_PINS_INTERRUPT_MASK) {
	_BIC (P1IFG, P1_PINS_INTERRUPT_MASK);
	trigger (BUTTON_PRESSED_EVENT);
}

#endif

#ifdef	P2_INTERRUPT_SERVICE

if (P2IFG & P2_PINS_INTERRUPT_MASK) {
	_BIC (P2IFG, P2_PINS_INTERRUPT_MASK);
	trigger (BUTTON_PRESSED_EVENT);
}

#endif
