/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Departures from default pre-initialization
#define	PIN_DEFAULT_P1DIR	0x80	// P1.7 == DATA for the SHT sensor
#define	PIN_DEFAULT_P2DIR	0x83	// 0,1 and 7 hang loose
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
#include "qso_par.h"
#include "sensors.h"

#define	SENSOR_LIST	{ \
		SENSOR_DEF (NULL, qso_par_read), \
		SENSOR_DEF (shtxx_init, shtxx_temp), \
		SENSOR_DEF (NULL, shtxx_humid) \
	}

#define	SENSOR_PAR	0
#define	SENSOR_TEMP	1
#define	SENSOR_HUMID	2

// Pin definitions for the SHT sensor

#define	shtxx_ini_regs	do { _BIS (P1OUT, 0x80); _BIC (P1OUT, 0x40); } while (0)
#define	shtxx_dtup	_BIC (P1DIR, 0x40)
#define	shtxx_dtdown	_BIS (P1DIR, 0x40)
#define	shtxx_dtin	_BIC (P1DIR, 0x40)
#define	shtxx_dtout	do { } while (0)
#define	shtxx_data	(P1IN & 0x40)

#define	shtxx_ckup	_BIS (P1OUT, 0x80)
#define	shtxx_ckdown	_BIC (P1OUT, 0x80)

// Definitions for the QSO PAR sensor

#define QSO_PAR_PIN	0	// P6.0
#define	QSO_PAR_REF	3	// Veref

// For providing the reference voltage dynamically - to save on battery;
// let us just check it out
#define	qso_set_ref	_BIS (P5DIR, 0x10)
#define	qso_clr_ref	_BIC (P5DIR, 0x10)
