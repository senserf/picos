/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//
// WARSAW + SCA3100 acceleration sensor
//
//	P6.7	= Vcc
//	P2.2	= CSB
//	P2.3	= MISO
//	P2.4	= MOSI
//	P2.5	= SCK
//

#define	PIN_DEFAULT_P1DIR	0x00
#define	PIN_DEFAULT_P2DIR	0xBF	// 0,1 and 7 hang loose
#define	PIN_DEFAULT_P3DIR	0xC9

// EEPROM: 0-CS, 1-SI, 2-SO, 3-SCK
#define	PIN_DEFAULT_P5DIR	0xFB
#define	PIN_DEFAULT_P5OUT	0x09

#define	PIN_DEFAULT_P4OUT	0x0E	// LEDs off by default
#define	PIN_DEFAULT_P4DIR	0x8E	//

#define	PIN_DEFAULT_P6DIR	0x80
#define	PIN_DEFAULT_P6SEL	0x00

#define	PIN_MAX 		0
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0

// SHT array
#include "sca3100.h"
#include "analog_sensor.h"
#include "sensors.h"

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,	\
		INTERNAL_VOLTAGE_SENSOR,	\
		DIGITAL_SENSOR (0, NULL, sca3100_read) \
	}

#define	N_HIDDEN_SENSORS	2

// Pin definitions for SCA3100
#define	sca3100_bring_down	do { \
					_BIC (P6OUT, 0x80); \
					_BIC (P2OUT, 0x3C); \
					_BIS (P2DIR, 0x3C); \
				} while (0)

#define	sca3100_bring_up	do { \
					sca3100_cunsel; \
					_BIC (P2DIR, 0x08); \
					_BIS (P6OUT, 0x80); \
				} while (0)

#define	sca3100_csel		_BIC (P2OUT, 0x04)
#define	sca3100_cunsel		_BIS (P2OUT, 0x04)

#define	sca3100_clkh		_BIS (P2OUT, 0x20)
#define	sca3100_clkl		_BIC (P2OUT, 0x20)

#define	sca3100_data		(P2IN & 0x08)

#define	sca3100_outh		_BIS (P2OUT, 0x10)
#define	sca3100_outl		_BIC (P2OUT, 0x10)
					
#define	SENSOR_ANALOG		// To make sure analog sensors are processed
#define	SENSOR_DIGITAL		// To make sure digital sensors are processed
