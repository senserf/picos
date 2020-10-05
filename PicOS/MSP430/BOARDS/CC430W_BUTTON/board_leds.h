/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// ============================================================================
// 1 leds on P4.6, polarity low
// ============================================================================

#define	LED0_ON		_BIC (P4OUT, 0x40)
#define	LED0_OFF	_BIS (P4OUT, 0x40)

#define	LEDS_SAVE(a)	(a) = (P4DIR & 0x40)
#define	LEDS_RESTORE(a)	P4DIR = (P4DIR & ~0x40) | ((a) & 0x40)
