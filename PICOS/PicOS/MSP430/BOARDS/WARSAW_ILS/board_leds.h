/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// ============================================================================
// 3 leds on P4, polarity 0 ===================================================
// ============================================================================

#define	LED0_ON		_BIC (P4OUT, 0x04)
#define	LED1_ON		_BIC (P4OUT, 0x02)
#define	LED2_ON		_BIC (P4OUT, 0x08)
#define	LEDS_ON		_BIC (P4OUT, 0x0E)

#define	LED0_OFF	_BIS (P4OUT, 0x04)
#define	LED1_OFF	_BIS (P4OUT, 0x02)
#define	LED2_OFF	_BIS (P4OUT, 0x08)
#define	LEDS_OFF	_BIS (P4OUT, 0x0E)

#define	LEDS_SAVE(a)	(a) = P4OUT & 0xE
#define	LEDS_RESTORE(a)	P4OUT = (P4OUT & ~0xE) | (a)
