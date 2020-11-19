/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__model_h__
#define	__model_h__

#include "interface.h"
#include "plant.h"
#include "params.h"

#define	CONTROLLER_HOST	"localhost"
#define	VUEE_PORT	4443		// This is the default port

// Macro to make sure that a bounded value is actually between the bounds
#define	enforce_bounds(a,min,max)	do { \
						if ((a) < (min)) \
							(a) = (min); \
						else if ((a) > (max)) \
							(a) = (max); \
					} while (0)

// ============================================================================

station Oven : Controller {

	double	Target,		// The current target temperature setting
		Current,	// Current temperature
		CoolingFrom,	// Starting temperature for cooling
		BaseTime,	// Staring time for heating
		TCHeat,		// Time constant for heating
		TCCool,		// Time constant for cooling
		Inertia,	// The inertia (between 0 and 1)
		DeltaT;		// Time elapsed since last update

	Boolean Heating,	// YES if heating
		Cooling;	// YES if cooling

	TIME	LastUpdateTime;

	void update_current ();
	void update_setting ();
	int temp_to_reading ();
	void update_sensor ();

	void setup (double tch, double tcc, double ine) {

		TCHeat = tch;
		TCCool = tcc;
		Inertia = ine;

		Controller::setup (DELTA_T, CONTROLLER_HOST, VUEE_PORT, 0, 
			THERMOCOUPLE, OVEN, 0);
	};

	void showState ();
	void response ();
	void reset ();
};

process Root {

	states { Start, Never };

	void buildPlant ();

	perform;
};

process Logger (Oven) {

	TIME NextRunTime, Delta;

	states { Loop };

	void setup (int);

	perform;
};

#endif
