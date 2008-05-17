/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Departures from default pre-initialization
#define	PIN_DEFAULT_P1DIR	0x80	// P1.7 == CLK for the SHT sensor
#define	PIN_DEFAULT_P2DIR	0x83	// 0,1 and 7 hang loose, 2 = DS18B20
#define	PIN_DEFAULT_P3DIR	0xC9
#define	PIN_DEFAULT_P5DIR	0xE0
#define	PIN_DEFAULT_P5OUT	0x10	// P5.4 == reference voltage for PAR
#define	PIN_DEFAULT_P4DIR	0x0E
#define	PIN_DEFAULT_P4OUT	0x0E	// LEDs off by default
#define	PIN_DEFAULT_P6DIR	0x00	// P6.0 input from PAR

//#define	EEPROM_INIT_ON_KEY_PRESSED	((P2IN & 0x10) == 0)

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
#define	QSO_PAR_NSA	6	// Number of samples, corresponds to 512
#define	QSO_PAR_REF	3	// Voltage reference: Veref (Zener)

#define	PHT_DIO_PIN	1	// Photodiode = P6.1
#define	PHT_DIO_SHT	4	// Sample hold time indicator
#define	PHT_DIO_ISI	1	// Inter sample interval indicator
#define	PHT_DIO_NSA	6	// Number of samples, corresponds to 512
#define	PHT_DIO_REF	3	// Veref as well

#define	EREF_ON		_BIS (P5DIR, 0x10)	// Eref is switchable
#define	EREF_OFF	_BIC (P5DIR, 0x10)

#define	SENSOR_LIST	{ \
		SENSOR_DEF (NULL, analog_sensor_read, \
			 QSO_PAR_PIN | \
			(QSO_PAR_SHT << ASNS_SHT_SH) | \
			(QSO_PAR_ISI << ASNS_ISI_SH) | \
			(QSO_PAR_NSA << ASNS_NSA_SH) | \
			(QSO_PAR_REF << ASNS_REF_SH)), \
		SENSOR_DEF (shtxx_init, shtxx_temp, 0), \
		SENSOR_DEF (NULL, shtxx_humid, 0), \
		SENSOR_DEF (NULL, analog_sensor_read, \
			 PHT_DIO_PIN | \
			(PHT_DIO_SHT << ASNS_SHT_SH) | \
			(PHT_DIO_ISI << ASNS_ISI_SH) | \
			(PHT_DIO_NSA << ASNS_NSA_SH) | \
			(PHT_DIO_REF << ASNS_REF_SH)), \
		SENSOR_DEF (ds18b20_init, ds18b20_read, 0) \
	}

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
