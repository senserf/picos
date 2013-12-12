/* ========================================================================= */
/* Copyright (C) Olsonet Communications, 2013                                */
/* All rights reserved.                                                      */
/* ========================================================================= */

// ============================================================================
// P1.0	external LED on high
// P1.1 button
// P1.2 LED (on low)
// P1.3 LED
// P1.4 LED
// P1.5 UARTRX (normally unused)
// P1.6 UARTTX (normally unused)
// P1.7 NC
#define	PIN_DEFAULT_P1SEL	0x00
#define	PIN_DEFAULT_P1DIR	0xBD
#define	PIN_DEFAULT_P1OUT	0x1C

// ============================================================================
// P2.0 NC
// P2.1 NC
// P2.2 NC
// P2.3 NC
// P2.4 NC
// P2.5 NC
// P2.6 NC
// P2.7 NC
#define	PIN_DEFAULT_P2SEL		0x00
#define	PIN_DEFAULT_P2DIR		0xFF
#define	PIN_DEFAULT_P2OUT		0x00
// ============================================================================
// P3.0 NC
// P3.1 NC
// P3.2 NC
// P3.3 NC
// P3.4 NC
// P3.5 NC
// P3.6 NC
// P3.7 LED
#define	PIN_DEFAULT_P3SEL		0x00
#define	PIN_DEFAULT_P3DIR		0xFF
#define	PIN_DEFAULT_P3OUT		0x00
// ============================================================================

// Make sure P4 is completely disabled
#define	PIN_DEFAULT_P4SEL		(-1)

// ============================================================================
// P5.0 NC (no crystal)
// P5.1 NC
// P5.2 BMA250 CSB
// P5.3 NC
// P5.4 NC
// P5.5 BMA250 SCx
// P5.6 BMA250 SDx
// P5.7 BMA250 SDO
#define	PIN_DEFAULT_P5SEL	0x00
#define	PIN_DEFAULT_P5DIR	0xFF
#define	PIN_DEFAULT_P5OUT	0x00
// ============================================================================

// Button on P1.0
#define	BUTTON_LIST 		{ BUTTON_DEF (1, 0x02, 0) }
#define	BUTTON_PIN_P1_IRQ	0x02
#define	BUTTON_DEBOUNCE_DELAY	32
#define	BUTTON_PRESSED_LOW	1
#define	BUTTON_PANIC		0

#define	PIN_MAX		0
#define	PIN_MAX_ANALOG	0
#define	PIN_DAC_PINS	0x00

// ============================================================================

#include "board_rtc.h"
#include "analog_sensor.h"
#include "sensors.h"

// ============================================================================

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,			\
		INTERNAL_VOLTAGE_SENSOR				\
	}


// ============================================================================

#define	SENSOR_ANALOG

#define	N_HIDDEN_SENSORS	2

// ============================================================================

#define	EXTRA_INITIALIZERS	__pi_rtc_init ()
