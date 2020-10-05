/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#ifdef	P1_INTERRUPT_SERVICE

#if CC1100

#include "irq_cc1100.h"

#endif
#endif

// ============================================================================

#ifdef	P2_INTERRUPT_SERVICE

#if MONITOR_PINS_SEND_INTERRUPTS
#include "irq_pins.h"
#endif

#endif
