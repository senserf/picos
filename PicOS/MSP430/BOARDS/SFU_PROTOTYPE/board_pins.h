/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Departures from default pre-initialization
#define	PIN_DEFAULT_P1DIR	0x80
#define	PIN_DEFAULT_P3DIR	0xCF
#define	PIN_DEFAULT_P5DIR	0xE0
#define	PIN_DEFAULT_P6DIR	0x00

#define	PIN_LIST	{ 	\
				\
	PIN_DEF (P6, 0),	\
	PIN_DEF (P6, 1),	\
	PIN_DEF (P6, 2),	\
	PIN_DEF (P6, 3),	\
	PIN_DEF (P6, 4),	\
	PIN_DEF (P6, 5),	\
	PIN_DEF (P6, 6),	\
	PIN_DEF (P6, 7),	\
				\
	PIN_DEF (P2, 0),	\
	PIN_DEF (P2, 1),	\
	PIN_DEF (P2, 2),	\
	PIN_DEF (P2, 3),	\
	PIN_DEF (P2, 4),	\
	PIN_DEF (P2, 5),	\
	PIN_DEF (P2, 6),	\
	PIN_DEF (P2, 7),	\
				\
	PIN_DEF (P4, 0),	\
	PIN_DEF (P4, 1),	\
	PIN_DEF (P4, 2),	\
	PIN_DEF (P4, 3),	\
	PIN_DEF (P4, 4),	\
	PIN_DEF (P4, 5),	\
	PIN_DEF (P4, 6),	\
	PIN_DEF (P4, 7),	\
				\
	PIN_DEF (P5, 0),	\
	PIN_DEF (P5, 1),	\
	PIN_DEF (P5, 2),	\
	PIN_DEF (P5, 3),	\
	PIN_DEF (P5, 4),	\
	PIN_DEF (P5, 5),	\
	PIN_DEF (P5, 6),	\
	PIN_DEF (P5, 7),	\
}

#define	PIN_MAX			32	// Number of pins
#define	PIN_MAX_ANALOG		8	// Number of available analog pins
#define	PIN_DAC_PINS		0x0706	// Two DAC pins: #6 and #7
