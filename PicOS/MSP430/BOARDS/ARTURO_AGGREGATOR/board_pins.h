/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//
// This is a sensor-less aggregator with an SD card on
//
//	DO	- P6.3
//	SCK	- P6.4
//	DI	- P6.6
//	CS	- P6.7
//

// Departures from default pre-initialization
#define	PIN_DEFAULT_P1DIR	0x00	// P1.7 == CLK for the SHT sensor
#define	PIN_DEFAULT_P2DIR	0x83	// 0,1 and 7 hang loose, 2-6 nc
#define	PIN_DEFAULT_P3DIR	0xC9

// EEPROM/SD: 0-CS, 1-SI, 2-SO, 3-SCK
#define	PIN_DEFAULT_P5DIR	0xFB
#define	PIN_DEFAULT_P5OUT	0x09

#define	PIN_DEFAULT_P4OUT	0x0E	// LEDs off by default
#define	PIN_DEFAULT_P4DIR	0x0E
#define	PIN_DEFAULT_P6DIR	0xD0	// For the SD card
#define	PIN_DEFAULT_P6OUT	0xD0
#define	PIN_DEFAULT_P6SEL	0x01	// P6.0 - power source sensor

#define	PIN_MAX 		0
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0

// This makes the SHT unit for ADC equal to ca. 61us. The typical power source
// is a 12V battery, which we will divide through 4.7K + 47K. With this
// impedance, a safe SHT is calculated as 20us. We shall overkill it, thrice or
// so, and sample multiple times comparing against the internal reference of
// 1.5V

#include "analog_sensor.h"
#include "sensors.h"

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,	\
		INTERNAL_VOLTAGE_SENSOR		\
	}

// Note: I have removed the external-internal voltage sensor on Pin P6.0 from
// the configuration (assuming that the internal-internal one will do)

#define	N_HIDDEN_SENSORS	2

#define	SENSOR_ANALOG
