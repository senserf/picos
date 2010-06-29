/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "pins.h"
/*
 * This code handles all interrupts triggered by P2 pins
 */

interrupt (PORT2_VECTOR) p2irq () {

#define	P2_INTERRUPT_SERVICE
#include "board_pins_interrupts.h"
#undef	P2_INTERRUPT_SERVICE

	RTNI;
}
