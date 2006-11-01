/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

					
/* ==================================================================== */
/*     V E R S A 2   (pre-production variant: P1.2 instead of P2.2)     */
/* ==================================================================== */

#define	PIN_DEFAULT_P1DIR	0xF0
#define	PIN_DEFAULT_P2DIR	0x02
#define	PIN_DEFAULT_P4DIR	0xF0
#define	PIN_DEFAULT_P5DIR	0xFC
#define	PIN_DEFAULT_P6DIR	0x00


/*
 * Access to GP pins on the board:
 *
 * GP0-7 == P6.0 - P6.7
 *
 * CFG0 == P1.0
 * CFG1 == P1.1
 * CFG2 == P1.2 (2.2 in the target version)
 * CFG3 == P1.3 (used for reset)
 */

// Reset on CFG3 low (must be pulled up for normal operation)

#define	VERSA2_RESET_KEY_PRESSED	((P1IN & 0x08) == 0)

#define	PIN_LIST	{ 	\
	PIN_RESERVED,		\
	PIN_DEF (P6, 1),	\
	PIN_DEF (P6, 2),	\
	PIN_DEF (P6, 3),	\
	PIN_DEF (P6, 4),	\
	PIN_DEF (P6, 5),	\
	PIN_DEF (P6, 6),	\
	PIN_DEF (P6, 7),	\
	PIN_DEF (P1, 0),	\
	PIN_DEF (P1, 1),	\
	PIN_DEF (P1, 2),	\
	PIN_RESERVED,		\
}

#define	PIN_MAX			12	// Number of pins
#define	PIN_MAX_ANALOG		8	// Number of available analog pins
#define	PIN_DAC_PINS		0	// No DAC

/* ================= */
/* ADC used for RSSI */
/* ================= */
#define	PIN_ADC_RSSI		0	// This is P6.0 (P6 is implicit)

/* ================= */
/* Pin monitor stuff */
/* ================= */
#define	PULSE_MONITOR			PINS_MONITOR_INT (3, 4);
#define	MONITOR_PINS_SEND_INTERRUPTS	1

