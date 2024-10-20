/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// ============================================================================
// 3 leds on P5, polarity 1 ===================================================
// ============================================================================

#define	LED0_ON		_BIS (P5OUT, 0x10)
#define	LED1_ON		_BIS (P5OUT, 0x20)
#define	LED2_ON		_BIS (P5OUT, 0x40)
#define	LEDS_ON		_BIS (P5OUT, 0x70)

#define	LED0_OFF	_BIC (P5OUT, 0x10)
#define	LED1_OFF	_BIC (P5OUT, 0x20)
#define	LED2_OFF	_BIC (P5OUT, 0x40)
#define	LEDS_OFF	_BIC (P5OUT, 0x70)

#define	LEDS_SAVE(a)	(a) = P5OUT & 0x70
#define	LEDS_RESTORE(a)	P5OUT = (P5OUT & ~0x70) | (a)
