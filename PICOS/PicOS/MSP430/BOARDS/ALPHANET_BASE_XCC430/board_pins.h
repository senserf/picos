/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// P1.5 = UARTRX, P1.6 = UARTTX, P1.7 = BUTTON (pressed low, requires pullup)
#define	PIN_DEFAULT_P1SEL	0x60
#define	PIN_DEFAULT_P1DIR	0x5F

// Internal pullup for P1.7
#define	PIN_DEFAULT_P1OUT	0x80
#define	PIN_DEFAULT_P1REN	0x80

// P4.5-7 LEDs, active high
#define PIN_DEFAULT_P4SEL	0x00
#define	PIN_DEFAULT_P4DIR	0xFF
#define	PIN_DEFAULT_P4OUT	0x00

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

#define	PIN_MAX		0
#define	PIN_MAX_ANALOG	0
#define	PIN_DAC_PINS	0x00

#include "board_rtc.h"
#include "analog_sensor.h"
#include "sensors.h"

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

#define	EXTRA_INITIALIZERS	__pi_rtc_init ()
