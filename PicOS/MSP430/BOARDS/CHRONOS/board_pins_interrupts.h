/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifdef	P1_INTERRUPT_SERVICE
#endif

#ifdef	P2_INTERRUPT_SERVICE

// ============================================================================
// Button service =============================================================
// ============================================================================
#if BUTTONS_DRIVER

#include "irq_buttons.h"

#else

#define	pin_sensor_int	(P2IFG & PIN_SENSOR_P2_IRQ)
#include "irq_pin_sensor.h"
#undef	pin_sensor_int

#endif
// ============================================================================

// The accelerometer
#include "irq_cma3000.h"

#endif
