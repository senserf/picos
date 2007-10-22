/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Departures from default pre-initialization
#define	PIN_DEFAULT_P1DIR	0x00
#define	PIN_DEFAULT_P2DIR	0x83	// 0,1 and 7 hang loose
#define	PIN_DEFAULT_P3DIR	0xC9
#define	PIN_DEFAULT_P5DIR	0xE0
#define	PIN_DEFAULT_P4DIR	0x0E
#define	PIN_DEFAULT_P4OUT	0x0E	// LEDs off by default
#define	PIN_DEFAULT_P6DIR	0x00

#define	EEPROM_INIT_ON_KEY_PRESSED	((P2IN & 0x10) == 0)

#if ADC_SAMPLER

#define	PIN_LIST	{	\
	PIN_DEF	(P1, 6),	\
	PIN_DEF	(P1, 7),	\
	PIN_DEF	(P2, 2),	\
	PIN_DEF	(P2, 3),	\
	PIN_DEF	(P2, 4),	\
	PIN_DEF	(P2, 5),	\
	PIN_DEF	(P2, 6),	\
	PIN_DEF	(P4, 0),	\
	PIN_DEF	(P4, 4),	\
	PIN_DEF	(P4, 5),	\
	PIN_DEF	(P4, 6),	\
	PIN_DEF	(P4, 7),	\
	PIN_DEF	(P5, 4),	\
}

#define	PIN_MAX			13	// Number of pins
#define	PIN_MAX_ANALOG		0	// Number of available analog pins
#define	PIN_DAC_PINS		0

#else	/* NO SAMPLER */

#define	PIN_LIST	{	\
	PIN_DEF	(P6, 0),	\
	PIN_DEF	(P6, 1),	\
	PIN_DEF	(P6, 2),	\
	PIN_DEF	(P6, 3),	\
	PIN_DEF	(P6, 4),	\
	PIN_DEF	(P6, 5),	\
	PIN_DEF	(P6, 6),	\
	PIN_DEF	(P6, 7),	\
	PIN_DEF	(P1, 6),	\
	PIN_DEF	(P1, 7),	\
	PIN_DEF	(P2, 2),	\
	PIN_DEF	(P2, 3),	\
	PIN_DEF	(P2, 4),	\
	PIN_DEF	(P2, 5),	\
	PIN_DEF	(P2, 6),	\
	PIN_DEF	(P4, 0),	\
	PIN_DEF	(P4, 4),	\
	PIN_DEF	(P4, 5),	\
	PIN_DEF	(P4, 6),	\
	PIN_DEF	(P4, 7),	\
	PIN_DEF	(P5, 4),	\
}

#define	PIN_MAX			21	// Number of pins
#define	PIN_MAX_ANALOG		8	// Number of available analog pins
#define	PIN_DAC_PINS		0x0706	// Two DAC pins: #6 and #7

#endif /* SAMPLER or no SAMPLER */

