/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// ============================================================================
// CMA3000 - acceleration sensor ==============================================
// ============================================================================

#include "cma3000.h"

#define	cma3000_bring_down		do { \
						_BIC (P4OUT, 0x10); \
						_BIC (P5OUT, 0xE4); \
						_BIS (P5DIR, 0xE4); \
						cma3000_disable; \
					} while (0)
//+++ "p1irq.c"
REQUEST_EXTERNAL (p1irq);

#define	cma3000_bring_up		do { \
						_BIC (P5DIR, 0x20); \
						_BIS (P4OUT, 0x10); \
					} while (0)
					
#define	cma3000_csel			do { \
						_BIC (P5REN, 0x20); \
						_BIC (P5OUT, 0x04); \
					} while (0)

#define	cma3000_cunsel			do { \
						_BIS (P5OUT, 0x04); \
						_BIS (P5REN, 0x80); \
					} while (0)

// Rising edge, no need to change IES
#define	cma3000_enable	_BIS (P1IE, 0x80)
#define	cma3000_disable	_BIC (P1IE, 0x80)
#define	cma3000_clear	_BIC (P1IFG, 0x80)
#define	cma3000_int	(P1IFG & 0x80)

// Raw pin I/O
#define	cma3000_clkh	_BIS (P5OUT, 0x40)
#define	cma3000_clkl	_BIC (P5OUT, 0x40)
#define	cma3000_outh	_BIS (P5OUT, 0x80)
#define	cma3000_outl	_BIC (P5OUT, 0x80)
#define	cma3000_data	(P5IN & 0x20)

// Delay for write
#define	cma3000_delay	udelay (10)

// ============================================================================
// ============================================================================

#define	SENSOR_DIGITAL
#define	SENSOR_ANALOG
#define	SENSOR_EVENTS
#define	SENSOR_INITIALIZERS

#include "analog_sensor.h"
#include "sensors.h"
//+++ "sensors.c"

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,   \
		INTERNAL_VOLTAGE_SENSOR,       \
		DIGITAL_SENSOR (0, NULL, cma3000_read) \
}

// Temperature				<-> hidden
// Voltage internal-internal
// -------------------------
// Motion

#define	N_HIDDEN_SENSORS	2
