/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

//
// Sensor connections:
//
//	Sonar on P6.3 (analog data), P1.7 (power supply)
//

// Departures from default pre-initialization
#define	PIN_DEFAULT_P1DIR	0x80	// P1.7 == CLK for the SHT sensor
#define	PIN_DEFAULT_P2DIR	0x83	// 0,1 and 7 hang loose
#define	PIN_DEFAULT_P3DIR	0xC9

// EEPROM: 0-CS, 1-SI, 2-SO, 3-SCK
#define	PIN_DEFAULT_P5DIR	0xFB
#define	PIN_DEFAULT_P5OUT	0x09

#define	PIN_DEFAULT_P4OUT	0x0E	// LEDs off by default
#define	PIN_DEFAULT_P4DIR	0x8E	//

#define	PIN_DEFAULT_P6DIR	0x00
#define	PIN_DEFAULT_P6SEL	0x08

#define	PIN_MAX 		0
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0

#include "analog_sensor.h"
#include "sensors.h"

// 4 == 64 cycles @ 200nsec (5 MHz ADC12OSC source) == 128 uS
#define	SEN_SON_SHT		4
#define	SEN_SON_ISI		0
#define	SEN_SON_NSA		16
// Using lower reference voltage for accuracy probably makes little sense
#define	SEN_SON_REF		SREF_AVCC_AVSS
#define	SEN_SON_PIN		3

#define	SEN_GEN_SHT		4
#define	SEN_GEN_ISI		0
#define	SEN_GEN_NSA		16
#define	SEN_GEN_REF		SREF_AVCC_AVSS

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,	\
		INTERNAL_VOLTAGE_SENSOR,	\
		ANALOG_SENSOR ( SEN_SON_ISI,  \
				SEN_SON_NSA,  \
				SEN_SON_PIN,  \
				SEN_SON_REF,  \
				SEN_SON_SHT,  \
				0)  	      \
	}

#define	N_HIDDEN_SENSORS	2

#define	SENSOR_INITIALIZERS	// To make sure global init function gets called
#define	SENSOR_ANALOG		// To make sure analog sensors are processed

#define	sensor_adc_prelude(p) \
			do { \
				_BIS (P1OUT, 0x80); \
		  		mdelay (500); \
			} while (0)

#define	sensor_adc_postlude(p) _BIC (P1OUT, 0x80)

