/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_irq_rf24g_h
#define __pg_irq_rf24g_h

if (rf24g_int) {

    clear_rf24g_int;
    i_trigger (rxevent);
    RISE_N_SHINE;

}

#endif
