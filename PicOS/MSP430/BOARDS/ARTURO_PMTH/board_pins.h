/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//
// Sensor connections:
//
//	PAR = P6.0	(analog input)
//	MOI = P6.2	(analog input)
//	SHT = P1.6 Data, P1.7 Clock
//
//	For reading PAR, set Veref to 1.2V by pulling P4.7 down and P5.4
//	up to Vcc
//	For reading MOI, set Veref to excitation voltage (minus constant
//	forward Zener drop) by setting P5.4 to ground and P4.7 to Vcc
//	(which will also excite the sensor).
//	The normal state of P4.7/P5.4 should be output low for both
//

// Departures from default pre-initialization
#define	PIN_DEFAULT_P1DIR	0x80	// P1.7 == CLK for the SHT sensor
#define	PIN_DEFAULT_P2DIR	0x83	// 0,1 and 7 hang loose, 2-6 nc
#define	PIN_DEFAULT_P3DIR	0xC9
#define	PIN_DEFAULT_P5DIR	0xF0	// P5.4 == Veref selector (PAR/MOI)
#define	PIN_DEFAULT_P5OUT	0x00
#define	PIN_DEFAULT_P4OUT	0x0E	// LEDs off by default
#define	PIN_DEFAULT_P4DIR	0x8E	// P4.7 == Veref selector (PAR/MOI)
#define	PIN_DEFAULT_P6DIR	0x00	// P6.0 PAR input, P6.2 MOI input

//#define	EEPROM_INIT_ON_KEY_PRESSED	((P2IN & 0x10) == 0)

#define	PIN_MAX 		0
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0

#include "sht_xx.h"
#include "analog_sensor.h"
#include "sensors.h"

#define	QSO_PAR_PIN	0	// PAR sensor = P6.0
#define	QSO_PAR_SHT	4	// Sample hold time indicator
#define	QSO_PAR_ISI	1	// Inter sample interval indicator
#define	QSO_PAR_NSA	6	// Number of samples, corresponds to 512
#define	QSO_PAR_REF	3	// Voltage reference: Veref

#define	MOI_ECO_PIN	7	// ECHO moisture sensor = P6.7
#define	MOI_ECO_SHT	4	// Sample hold time indicator
#define	MOI_ECO_ISI	0	// Inter sample interval indicator
#define	MOI_ECO_NSA	2	// Number of samples, corresponds to 16
#define	MOI_ECO_REF	3	// Voltage reference: Veref

// This is referenced in analog_senor.c
#define	EREF_ON		do { \
				if (ASNS_PNO == QSO_PAR_PIN) { \
					_BIS (P5OUT, 0x10); \
				} else { \
					_BIS (P4OUT, 0x80); \
					mdelay (20); \
				} \
			} while (0)

#define	EREF_OFF	do { _BIC (P5OUT, 0x10); _BIC (P4OUT, 0x80); } while (0)

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
			MOI_ECO_PIN | \
			(MOI_ECO_SHT << ASNS_SHT_SH) | \
			(MOI_ECO_ISI << ASNS_ISI_SH) | \
			(MOI_ECO_NSA << ASNS_NSA_SH) | \
			(MOI_ECO_REF << ASNS_REF_SH)) \
	}

#define	SENSOR_PAR	0
#define	SENSOR_TEMP	1
#define	SENSOR_HUMID	2
#define	SENSOR_MOI	3

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
