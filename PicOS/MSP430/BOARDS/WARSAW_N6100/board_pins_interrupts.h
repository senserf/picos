/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifdef	P1_INTERRUPT_SERVICE

#if CC1100
#include "irq_cc1100.h"
#endif

#define	buttons_int (P1IFG & P1_PINS_INTERRUPT_MASK)
#include "irq_buttons.h"
#undef 	buttons_int

#endif

#ifdef	P2_INTERRUPT_SERVICE

#define	buttons_int (P2IFG & P2_PINS_INTERRUPT_MASK)
#include "irq_buttons.h"
#undef 	buttons_int

#endif
