/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2014                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

// ============================================================================
// P1.5 = UARTRX, P1.6 = UARTTX, P1.1 = BUTTON (pressed low, explicit pullup)
// P1.0 = LED (on high), P1.7 = BMA250/INT1, P1.4 = BMA250/INT2
#define	PIN_DEFAULT_P1SEL	0x60
#define	PIN_DEFAULT_P1DIR	0x4D
#define	PIN_DEFAULT_P1REN	0x90
#define	PIN_DEFAULT_P1OUT	0x00
// ============================================================================
// P3.1 BMA250 SDI
// P3.2        SDO
// P3.3        SCK
// P3.4        PS
// P3.5        CSB
#define	PIN_DEFAULT_P3SEL	0x00
#define	PIN_DEFAULT_P3DIR	0xFB
#define	PIN_DEFAULT_P3OUT	0x28

// P5.0, P5.1 == LF crystal
#define PIN_DEFAULT_P5SEL	0x03
#define	PIN_DEFAULT_P5DIR	0xFC

#define	BUTTON_LIST 	{ \
				BUTTON_DEF (1, 0x02, 0), \
			}

#define	BUTTON_M1	0

#define	BUTTON_PIN_P1_IRQ	0x02
#define	BUTTON_DEBOUNCE_DELAY	64
#define	BUTTON_PRESSED_LOW	1

#include "board_rtc.h"

// #define	EXTRA_INITIALIZERS	CNOP

// P2 pins are analog-capable
#define	PIN_LIST	{	\
	PIN_DEF	(P2, 0),	\
	PIN_DEF	(P2, 1),	\
	PIN_DEF	(P2, 2),	\
	PIN_DEF	(P2, 3),	\
	PIN_DEF	(P2, 4),	\
	PIN_DEF	(P2, 5),	\
	PIN_DEF	(P2, 6),	\
	PIN_DEF	(P2, 7),	\
	PIN_DEF	(P1, 2),	\
	PIN_DEF	(P1, 3),	\
	PIN_DEF (P3, 0),	\
	PIN_DEF (P3, 6),	\
	PIN_DEF (P3, 7)		\
}

#define	PIN_MAX		13
#define	PIN_MAX_ANALOG	8
#define	PIN_DAC_PINS	0x00

// ============================================================================

#include "analog_sensor.h"
#include "sensors.h"
#include "bma250.h"

#define	bma250_csel	_BIC (P3OUT, 0x20)
#define	bma250_cunsel	_BIS (P3OUT, 0x20)

#define	bma250_enable	_BIS (P1IE, 0x80)
#define	bma250_disable	_BIC (P1IE, 0x80)
#define	bma250_clear	_BIC (P1IFG, 0x80)
#define	bma250_int	(P1IFG & 0x80)

#define	bma250_clkl	_BIC (P3OUT, 0x08)
#define	bma250_clkh	_BIS (P3OUT, 0x08)

#define	bma250_outl	_BIC (P3OUT, 0x02)
#define	bma250_outh	_BIS (P3OUT, 0x02)

#define	bma250_data	(P3IN & 0x04)

// Note: this delay only applies when writing. 15us didn't work, 20us did,
// so 40 looks like a safe bet
#define	bma250_delay	udelay (40)

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,			\
		INTERNAL_VOLTAGE_SENSOR,			\
		DIGITAL_SENSOR (0, bma250_init, bma250_read)	\
	}

#define	SENSOR_ANALOG
#define	N_HIDDEN_SENSORS	2
#define	SENSOR_MOTION		0
#define	SENSOR_DIGITAL
#define	SENSOR_EVENTS
#define	SENSOR_INITIALIZERS

// ============================================================================

#define	EXTRA_INITIALIZERS	__pi_rtc_init ()


