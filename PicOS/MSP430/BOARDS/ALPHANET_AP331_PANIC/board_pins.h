/* ========================================================================= */
/* Copyright (C) Olsonet Communications, 2015                                */
/* All rights reserved.                                                      */
/* ========================================================================= */

// ============================================================================
// P1.0	BUTTON, on low
// P1.5 DATA from loop detector
// P1.6 WAKE from loop detector
#define	PIN_DEFAULT_P1SEL		0x00
#define	PIN_DEFAULT_P1DIR		0x9E
#define	PIN_DEFAULT_P1REN		0x61
#define	PIN_DEFAULT_P1OUT		0x01

// ============================================================================
// P2.0 LOOP AS3932	SDO
// P2.1 		SDI
// P2.2 		SCL
// P2.3 		CS
#define	PIN_DEFAULT_P2SEL		0x00
#define	PIN_DEFAULT_P2DIR		0xFE
#define	PIN_DEFAULT_P2OUT		0x00
#define	PIN_DEFAULT_P2REN		0x01
// ============================================================================
// P3.1 BUZZ
// P3.2 BUZZ		(later, need the buzzer to configure)
#define	PIN_DEFAULT_P3SEL		0x00
#define	PIN_DEFAULT_P3DIR		0xFF
#define	PIN_DEFAULT_P3OUT		0x00
// ============================================================================
// P4.4 - LED (G) (active low)
// P4.5 - LED (R) (active low)
#define	PIN_DEFAULT_P4SEL		0x00
#define	PIN_DEFAULT_P4DIR		0x30
#define	PIN_DEFAULT_P4OUT		0x30

// ============================================================================
#define	PIN_DEFAULT_P5SEL		0x00
#define	PIN_DEFAULT_P5DIR		0xFF
#define	PIN_DEFAULT_P5OUT		0x00
// ============================================================================

// Normal button on P1.0, for now ignore the WAKE "button"; we will have to
// do something, because the polarity of WAKE is 1 wherease that of the normal
// panic button is 0
#define	BUTTON_LIST 		{ BUTTON_DEF (1, 0x01, 0) }
#define	BUTTON_PIN_P1_IRQ	0x03
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
		INTERNAL_VOLTAGE_SENSOR,			\
	}

// ============================================================================

#define	SENSOR_ANALOG
#define	N_HIDDEN_SENSORS	2

// ============================================================================

#define	EXTRA_INITIALIZERS	__pi_rtc_init ()
