/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "pins.h"
/*
 * This code handles all interrupts triggered by P1 pins
 */

interrupt (PORT1_VECTOR) p1irq () {

#define	P1_INTERRUPT_SERVICE
#include "board_pins_interrupts.h"
#undef	P1_INTERRUPT_SERVICE

	RTNI;
}
