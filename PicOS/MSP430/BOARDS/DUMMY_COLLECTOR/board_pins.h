/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Departures from default pre-initialization
#define	PIN_DEFAULT_P1DIR	0xC0
#define	PIN_DEFAULT_P2DIR	0xEF	// 0,1,2 and 7 hang loose, 4 = reset
#define	PIN_DEFAULT_P3DIR	0xC9

// EEPROM: 0-CS, 1-SI, 2-SO, 3-SCK
#define	PIN_DEFAULT_P5DIR	0xFB
#define	PIN_DEFAULT_P5OUT	0x09

#define	PIN_DEFAULT_P4DIR	0xFF
#define	PIN_DEFAULT_P4OUT	0x0E	// LEDs off by default
#define	PIN_DEFAULT_P6DIR	0x00
#define	PIN_DEFAULT_P6SEL	0x17	// 5 dummy analog sensors

#define	PIN_MAX 		0
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0

#include "analog_sensor.h"
#include "sensors.h"

#define	SEN_0_PIN	0	// PAR sensor = P6.0
#define	SEN_1_PIN	1	// PAR sensor = P6.0
#define	SEN_2_PIN	2	// PAR sensor = P6.0
#define	SEN_3_PIN	3	// PAR sensor = P6.0
#define	SEN_4_PIN	4	// PAR sensor = P6.0
#define	SEN_5_PIN	5	// PAR sensor = P6.0
#define	SEN_x_SHT	4	// Sample hold time indicator
#define	SEN_x_ISI	1	// Inter sample interval indicator
#define	SEN_x_NSA	4	// Number of samples
#define	SEN_x_URE	SREF_AVCC_AVSS
#define	SEN_x_ERE	0	// Exported reference (none)

#define	SENSOR_ANALOG		// To make sure analog sensors are processed

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,  \
		INTERNAL_VOLTAGE_SENSOR,      \
		ANALOG_SENSOR (   SEN_x_ISI, \
				  SEN_x_NSA,  \
				  SEN_0_PIN,  \
				  SEN_x_URE,  \
				  SEN_x_SHT,  \
				  SEN_x_ERE), \
		ANALOG_SENSOR (   SEN_x_ISI, \
				  SEN_x_NSA,  \
				  SEN_1_PIN,  \
				  SEN_x_URE,  \
				  SEN_x_SHT,  \
				  SEN_x_ERE), \
		ANALOG_SENSOR (   SEN_x_ISI, \
				  SEN_x_NSA,  \
				  SEN_2_PIN,  \
				  SEN_x_URE,  \
				  SEN_x_SHT,  \
				  SEN_x_ERE), \
		ANALOG_SENSOR (   SEN_x_ISI, \
				  SEN_x_NSA,  \
				  SEN_3_PIN,  \
				  SEN_x_URE,  \
				  SEN_x_SHT,  \
				  SEN_x_ERE), \
		ANALOG_SENSOR (   SEN_x_ISI, \
				  SEN_x_NSA,  \
				  SEN_4_PIN,  \
				  SEN_x_URE,  \
				  SEN_x_SHT,  \
				  SEN_x_ERE), \
	}

#define N_HIDDEN_SENSORS	2
