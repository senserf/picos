/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// ============================================================================
// P1.0	BUTTON EXT #1
// P1.1 BUTTON EXT #2
// P1.2 BUTTON EXT #3
// P1.3 BUTTON EXT #0	(these four have external 10K pullups)
// P1.4 NC
// P1.5 UARTRX
// P1.6 UARTTX
// P1.7 NC
#define	PIN_DEFAULT_P1SEL	0x60
#define	PIN_DEFAULT_P1DIR	0xD0
#define	PIN_DEFAULT_P1OUT	0x00
// ============================================================================
// P2.0 NC
// P2.1 NC
// P2.2 NC
// P2.3 NC
// P2.4 NC
// P2.5 NC
// P2.6 NC
// P2.7 External voltage sensor
#define	PIN_DEFAULT_P2SEL	0x80
#define	PIN_DEFAULT_P2DIR	0x7F
#define	PIN_DEFAULT_P2OUT	0x00
// ============================================================================
// P3.0 NC
// P3.1 NC
// P3.2 NC
// P3.3 NC
// P3.4 NC
// P3.5 NC
// P3.6 NC
// P3.7 NC
#define	PIN_DEFAULT_P3SEL	0x00
#define	PIN_DEFAULT_P3DIR	0xFF
#define	PIN_DEFAULT_P3OUT	0x00
// ============================================================================
// P4.0 NC
// P4.1 NC
// P4.2 NC
// P4.3 NC
// P4.4 NC
// P4.5 - LED (R) (active low)
// P4.6 - LED (G) (active low)
// P4.7 - LED (B) (active low)
#define	PIN_DEFAULT_P4SEL	0x00
#define	PIN_DEFAULT_P4DIR	0xFF
#define	PIN_DEFAULT_P4OUT	0xE0
// ============================================================================
// P5.0 Crystal
// P5.1 Crystal
// P5.2 NC
// P5.3 NC
// P5.4 NC
// P5.5 NC
// P5.6 NC
// P5.7 NC
#define	PIN_DEFAULT_P5SEL	0x03
#define	PIN_DEFAULT_P5DIR	0xFC
#define	PIN_DEFAULT_P5OUT	0x00
// ============================================================================

#define	BUTTON_LIST 		{	\
					BUTTON_DEF (1, 0x08, 0), \
					BUTTON_DEF (1, 0x01, 0), \
					BUTTON_DEF (1, 0x02, 0), \
					BUTTON_DEF (1, 0x04, 0), \
				}

#define	BUTTON_PIN_P1_IRQ	0x0F

#define	BUTTON_DEBOUNCE_DELAY	4
#define	BUTTON_PRESSED_LOW	1

// ============================================================================

#define	PIN_MAX			0
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0x00

// ============================================================================

#include "board_rtc.h"
#include "analog_sensor.h"
#include "sensors.h"

// ============================================================================

#define	VSN_INT_PIN	7
#define	VSN_INT_SHT	4
#define	VSN_INT_ISI	0
#define	VSN_INT_NSA	8
#define	VSN_INT_URE	SREF_VREF_AVSS
#define	VSN_INT_ERE	(REFON + REF2_5V)

#define	SENSOR_LIST { \
		ANALOG_SENSOR (   VSN_INT_ISI,  		\
				  VSN_INT_NSA,  		\
				  VSN_INT_PIN,  		\
				  VSN_INT_URE,  		\
				  VSN_INT_SHT,  		\
				  VSN_INT_ERE),			\
		INTERNAL_TEMPERATURE_SENSOR,			\
		INTERNAL_VOLTAGE_SENSOR				\
	}


#define	SENSOR_ANALOG

#define	N_HIDDEN_SENSORS	3

// ============================================================================

#define	EXTRA_INITIALIZERS	__pi_rtc_init ()
