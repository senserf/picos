/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifdef	P1_INTERRUPT_SERVICE

// Button service
#define	buttons_int (P1IFG & P1_PINS_INTERRUPT_MASK)
#include "irq_buttons.h"

#endif
