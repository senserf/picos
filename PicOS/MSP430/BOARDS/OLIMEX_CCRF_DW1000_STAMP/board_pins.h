/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2015                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifndef	DW1000_USE_SPI
#define	DW1000_USE_SPI		1
#endif

// ============================================================================
// DW1000 connected as follows:
//
//	P3.6 - RST, open drain, pulled down for reset, never pull high
//	P3.5 - SYNC, default input (open drain), emulating NC
//	P2.2 - IRQ
//	P2.3 - SPI CLK (auto pulled down)
//	P2.4 - SPI MOSI (auto pulled down, keep down after SPI operation)
//	P2.6 - SPI MISO (input)
//	P2.7 - SPI CSn, internally pulled up, pull down to activate, also
//	       wakeup if pulled for 0.5ms
// ============================================================================


// P1.5 = UARTRX, P1.6 = UARTTX, P1.1 = BUTTON (pressed low, explicit pullup)
// P1.0 = LED (on high)
#define	PIN_DEFAULT_P1SEL	0x60
#define	PIN_DEFAULT_P1DIR	0xDD
#define	PIN_DEFAULT_P1OUT	0x00

// 0, 1, 5 unused, 2 IRQ input, 3 CLK parked input low, 4 MOSI parked input low,
// 6 MISO input, 7 CS parked input low
#define	PIN_DEFAULT_P2DIR	0x23
#define	PIN_DEFAULT_P2OUT	0x00
#define	PIN_DEFAULT_P2REN	0x04

#define	PIN_DEFAULT_P3DIR	0x9F
#define	PIN_DEFAULT_P3OUT	0x00

// P5.0, P5.1 == LF crystal
#define PIN_DEFAULT_P5SEL	0x03
#define	PIN_DEFAULT_P5DIR	0xFC

// Portmapper
#define	PIN_PORTMAP	{ 	portmap_entry (P2MAP0, \
						PM_NONE, \
						PM_NONE, \
						PM_NONE, \
						PM_UCB0CLK, \
						PM_UCB0SIMO, \
						PM_NONE, \
						PM_UCB0SOMI, \
						PM_NONE) \
			}


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
	PIN_DEF	(P2, 5),	\
	PIN_DEF	(P1, 2),	\
	PIN_DEF	(P1, 3),	\
	PIN_DEF	(P1, 4),	\
	PIN_DEF	(P1, 7),	\
	PIN_DEF (P3, 0),	\
	PIN_DEF (P3, 1),	\
	PIN_DEF (P3, 2),	\
	PIN_DEF (P3, 3),	\
	PIN_DEF (P3, 4),	\
}

#define	PIN_MAX		12
#define	PIN_MAX_ANALOG	3
#define	PIN_DAC_PINS	0x00

// ============================================================================

#include "analog_sensor.h"
#include "sensors.h"
#include "actuators.h"
#include "dw1000.h"

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,			\
		INTERNAL_VOLTAGE_SENSOR,			\
		DIGITAL_SENSOR (0, NULL, dw1000_read) 		\
	}
#define	SENSOR_ANALOG
#define	SENSOR_DIGITAL
#define	SENSOR_EVENTS

#define	ACTUATOR_LIST { \
		DIGITAL_ACTUATOR (0, NULL, dw1000_write)	\
	}
#define	ACTUATOR_DIGITAL

#define	N_HIDDEN_SENSORS	2

#define	SENSOR_RANGE		0
#define	ACTUATOR_RANGE		0

// ============================================================================

