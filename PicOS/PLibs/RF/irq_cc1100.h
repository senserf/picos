#ifndef __pg_irq_cc1100_h
#define __pg_irq_cc1100_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

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