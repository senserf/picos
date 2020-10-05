/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_irq_cc1100_h
#define __pg_irq_cc1100_h

#include "cc1100.h"

if (cc1100_int) {

#ifdef	MONITOR_PIN_CC1100_INT
    _PVS (MONITOR_PIN_CC1100_INT, 1);
#endif

    cc1100_clear_int;

    cc1100_rcv_int_disable;

    p_trigger (__cc1100_v_drvprcs, __cc1100_v_qevent);

    RISE_N_SHINE;

#ifdef	MONITOR_PIN_CC1100_INT
    _PVS (MONITOR_PIN_CC1100_INT, 0);
#endif

}

#endif
