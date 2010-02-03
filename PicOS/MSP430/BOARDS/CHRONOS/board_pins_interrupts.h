/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifdef	P1_INTERRUPT_SERVICE
#endif

#ifdef	P2_INTERRUPT_SERVICE

// Button service

if (P2IFG & P2_PINS_INTERRUPT_MASK) {
	buttons_disable ();
	i_trigger (ETYPE_USER, BUTTON_PRESSED_EVENT);
	RISE_N_SHINE;
}

#endif
