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

    clear_cc1100_int;

    rcv_disable_int;

    p_trigger (__pi_v_drvprcs, __pi_v_qevent);

    RISE_N_SHINE;

#ifdef	MONITOR_PIN_CC1100_INT
    _PVS (MONITOR_PIN_CC1100_INT, 0);
#endif

}

#endif
