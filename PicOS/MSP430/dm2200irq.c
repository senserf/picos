/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "dm2200.h"

interrupt (TIMERA0_VECTOR) dm2200_st_int () {

// Signal strobe for transmission
#include "irq_dm2200_xmt.h"

}
