/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#define	rtc_clk_delay	udelay (2)

// Both CLK and DATA are open drain, so we never pull them up from here; this
// assumes that their OUT bits are both zero
#define	rtc_clkh	do { rtc_clk_delay; _BIC (P2DIR, 0x02); } while (0)
#define	rtc_clkl	do { rtc_clk_delay; _BIS (P2DIR, 0x02); } while (0)
#define	rtc_set_input	_BIC (P2DIR, 0x80)
#define	rtc_outh	rtc_set_input
#define	rtc_outl	_BIS (P2DIR, 0x80)
#define	rtc_inp		(P2IN & 0x80)
// In case special actions are required before we start talking

#define	rtc_open	CNOP
#define	rtc_close	CNOP

// Note: for battery backup, use a pin to drive the pull-up resistors, and do 
// something like this:
// #define	rtc_open	do { _BIS (P2DIR, 0x20); _BIS (P2OUT, 0x20); mdelay (1); } while (0)
// #define	rtc_close	do { _BIS (P2DIR, 0x20); _BIC (P2OUT, 0x20); } while (0)
// Of course, the direction can be preset in board_pins.h

// Note: if the clock is not uninitialized, then the chip drains about 150uA,
// but the roblem goes away if the pull-ups are driven by a pin.
