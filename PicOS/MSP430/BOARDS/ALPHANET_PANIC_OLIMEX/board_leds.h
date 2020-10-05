/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// ============================================================================
// 3 leds on P4.5, P4.6, P4.7, polarity 0, one LED on P1.0, polarity 1
// ============================================================================

#define	LED0_ON		_BIC (P1OUT, 0x04)
#define	LED0_OFF	_BIS (P1OUT, 0x04)
#define	LED1_ON		_BIC (P1OUT, 0x08)
#define	LED1_OFF	_BIS (P1OUT, 0x08)
#define	LED2_ON		_BIC (P1OUT, 0x10)
#define	LED2_OFF	_BIS (P1OUT, 0x10)

#define	LED3_ON		_BIS (P1OUT, 0x01)
#define	LED3_OFF	_BIC (P1OUT, 0x01)

#define	LEDS_SAVE(a)	(a) = ((P1OUT & 0x1D) | (P1OUT & 0x1D))
#define	LEDS_RESTORE(a)	do { \
				P1OUT = (P1OUT & ~0x1D) | ((a) & 0x1D); \
			} while (0)
