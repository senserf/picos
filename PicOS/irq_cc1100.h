#ifndef __pg_irq_cc1100_h
#define __pg_irq_cc1100_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "cc1100.h"

if (cc1100_int) {

    clear_cc1100_int;

    rcv_disable_int;

    p_trigger (zzv_drvprcs, ETYPE_USER, zzv_qevent);

    RISE_N_SHINE;
}

#endif
