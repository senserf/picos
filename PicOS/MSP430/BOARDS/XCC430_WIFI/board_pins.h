/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2014                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifndef	CC3000_USE_SPI
#define	CC3000_USE_SPI		0
#endif

// P1.0 = LED (on high)
// P1.1 = CC3000 IRQ	input
// P1.2 = CC3000 DOUT	input
// P1.3 = CC3000 DIN
// P1.4 = CC3000 CLK
// P1.5 = UARTRX	input
// P1.6 = UARTTX
// P1.7 = BUTTON (pressed low, requires pullup)
#define	PIN_DEFAULT_P1DIR	0x59

// Internal pullup for P1.7
#define	PIN_DEFAULT_P1OUT	0x82
#define	PIN_DEFAULT_P1REN	0x82

#if CC3000_USE_SPI		
#define	PIN_DEFAULT_P1SEL	0x7C
#else
#define	PIN_DEFAULT_P1SEL	0x60
#endif

// P3.0 = CC3000 EN
// P3.1 = CC3000 CS
// P3.6 = LED (on high)
#define	PIN_DEFAULT_P3DIR	0xFF
#define	PIN_DEFAULT_P3OUT	0x00

// P5.0, P5.1 == LF crystal
#define PIN_DEFAULT_P5SEL	0x03
#define	PIN_DEFAULT_P5DIR	0xFC

#define	BUTTON_LIST 	{ \
				BUTTON_DEF (1, 0x80, 0), \
			}

#define	BUTTON_M1	0

#define	BUTTON_PIN_P1_IRQ	0x80
#define	BUTTON_DEBOUNCE_DELAY	64
#define	BUTTON_PRESSED_LOW	1

// Current measurement on EM430F6137RF900 development board:
//
//	CPU Idle in PD mode (normal OS activity): 2.7uA at 3.3V
//	----------- PU -------------------------: 452uA
//	CPU spin                                : 2.8mA
//
// Added 100uF capacitor Vdd-GND, no noticeable increase in current

// ============================================================================

#include "board_rtc.h"
#include "board_rf.h"

#define	EXTRA_INITIALIZERS	cc3000_reginit

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

#define	PIN_MAX		27
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

