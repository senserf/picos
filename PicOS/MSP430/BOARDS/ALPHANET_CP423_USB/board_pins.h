/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// ============================================================================
// P1.0	- P1.4 NC (make them input with pulldowns)
// P1.5 UARTRX
// P1.6 UARTTX
// P1.7 - NC
#define	PIN_DEFAULT_P1SEL	0x60
#define	PIN_DEFAULT_P1DIR	0x40
#define	PIN_DEFAULT_P1OUT	0x00
#define	PIN_DEFAULE_P1REN	0x9F
// ============================================================================
// P2.0 - P2.7 assume NC (input with pulldowns)
#define	PIN_DEFAULT_P2SEL	0x00
#define	PIN_DEFAULT_P2DIR	0x00
#define	PIN_DEFAULT_P2OUT	0x00
#define	PIN_DEFAULT_P2REN	0xFF
// ============================================================================
// P3.0, P3.4 - P3.7 NC (input with pulldown)
// P3.1 - P3.3 solder on selector to ground (input with pullup)
#define	PIN_DEFAULT_P3SEL	0x00
#define	PIN_DEFAULT_P3DIR	0x00
#define	PIN_DEFAULT_P3OUT	0x07
#define	PIN_DEFAULT_P3REN	0xFF
// ============================================================================
// P4.0, P4.2, P4.3, P4.4 grounded (input)
// P4.1 solder on selector to ground (input with pullup)
// P4.4 OUT LED (active low)
// P4.5 - LED (B) (active low)
// P4.6 NC
// P4.7 - LED (R) (active low)
#define	PIN_DEFAULT_P4SEL	0x00
#define	PIN_DEFAULT_P4DIR	0xA0
#define	PIN_DEFAULT_P4OUT	0xA2
#define	PIN_DEFAULT_P4REN	0x5F
// ============================================================================
// P5.0 - P5.7 assume NC (input with pulldowns)
#define	PIN_DEFAULT_P5SEL	0x00
#define	PIN_DEFAULT_P5DIR	0x00
#define	PIN_DEFAULT_P5OUT	0x00
#define	PIN_DEFAULT_P5REN	0xFF
// ============================================================================

#define	PIN_MAX			0
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0x00

// ============================================================================

#include "board_rtc.h"
#include "analog_sensor.h"
#include "sensors.h"

// ============================================================================

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,			\
		INTERNAL_VOLTAGE_SENSOR				\
	}


#define	SENSOR_ANALOG

#define	N_HIDDEN_SENSORS	2

// ============================================================================

#define	EXTRA_INITIALIZERS	__pi_rtc_init ()
