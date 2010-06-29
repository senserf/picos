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
#define	PIN_DEFAULT_P4DIR	0x0E	// Veref selector (PAR/MOI)
#define	PIN_DEFAULT_P6DIR	0xD0	// For the SD card
#define	PIN_DEFAULT_P6OUT	0xD0

#define	PIN_MAX 		0
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0
