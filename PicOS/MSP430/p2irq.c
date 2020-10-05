/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "kernel.h"
#include "pins.h"
/*
 * This code handles all interrupts triggered by P2 pins
 */

interrupt (PORT2_VECTOR) p2irq () {

#define	P2_INTERRUPT_SERVICE
// ============================================================================

// Any board-specific requests for P2
#include "board_pins_interrupts.h"

// P2 pins of the pin sensor
#ifdef INPUT_PIN_P2_IRQ
#define	pin_sensor_int (P2IFG & INPUT_PIN_P2_IRQ)
#include "irq_pin_sensor.h"
#undef	pin_sensor_int
#endif

// P2 pins of buttons
#ifdef BUTTON_PIN_P2_IRQ
#define	buttons_int (P2IFG & BUTTON_PIN_P2_IRQ)
#include "irq_buttons.h"
#undef	buttons_int
#endif

// ============================================================================
#undef	P2_INTERRUPT_SERVICE

	RTNI;
}
