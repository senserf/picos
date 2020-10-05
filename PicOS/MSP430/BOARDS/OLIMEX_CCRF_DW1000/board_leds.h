/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// ============================================================================
// 1 led on P1, polarity 1 ====================================================
// ============================================================================

#define	LED0_ON		_BIS (P1OUT, 0x01)

#define	LED0_OFF	_BIC (P1OUT, 0x01)

#define	LEDS_SAVE(a)	(a) = (P1OUT & 0x01)
#define	LEDS_RESTORE(a)	do { \
				P1OUT = (P1OUT & ~0x01) | ((a) & 0x01); \
			} while (0)
