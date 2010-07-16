#ifndef __pg_irq_rf24l01_h
#define __pg_irq_rf24l01_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "rf24l01.h"

if (rf24l01_int) {

    clear_rf24l01_int;
    clr_rcv_int;
    p_trigger (__pi_v_drvprcs, __pi_v_qevent);
    RISE_N_SHINE;

}

#endif
