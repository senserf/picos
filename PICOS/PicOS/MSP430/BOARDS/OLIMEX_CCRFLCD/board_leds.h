/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// ============================================================================
// 1 led on P1, 1 led on P2, polarity 1 =======================================
// ============================================================================

#define	LED0_ON		_BIS (P1OUT, 0x01)
#define	LED1_ON		_BIS (P2OUT, 0x40)
#define	LED0_OFF	_BIC (P1OUT, 0x01)
#define	LED1_OFF	_BIC (P2OUT, 0x40)

#define	LEDS_SAVE(a)	(a) = ((P1OUT & 0x01) | (P2OUT & 0x40))
#define	LEDS_RESTORE(a)	do { \
				P1OUT = (P1OUT & ~0x01) | ((a) & 0x01); \
				P2OUT = (P2OUT & ~0x40) | ((a) & 0x40); \
			} while (0)
