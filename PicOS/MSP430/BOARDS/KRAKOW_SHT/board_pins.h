/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

//
// Sensor connections:
//
//	SHT = P1.6 Data, P1.7 Clock
//

// Departures from default pre-initialization
#define	PIN_DEFAULT_P1DIR	0x80	// P1.7 == CLK for the SHT sensor
#define	PIN_DEFAULT_P2DIR	0x83	// 0,1 and 7 hang loose, 2-6 nc
#define	PIN_DEFAULT_P3DIR	0xC9

// EEPROM: 0-CS, 1-SI, 2-SO, 3-SCK
#define	PIN_DEFAULT_P5DIR	0xFB
#define	PIN_DEFAULT_P5OUT	0x09

#define	PIN_DEFAULT_P4OUT	0x0E	// LEDs off by default
#define	PIN_DEFAULT_P4DIR	0x8E	//
#define	PIN_DEFAULT_P6DIR	0x00
#define PIN_DEFAULT_P6SEL	0xB9	// Five sensors

#define	PIN_MAX 		0
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0

#include "sht_xx.h"
#include "analog_sensor.h"
#include "sensors.h"

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,	\
		INTERNAL_VOLTAGE_SENSOR,	\
		DIGITAL_SENSOR (0, shtxx_init, shtxx_temp), \
		DIGITAL_SENSOR (0, NULL, shtxx_humid) \
	}

#define	N_HIDDEN_SENSORS	2

// Pin definitions for the SHT sensor

// No need to initialize - just make sure the defaults above are right
#define	shtxx_ini_regs	CNOP
#define	shtxx_dtup	_BIC (P1DIR, 0x40)
#define	shtxx_dtdown	_BIS (P1DIR, 0x40)
#define	shtxx_dtin	_BIC (P1DIR, 0x40)
#define	shtxx_dtout	do { } while (0)
#define	shtxx_data	(P1IN & 0x40)

#define	shtxx_ckup	_BIS (P1OUT, 0x80)
#define	shtxx_ckdown	_BIC (P1OUT, 0x80)

#define	SENSOR_INITIALIZERS	// To make sure global init function gets called
#define	SENSOR_ANALOG		// To make sure analog sensors are processed
#define	SENSOR_DIGITAL		// To make sure digital sensors are processed
