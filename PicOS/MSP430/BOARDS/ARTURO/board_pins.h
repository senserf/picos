/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// Departures from default pre-initialization
#define	PIN_DEFAULT_P1DIR	0x80	// P1.7 == CLK for the SHT sensor
#define	PIN_DEFAULT_P2DIR	0x83	// 0,1 and 7 hang loose, 2 = DS18B20
#define	PIN_DEFAULT_P3DIR	0xC9

// P5.4 == reference voltage for PAR, EEPROM: 0-CS, 1-SI, 2-SO, 3-SCK
#define	PIN_DEFAULT_P5DIR	0xEB
#define	PIN_DEFAULT_P5OUT	0x19

#define	PIN_DEFAULT_P4DIR	0x0E
#define	PIN_DEFAULT_P4OUT	0x0E	// LEDs off by default
#define	PIN_DEFAULT_P6DIR	0x00
#define	PIN_DEFAULT_P6SEL	0x03	// The two analog sensors

#define	PIN_MAX 		0
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0

#include "sht_xx.h"
#include "ds18b20.h"
#include "analog_sensor.h"
#include "sensors.h"

#define	QSO_PAR_PIN	0	// PAR sensor = P6.0
#define	QSO_PAR_SHT	4	// Sample hold time indicator
#define	QSO_PAR_ISI	1	// Inter sample interval indicator
#define	QSO_PAR_NSA	512	// Number of samples
#define	QSO_PAR_URE	SREF_VEREF_AVSS	// Voltage reference: Veref (Zener)
#define	QSO_PAR_ERE	0	// Exported reference (none)

#define	PHT_DIO_PIN	1	// Photodiode = P6.1
#define	PHT_DIO_SHT	4	// Sample hold time indicator
#define	PHT_DIO_ISI	1	// Inter sample interval indicator
#define	PHT_DIO_NSA	512	// Number of samples
#define	PHT_DIO_URE	SREF_VEREF_AVSS
#define	PHT_DIO_ERE	0

#define	SENSOR_INITIALIZERS	// To make sure global init function gets called
#define	SENSOR_ANALOG		// To make sure analog sensors are processed
#define	SENSOR_DIGITAL		// To make sure digital sensors are processed

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,	\
		INTERNAL_VOLTAGE_SENSOR,	\
		ANALOG_SENSOR (   QSO_PAR_ISI,  \
				  QSO_PAR_NSA,  \
				  QSO_PAR_PIN,  \
				  QSO_PAR_URE,  \
				  QSO_PAR_SHT,  \
				  QSO_PAR_ERE), \
		DIGITAL_SENSOR (   0, shtxx_init, shtxx_temp), \
		DIGITAL_SENSOR (   0, NULL, shtxx_humid), \
		ANALOG_SENSOR (   PHT_DIO_ISI,  \
				  PHT_DIO_NSA,  \
				  PHT_DIO_PIN,  \
				  PHT_DIO_URE,  \
				  PHT_DIO_SHT,  \
				  PHT_DIO_ERE), \
		DIGITAL_SENSOR (   0, ds18b20_init, ds18b20_read) \
	}

#define	N_HIDDEN_SENSORS	2

#define	sensor_adc_prelude(p) 	_BIS (P5DIR, 0x10)	// Eref is switchable
#define	sensor_adc_postlude(p) 	_BIC (P5DIR, 0x10)	// Eref is switchable

#define	SENSOR_PAR	0
#define	SENSOR_TEMP	1
#define	SENSOR_HUMID	2
#define	SENSOR_PHOTO	3
#define SENSOR_TEMPA	4

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

// Pin definitions for the DS18B20 sensor
#define	ds18b20_ini_regs	CNOP
#define	ds18b20_pull_down	_BIS (P2DIR, 0x04)
#define	ds18b20_release		_BIC (P2DIR, 0x04)
#define	ds18b20_value		(P2IN & 0x04)
