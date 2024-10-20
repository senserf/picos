/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_irq_rf24l01_h
#define __pg_irq_rf24l01_h

#include "rf24l01.h"

if (rf24l01_int) {

    clear_rf24l01_int;
    clr_rcv_int;
    p_trigger (__rf24l01_v_drvprcs, __rf24l01_v_qevent);
    RISE_N_SHINE;

}

#endif
