/* ========================================================================= */
/* Copyright (C) Olsonet Communications, 2013                                */
/* All rights reserved.                                                      */
/* ========================================================================= */

// ============================================================================
// P1.0	BUTTON, on low, BROKEN: LED on high
// P1.1 NC
// P1.2 NC
// P1.3 NC
// P1.4 INT2 of BMA250
// P1.5 UARTRX (normally unused)
// P1.6 UARTTX (normally unused)
// P1.7 INT1 of BMA250
#define	PIN_DEFAULT_P1SEL	0x60

#ifdef AP320_BROKEN
#define	PIN_DEFAULT_P1DIR	0x4F
#define	PIN_DEFAULT_P1REN	0x90
#define	PIN_DEFAULT_P1OUT	0x00
#else
#define	PIN_DEFAULT_P1DIR	0x4E
// Pull down the two interrupt pins in case BMA250 is absent, also the button
// pin needs a pullup
#define	PIN_DEFAULT_P1REN	0x91
#define	PIN_DEFAULT_P1OUT	0x01
#endif

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
// P3.7 NC
#define	PIN_DEFAULT_P3SEL		0x00
#define	PIN_DEFAULT_P3DIR		0xFF
#define	PIN_DEFAULT_P3OUT		0x00
// ============================================================================
// P4.0 NC
// P4.1 NC
// P4.2 NC
// P4.3 NC
// P4.4 Extra LED (connector, active low), BROKEN: PANIC BUTTON (active high,
// needs pulldown)
// P4.5 - LED (R) (active low)
// P4.6 - LED (G) (active low)
// P4.7 - LED (B) (active low)
#define	PIN_DEFAULT_P4SEL		0x00
#define	PIN_DEFAULT_P4OUT		0xE0

#ifdef AP320_BROKEN
#define	PIN_DEFAULT_P4DIR		0xEF
#define	PIN_DEFAULT_P4REN		0x10
#else
#define	PIN_DEFAULT_P4DIR		0xFF
#endif

// ============================================================================
// P5.0 NC (no crystal)
// P5.1 NC
// P5.2 BMA250 CSB
// P5.3 NC
// P5.4 NC
// P5.5 BMA250 SCx
// P5.6 BMA250 SDx
// P5.7 BMA250 SDO
// II OO OO OO OO OO OO OO
//       HI       HI
#define	PIN_DEFAULT_P5SEL	0x00
#define	PIN_DEFAULT_P5DIR	0x7F
#define	PIN_DEFAULT_P5OUT	0x24
// ============================================================================

#ifdef AP320_BROKEN
// Temporary button
#define	button_pressed		(P4IN & 0x10)
#else
// Normal button on P1.0
#define	BUTTON_LIST 		{ BUTTON_DEF (1, 0x01, 0) }
#define	BUTTON_PIN_P1_IRQ	0x01
#define	BUTTON_DEBOUNCE_DELAY	32
#define	BUTTON_PRESSED_LOW	1
#define	BUTTON_PANIC	0
#endif

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

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,			\
		INTERNAL_VOLTAGE_SENSOR,			\
		DIGITAL_SENSOR (0, NULL, bma250_read)		\
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
