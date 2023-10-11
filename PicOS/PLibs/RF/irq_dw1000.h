/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_irq_dw1000_h
#define __pg_irq_dw1000_h

#include "dw1000.h"

if (dw1000_int) {

    dw1000_clear_int;

    dw1000_int_disable;

    p_trigger (__dw1000_v_drvprcs, (word)(&__dw1000_v_drvprcs));

    RISE_N_SHINE;

}

#endif
