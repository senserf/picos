/* ========================================================================= */
/* Copyright (C) Olsonet Communications, 2013                                */
/* All rights reserved.                                                      */
/* ========================================================================= */

// P1.5 = UARTRX, P1.6 = UARTTX, P1.7 = BUTTON (presed low, requires pullup)
// P1.0 = LED (on high)


// ============================================================================
// P1.0	panic button, pulled down when pressed, internal pullup required
// P1.1 pin 3 of J2
// P1.2 pin 4 of J2
// P1.3 pin 5 of J2
// P1.4 INT2 of BMA250
// P1.5 UARTRX
// P1.6 UARTTX
// P1.7 INT1 of BMA250
// II SO SI II OO OO OO II
#define	PIN_DEFAULT_P1SEL	0x60
#define	PIN_DEFAULT_P1DIR	0x4E
#define	PIN_DEFAULT_P1OUT	0x01
#define	PIN_DEFAULT_P1REN	0x01
// ============================================================================

// ============================================================================
// P2.0 pin 6 of J2
// P2.1 soft reset button externally pulled up with low resistance
// P2.2 pin 7 of J2
// P2.3 pin 8 of J2
// P2.4 pin 9 of J2
// P2.5 VREF+/VEREF+ for ADC converter
// P2.6 pin 10 of J2
// P2.7 pin 11 of J2
// OO OO OO OO OO OO II OO
// Note: for now, we disable the ADC function on the P2 pins available on the
// connector (J2); these functions can be assumed whenever needed
#define	PIN_DEFAULT_P2SEL		0x00
#define	PIN_DEFAULT_P2DIR		0xFD
#define	SOFT_RESET_BUTTON_PRESSED	((P2IN & 0x02) == 0)
// ============================================================================

// ============================================================================
// P3.0 pin 12 of J2
// P3.1 pin 13 of J2
// P3.2 pin 14 of J2
// P3.3 - P3.7 NC
// No need to change the defaults for now
// ============================================================================

// ============================================================================
// P4.0 - P4.4 NC
// P4.5 - LED (R) lit when pulled down
// P4.6 - LED (G)
// P4.7 - LED (B)
// II II II OO OO OO OO OO
#define	PIN_DEFAULT_P4DIR	0x1F
// ============================================================================

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
#define	PIN_DEFAULT_P5SEL	0x03
#define	PIN_DEFAULT_P5DIR	0x7C
#define	PIN_DEFAULT_P5OUT	0x24
// ============================================================================

#define	BUTTON_LIST 		{ BUTTON_DEF (1, 0x01, 0) }
#define	BUTTON_PIN_P1_IRQ	0x01
#define	BUTTON_DEBOUNCE_DELAY	64
#define	BUTTON_PRESSED_LOW	1

#define	BUTTON_PANIC	0

#if 1

// Make the connector pins available
#define	PIN_LIST	{	\
	PIN_DEF (P2, 0),	\
	PIN_DEF (P2, 2),	\
	PIN_DEF (P2, 3),	\
	PIN_DEF (P2, 4),	\
	PIN_DEF (P2, 6),	\
	PIN_DEF (P2, 7),	\
	PIN_DEF (P1, 1),	\
	PIN_DEF (P1, 2),	\
	PIN_DEF (P1, 3),	\
	PIN_DEF (P3, 0),	\
	PIN_DEF (P3, 1),	\
	PIN_DEF (P3, 2)		\
}

#define	PIN_MAX		12
#define	PIN_MAX_ANALOG	6

#else

#define	PIN_MAX		0
#define	PIN_MAX_ANALOG	0

#endif

#define	PIN_DAC_PINS	0x00

// ============================================================================

#include "board_rtc.h"
#include "bma250.h"
#include "analog_sensor.h"
#include "sensors.h"

// ============================================================================

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
// so 0 looks like a safe bet
#define	bma250_delay	udelay (40)

// ============================================================================

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,			\
		INTERNAL_VOLTAGE_SENSOR,			\
		DIGITAL_SENSOR (0, bma250_init, bma250_read)	\
	}

#define	SENSOR_ANALOG
#define	SENSOR_DIGITAL
#define	SENSOR_EVENTS
#define	SENSOR_INITIALIZERS

#define	SENSOR_MOTION		0

#define	N_HIDDEN_SENSORS	2

// ============================================================================

#define	EXTRA_INITIALIZERS	__pi_rtc_init ()
