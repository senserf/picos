/* ========================================================================= */
/* Copyright (C) Olsonet Communications, 2015                                */
/* All rights reserved.                                                      */
/* ========================================================================= */

// ============================================================================
// P1.0	BUTTON, on low
// P1.3 WAKE from loop detector
// P1.4 DATA from loop detector (schematics says IN2 BMA250, error?)
// P1.7 INT1 of BMA250
#define	PIN_DEFAULT_P1SEL		0x00
#define	PIN_DEFAULT_P1DIR		0x66
#define	PIN_DEFAULT_P1REN		0x19
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
// P5.2 BMA250 CSB
// P5.5 BMA250 SCx
// P5.6 BMA250 SDx
// P5.7 BMA250 SDO
#define	PIN_DEFAULT_P5SEL		0x00
#define	PIN_DEFAULT_P5DIR		0x7F
#define	PIN_DEFAULT_P5OUT		0x24
// ============================================================================

// Normal button on P1.0, for now ignore the WAKE "button"; we will have to
// do something, because the polarity of WAKE is 1 whereas that of the normal
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

#include "bma250.h"
#define	bma250_csel	_BIC (P5OUT, 0x04)
#define	bma250_cunsel	_BIS (P5OUT, 0x04)

#define	bma250_enable	_BIS (P1IE, 0x80)
#define	bma250_disable	_BIC (P1IE, 0x80)
#define	bma250_clear	_BIC (P1IFG, 0x80)
#define	bma250_int	(P1IFG & 0x80)

#define	bma250_clkl	_BIC (P5OUT, 0x20)
#define	bma250_clkh	_BIS (P5OUT, 0x20)

#define	bma250_outl	_BIC (P5OUT, 0x40)
#define	bma250_outh	_BIS (P5OUT, 0x40)

#define	bma250_data	(P5IN & 0x80)

// Note: this delay only applies when writing. 15us didn't work, 20us did,
// so 40 looks like a safe bet
#define	bma250_delay	udelay (40)

// ============================================================================

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,			\
		INTERNAL_VOLTAGE_SENSOR,			\
		DIGITAL_SENSOR (0, bma250_init, bma250_read)	\
	}

#define	SENSOR_MOTION		0
#define	SENSOR_DIGITAL
#define	SENSOR_EVENTS
#define	SENSOR_INITIALIZERS

// ============================================================================

#define	SENSOR_ANALOG
#define	N_HIDDEN_SENSORS	2

// ============================================================================

#define	EXTRA_INITIALIZERS	__pi_rtc_init ()
