/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifdef	P1_INTERRUPT_SERVICE

// Button service

if (P1IFG & P1_PINS_INTERRUPT_MASK) {
	buttons_disable ();
	i_trigger (BUTTON_PRESSED_EVENT);
	RISE_N_SHINE;
}

#endif
