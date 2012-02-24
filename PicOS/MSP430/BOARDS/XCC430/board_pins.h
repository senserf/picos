/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// P1.5 = UARTRX, P1.6 = UARTTX, P1.7 = BUTTON (presed low, requires pullup)
// P1.0 = LED (on high)
#define	PIN_DEFAULT_P1SEL	0x60
#define	PIN_DEFAULT_P1DIR	0x5F

// Internal pullup for P1.7
#define	PIN_DEFAULT_P1OUT	0x80
#define	PIN_DEFAULT_P1REN	0x80

// P3.6 = LED (on high); no need to change default setting for the pin

// P5.0, P5.1 == LF crystal
#define PIN_DEFAULT_P5SEL	0x03
#define	PIN_DEFAULT_P5DIR	0xFC

#define	BUTTON_LIST 	{ \
				BUTTON_DEF (1, 0x80, 0), \
			}

#define	BUTTON_M1	0

#define	P1_PINS_INTERRUPT_MASK	0x80
#define	BUTTON_DEBOUNCE_DELAY	64
#define	BUTTON_PRESSED_LOW	1

// ============================================================================

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
	PIN_DEF	(P1, 0),	\
	PIN_DEF	(P1, 1),	\
	PIN_DEF	(P1, 1),	\
	PIN_DEF	(P1, 2),	\
	PIN_DEF	(P1, 3),	\
	PIN_DEF	(P1, 4),	\
	PIN_DEF (P3, 0),	\
	PIN_DEF (P3, 1),	\
	PIN_DEF (P3, 2),	\
	PIN_DEF (P3, 3),	\
	PIN_DEF (P3, 4),	\
	PIN_DEF (P3, 5),	\
	PIN_DEF (P3, 7),	\
	PIN_DEF (P4, 0),	\
	PIN_DEF (P4, 1),	\
	PIN_DEF (P4, 2),	\
	PIN_DEF (P4, 3),	\
	PIN_DEF (P4, 4),	\
	PIN_DEF (P4, 5),	\
	PIN_DEF (P4, 6),	\
	PIN_DEF (P4, 7),	\
	PIN_DEF (P5, 2),	\
	PIN_DEF (P5, 3),	\
	PIN_DEF (P5, 4),	\
	PIN_DEF (P5, 5),	\
	PIN_DEF (P5, 6),	\
	PIN_DEF (P5, 7)		\
}

#define	PIN_MAX		35
#define	PIN_MAX_ANALOG	8
#define	PIN_DAC_PINS	0x00

// ============================================================================
#if 1

#include "analog_sensor.h"
#include "sensors.h"

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,	\
		INTERNAL_VOLTAGE_SENSOR		\
	}
#define	SENSOR_ANALOG
#define	N_HIDDEN_SENSORS	2

#endif
// ============================================================================

