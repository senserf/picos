/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
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
#define	PIN_DEFAULT_P6DIR	0x00	// For the SD card
#define	PIN_DEFAULT_P6OUT	0x00	// P6.0 - power source sensor

//#define	RESET_ON_KEY_PRESSED	((P2IN & 0x10) == 0)

#define	PIN_MAX 		0
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0

#define	ADC_CLOCK_SOURCE	ADC12SSEL_ACLK
#define	ADC_CLOCK_DIVIDER	ADC12DIV_1

// This makes the SHT unit for ADC equal to ca. 61us. The typical power source
// is a 12V battery, which we will divide through 4.7K + 47K. With this
// impedance, a safe SHT is calculated as 20us. We shall overkill it, thrice or
// so, and sample multiple times comparing against the internal reference of
// 1.5V

#include "analog_sensor.h"
#include "sensors.h"

#define	SEN_POWER_PIN	0	// P6.0
#define	SEN_POWER_SHT	1	// 8 cycles = 490us
#define	SEN_POWER_ISI	0	// Inter-sample interval
#define	SEN_POWER_NSA	2	// 16 samples
#define	SEN_POWER_REF	0	// Reference == internal 1.5V

#define	SENSOR_LIST	{ \
		SENSOR_DEF (NULL, analog_sensor_read, \
			 SEN_POWER_PIN | \
			(SEN_POWER_SHT << ASNS_SHT_SH) | \
			(SEN_POWER_ISI << ASNS_ISI_SH) | \
			(SEN_POWER_NSA << ASNS_NSA_SH) | \
			(SEN_POWER_REF << ASNS_REF_SH))  \
	}