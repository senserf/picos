/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//
// Sensor connections:
//
//	SHT =   P2.5, 2.3, 2.4, 6.7, 2.2, 1.6, 6.6, 6.5, 6.4 6.3 Data
//		P1.7 Clock
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
#define	PIN_DEFAULT_P6SEL	0x00

#define	PIN_MAX 		0
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0

// SHT array
#include "sht_xx_a.h"
#include "analog_sensor.h"
#include "sensors.h"

// Internal voltage sensor
#define	SEN_POWER_PIN		INCH_VCC2
#define	SEN_POWER_SHT		1
#define SEN_POWER_ISI		0
#define	SEN_POWER_NSA		16
#define SEN_POWER_URE		SREF_VREF_AVSS
#define	SEN_POWER_ERE		(REFON + REF2_5V)

#define	SENSOR_LIST { \
		ANALOG_SENSOR ( SEN_POWER_ISI,  \
				SEN_POWER_NSA,  \
				SEN_POWER_PIN,  \
				SEN_POWER_URE,  \
				SEN_POWER_SHT,  \
				SEN_POWER_ERE), \
		DIGITAL_SENSOR (0, shtxx_a_init, shtxx_a_temp), \
		DIGITAL_SENSOR (0, NULL, shtxx_a_humid) \
	}

// Pin definitions for the SHT sensor

// No need to initialize - just make sure the defaults above are right
#define	shtxx_a_ini_regs	CNOP

// The number of SHT sensors in the array
#define	SHTXX_A_SIZE	10

// P1.7 is the shared clock pin
#define	shtxx_a_ckup	_BIS (P1OUT, 0x80)
#define	shtxx_a_ckdown	_BIC (P1OUT, 0x80)

#define	shtxx_a_data(a) \
		a = 0; \
		if (P2IN & 0x20) a |= 0x0001; \
		if (P2IN & 0x08) a |= 0x0002; \
		if (P2IN & 0x10) a |= 0x0004; \
		if (P6IN & 0x80) a |= 0x0008; \
		if (P2IN & 0x04) a |= 0x0010; \
		if (P1IN & 0x40) a |= 0x0020; \
		if (P6IN & 0x40) a |= 0x0040; \
		if (P6IN & 0x20) a |= 0x0080; \
		if (P6IN & 0x10) a |= 0x0100; \
		if (P6IN & 0x08) a |= 0x0200

#define	shtxx_a_du	\
		_BIC (P2DIR, 0x3C); \
		_BIC (P1DIR, 0x40); \
		_BIC (P6DIR, 0xF8)

#define	shtxx_a_dd	\
		_BIS (P2DIR, 0x3C); \
		_BIS (P1DIR, 0x40); \
		_BIS (P6DIR, 0xF8)

#define	SENSOR_INITIALIZERS	// To make sure global init function gets called
#define	SENSOR_ANALOG		// To make sure analog sensors are processed
#define	SENSOR_DIGITAL		// To make sure digital sensors are processed
