/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "kernel.h"
#include "pins.h"
/*
 * This code handles all interrupts triggered by P1 pins
 */

interrupt (PORT1_VECTOR) p1irq () {

#define	P1_INTERRUPT_SERVICE
// ============================================================================

// Any board-specific requests for P1
#include "board_pins_interrupts.h"

// P1 pins of the pin sensor
#ifdef INPUT_PIN_P1_IRQ
#define	pin_sensor_int (P1IFG & INPUT_PIN_P1_IRQ)
#include "irq_pin_sensor.h"
#undef	pin_sensor_int
#endif

// P1 pins of buttons
#ifdef BUTTON_PIN_P1_IRQ
#define	buttons_int (P1IFG & BUTTON_PIN_P1_IRQ)
#include "irq_buttons.h"
#undef	buttons_int
#endif

// ============================================================================
#undef	P1_INTERRUPT_SERVICE

	RTNI;
}
