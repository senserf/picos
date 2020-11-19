/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __params_h__
#define	__params_h__

#include "interface.h"

#define	DELTA_T		0.1	// Make it 10 times faster than the controller
#define	MIN_TEMP	20.0	// The floor (lowest) temperature in deg C
#define	MAX_TEMP	(MAXTEMP/10.0)
#define	EPSILON_TEMP	0.05	// The accuracy of temperature setting
#define	OVERSHOT_FACTOR	1.0

#define	DEFAULT_INERTIA		0.7	// Between 0 and 1
#define	DEFAULT_HTC		30.0	// Heating time constant (sec * 10)
#define	DEFAULT_CTC		50.0	// Cooling time constant (sec * 10)

// ============================================================================
#endif
