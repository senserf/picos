/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// ============================================================================
// 3 leds on P4.5, P4.6, P4.7, polarity 0, one LED on P4.4, polarity 1
// ============================================================================

#define	LED0_ON		_BIC (P4OUT, 0x20)
#define	LED1_ON		_BIC (P4OUT, 0x40)
#define	LED2_ON		_BIC (P4OUT, 0x80)
#define	LED3_ON		_BIC (P4OUT, 0x10)

#define	LED0_OFF	_BIS (P4OUT, 0x20)
#define	LED1_OFF	_BIS (P4OUT, 0x40)
#define	LED2_OFF	_BIS (P4OUT, 0x80)
#define	LED3_OFF	_BIS (P4OUT, 0x10)


#define	LEDS_SAVE(a)	(a) = (P4OUT & 0xF0)
#define	LEDS_RESTORE(a)	do { \
				P4OUT = (P4OUT & ~0xF0) | ((a) & 0xF0); \
			} while (0)
