/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Departures from default pre-initialization
#define	PIN_DEFAULT_P1DIR	0x00
#define	PIN_DEFAULT_P2DIR	0x00
#define	PIN_DEFAULT_P3DIR	0x00
#define	PIN_DEFAULT_P4DIR	0xFF
#define	PIN_DEFAULT_P5DIR	0x00
#define	PIN_DEFAULT_P6DIR	0x00

#define	PIN_LIST	{	\
	PIN_DEF	(P1, 1),	\
	PIN_DEF (P2, 7),	\
	PIN_DEF	(P3, 0),	\
	PIN_DEF	(P3, 1),	\
	PIN_DEF	(P3, 2),	\
	PIN_DEF	(P3, 3),	\
	PIN_DEF	(P3, 6),	\
	PIN_DEF	(P3, 7),	\
	PIN_DEF	(P5, 4),	\
	PIN_DEF	(P5, 5),	\
	PIN_DEF	(P5, 6),	\
	PIN_DEF	(P5, 7),	\
}

#define	PIN_MAX			12	// Number of pins
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0x0
