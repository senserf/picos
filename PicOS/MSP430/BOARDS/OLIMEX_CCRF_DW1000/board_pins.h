/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2015                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// ============================================================================
// DW1000 connected as follows:
//
//	P3.0 - RST, open drain, pulled down for reset, never pull high
//	P3.1 - WAKEUP, optional, pull high for 0.5ms to wake up the chip
//	P3.2 - SPICS, internally pulled up, pull down to activate, also
//	       wakeup if pulled for 0.5ms
//	P1.2 - SP MISO (input)
//	P1.3 - SP MOSI (auto pulled down, keep down after SPI operation)
//	P1.4 - SP CLK (auto pulled down)
//	P2.2 - IRQ
// ============================================================================


// P1.5 = UARTRX, P1.6 = UARTTX, P1.1 = BUTTON (pressed low, explicit pullup)
// P1.0 = LED (on high)
#define	PIN_DEFAULT_P1SEL	0x60
// P1.3 and P1.4 are input (open drain) being pulled down by the chip
//				1 1 0 0 0 0 0 1
#define	PIN_DEFAULT_P1DIR	0xC1
#define	PIN_DEFAULT_P1OUT	0x00

// P2.2 IRQ
#define	PIN_DEFAULT_P2DIR	0xFB
#define	PIN_DEFAULT_P2OUT	0x00
#define	PIN_DEFAULT_P2REN	0x04

// P3.6 = LED (on high)		1 1 1 1 1 0 1 0
#define	PIN_DEFAULT_P3DIR	0xFA
#define	PIN_DEFAULT_P3OUT	0x00

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
	PIN_DEF	(P2, 3),	\
	PIN_DEF	(P2, 4),	\
	PIN_DEF	(P2, 5),	\
	PIN_DEF	(P2, 6),	\
	PIN_DEF	(P2, 7),	\
	PIN_DEF	(P1, 7),	\
	PIN_DEF (P3, 3),	\
	PIN_DEF (P3, 4),	\
	PIN_DEF (P3, 5),	\
	PIN_DEF (P3, 7)		\
}

#define	PIN_MAX		12
#define	PIN_MAX_ANALOG	7
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

