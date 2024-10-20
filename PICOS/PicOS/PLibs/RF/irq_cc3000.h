/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_irq_cc3000_h
#define	__pg_irq_cc3000_h
//
// IRQ service for CC3000
//
if (cc3000_interrupt) {
	cc3000_irq_disable;
	cc3000_irq_clear;
	p_trigger (cc3000_event_thread, (word)(&cc3000_event_thread));
	RISE_N_SHINE;
}

#endif
