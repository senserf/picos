/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// Directions as viewed by the uC

// P1:
//	0 - RF FIFOP IN
//	1 - UART1TX (cross, must be input, ignore otherwise)
//	2 - FLASH_W OUT (write protect)
//	3 - RF FIFO IN
//	4 - RF CCA IN
//	5 - SHT11 SDA IN
//	6 - SHT11 CLK OUT
//	7 - SHT11 POWER OUT
				// 1100 0100
#define	PIN_DEFAULT_P1DIR	0xC4
#define	PIN_DEFAULT_P1OUT	0x00

// P2:
//	0 - CHARGER STATUS IN [ pulled up 100K ]
//	1 - NC
//	2 - UART1RX (cross, must be IN)
//	3 - NC
//	4 - SERIAL ID can be open (IN) [ pulled up ]
//	5 - NC
//	6 - NC
//	7 - USB Vcc/3V IN
				// 0110 1010
#define	PIN_DEFAULT_P2DIR	0x6A
#define	PIN_DEFAULT_P2OUT	0x00

// P3:
//	0 - NC
//	1 - RF SI OUT
//	2 - RF SO IN
//	3 - RF CLK OUT
//	4 - NC
//	5 - NC
//	6 - UART1TX
//	7 - UART1RX
				// 0101 1011
#define	PIN_DEFAULT_P3DIR	0x5B
#define	PIN_DEFAULT_P3OUT	0x00

// P4:
//	0 - NC
//	1 - RF SFD IN
//	2 - RF CS OUT
//	3 - NC
//	4 - FLASH CS OUT [ def high ]
//	5 - RF VRGEN_EN OUT [ pulled down 500K ]
//	6 - RF RESET OUT [ active low, pulled up 500K ]
//	7 - FLASH HOLD OUT [ def high ? ]

				// 1111 1101
#define	PIN_DEFAULT_P4DIR	0xFD
#define	PIN_DEFAULT_P4OUT	0xB4

// P5:
//	0 - NC
//	1 - NC
//	2 - NC
//	3 - NC
//	4 - LED1 (active high) Y
//	5 - LED2 R
//	6 - LED3 G
//	7 - NC
				// 1111 1111
#define	PIN_DEFAULT_P5DIR	0xFF
#define	PIN_DEFAULT_P5OUT	0x00

// P6:
//	CONNECTOR
#define	PIN_DEFAULT_P6DIR	0xFF
#define	PIN_DEFAULT_P6OUT	0x00

#include "sht_xx.h"

// ?????
#define	shtxx_ini_regs	_BIS (P1OUT, 0x80)
#define	shtxx_dtup	_BIC (P1DIR, 0x20)
#define	shtxx_dtdown	_BIS (P1DIR, 0x20)
#define	shtxx_dtin	_BIC (P1DIR, 0x20)
#define	shtxx_dtout	do { } while (0)
#define	shtxx_data	(P1IN & 0x20)

#define	shtxx_ckup	_BIS (P1OUT, 0x40)
#define	shtxx_ckdown	_BIC (P1OUT, 0x40)

// ============================================================================

#define	PIN_LIST	{	\
	PIN_DEF	(P6, 0),	\
	PIN_DEF	(P6, 1),	\
	PIN_DEF	(P6, 2),	\
	PIN_DEF	(P6, 3),	\
	PIN_DEF	(P6, 4),	\
	PIN_DEF	(P6, 5),	\
	PIN_DEF	(P6, 6),	\
	PIN_DEF	(P6, 7),	\
}

#define	PIN_MAX			8	// Number of pins
#define	PIN_MAX_ANALOG		8	// Number of available analog pins
#define	PIN_DAC_PINS		0x0706	// Two DAC pins: #6 and #7


// ============================================================================
#if 1

#include "analog_sensor.h"
#include "sensors.h"

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,			\
		INTERNAL_VOLTAGE_SENSOR,			\
		DIGITAL_SENSOR (0, shtxx_init, shtxx_temp),	\
		DIGITAL_SENSOR (0, NULL, shtxx_humid),		\
	}
#define	SENSOR_ANALOG
#define	SENSOR_DIGITAL
#define	SENSOR_INITIALIZERS

#define	N_HIDDEN_SENSORS	2

#define	SENSOR_TEMP	0
#define	SENSOR_HUMID	1

#endif

// ============================================================================

