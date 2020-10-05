/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_irq_cc2420_h
#define __pg_irq_cc2420_h

#include "cc2420.h"

if (cc2420_int) {

#ifdef	MONITOR_PIN_CC2420_INT
    _PVS (MONITOR_PIN_CC2420_INT, 1);
#endif

    cc2420_clear_int;

    cc2420_rcv_int_disable;

    p_trigger (__cc2420_v_drvprcs, __cc2420_v_qevent);

    RISE_N_SHINE;

#ifdef	MONITOR_PIN_CC2420_INT
    _PVS (MONITOR_PIN_CC2420_INT, 0);
#endif

}

#endif
