#ifndef __pg_irq_rf24g_h
#define __pg_irq_rf24g_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

if (rf24g_int) {

    clear_rf24g_int;
    i_trigger (ETYPE_USER, rxevent);

}

#endif
