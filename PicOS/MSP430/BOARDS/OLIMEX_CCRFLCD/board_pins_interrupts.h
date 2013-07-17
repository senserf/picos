/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifdef	P1_INTERRUPT_SERVICE

// Button service
#define	buttons_int (P1IFG & P1_BUTTONS_INTERRUPT_MASK)
#include "irq_buttons.h"

#endif

#ifdef	P2_INTERRUPT_SERVICE

// Button service
#define	buttons_int (P2IFG & P2_BUTTONS_INTERRUPT_MASK)
#include "irq_buttons.h"

#endif
