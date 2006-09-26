#ifndef __pg_irq_rf24l01_h
#define __pg_irq_rf24l01_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

if (rf24l01_int) {

    clear_rf24l01_int;
    clr_rcv_int;
    p_trigger (zzv_drvprcs, ETYPE_USER, zzv_qevent);
    RISE_N_SHINE;

}

#endif
