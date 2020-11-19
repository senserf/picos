/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "model.h"

identify "A model of a reflow oven";

#include "plant.cc"

// ============================================================================

typedef struct {
//
// We store the temperature as a floating point
//
	double  Value;
	int	Reading;

} calentry_t;
	
static calentry_t Cal [] = THERMOCOUPLE_CALIBRATION;

#define	CalLength	(sizeof (Cal) / sizeof (calentry_t))

Oven *TheOven;

static void bad_args () {

	excptn ("bad model-specific arguments,"
		" usage: ... -- [-h htc] [-c ctc] [-l logintrvl]");
}

void Oven::update_current () {

	double dt, v;

	if (Heating) {
		if (Current >= Target) {
			Current = Target;
			Heating = NO;
			return;
		}
		dt = BaseTime / TCHeat;
		v = 1.0;
		if (dt < 20.0)
			v -= exp (-dt);
		Current = v * (Target - MIN_TEMP) + MIN_TEMP;
		update_sensor ();
		return;
	}

	if (Cooling) {
		if (Current <= Target) {
			Current = Target;
			Cooling = NO;
			return;
		}
		dt = BaseTime / TCCool;
		v = 1.0;
		if (dt < 20.0)
			v -= exp (-dt);
		Current = v * (Target - CoolingFrom) + CoolingFrom;
		update_sensor ();
		return;
	}
	update_sensor ();
}
		
void Oven::update_setting () {
//
// Called when the setting has changed
//
	int act;

	readActuator (act);
	enforce_bounds (act, OVEN_MIN, OVEN_MAX);

	// Set the target temperature
	Target = (1.0 - Inertia) *
		((act * MAX_TEMP * OVERSHOT_FACTOR) / OVEN_MAX) +
			Inertia * Target;

	if (Target < MIN_TEMP)
		Target = MIN_TEMP;

	if (fabs (Target - Current) < EPSILON_TEMP) {
		// Target reached
		Cooling = NO;
		Heating = NO;
		return;
	}

	if (Target < Current) {
		Cooling = YES;
		Heating = NO;
		CoolingFrom = Current;
		BaseTime = 0.0;
		return;
	}

	Heating = YES;
	Cooling = NO;

	// This is a bit more complicated; calculate base time as if the
	// heating curve started at MIN_TEMP
	BaseTime = -TCHeat * log (1.0 - (Current - MIN_TEMP) /
		(Target - MIN_TEMP));
}

inline int Oven::temp_to_reading () {
//
// Convert current to a sensor reading
//
	int a, b, c;
	double r0, r1, v0, v1, v;

	a = 0;
	b = CalLength - 1;

	while ((c = (a + b) >> 1) != a) {
		// Binary search for Current
		if (Cal [c] . Value < Current)
			// The upper half
			a = c;
		else if (Cal [c] . Value > Current)
			// The lower half
			b = c;
		else
			// The impossible direct hit
			return Cal [c] . Reading;
	}

	// Check the extremes
	if (Current > Cal [b] . Value)
		return Cal [b] . Reading;

	if (Current < Cal [a] . Value)
		return Cal [a] . Reading;

	// Interpolate
	r0 = Cal [a] . Reading;
	r1 = Cal [b] . Reading;
	v0 = Cal [a] . Value;
	v1 = Cal [b] . Value;

	return (int)(r0 + ((r1 - r0) * (Current - v0)) / (v1 - v0) + 0.5);
}

void Oven::update_sensor () {

	int sen = temp_to_reading ();

	enforce_bounds (sen, THERMOCOUPLE_MIN, THERMOCOUPLE_MAX);
	setSensor (sen);
}

// ============================================================================

void Root::buildPlant () {

	double htc, ctc, ine, v;
	int log, i, m;

	// Deafults for the time constants
	htc = DEFAULT_HTC;
	ctc = DEFAULT_CTC;
	ine = DEFAULT_INERTIA;
	// Dafault logging time
	log = 1;

	while (PCArgc--) {
		if (PCArgc == 0)
			bad_args ();
		if (strcmp (*PCArgv, "-h") == 0) {
			// Expect a double
			htc = atof (*(PCArgv+1));
			if (htc < 1.0)
				bad_args ();
		} else if (strcmp (*PCArgv, "-c") == 0) {
			ctc = atof (*(PCArgv+1));
			if (ctc < 1.0)
				bad_args ();
		} else if (strcmp (*PCArgv, "-i") == 0) {
			ine = atof (*(PCArgv+1));
			if (ine < 0.0 || ine > 1.0)
				bad_args ();
		} else if (strcmp (*PCArgv, "-l") == 0) {
			// Expect an unsigned int
			log = atoi (*(PCArgv+1));
			if (log < 0)
				bad_args ();
		} else
			bad_args ();

		PCArgc--;
		PCArgv += 2;
	}

	TheOven = create Oven (htc, ctc, ine);
	if (log)
		create Logger (log);

	// Preprocess the calibration table converting the temperature to
	// normal degrees
	m = -1;
	v = -1.0;
	for (i = 0; i < CalLength; i++) {
		Cal [i] . Value /= 10.0;
		if (Cal [i] . Value <= v || Cal [i] . Reading <= m)
			excptn ("non-monotonic calibration table");
		v = Cal [i] . Value;
		m = Cal [i] . Reading;
	}
}
	
// ============================================================================

Root::perform {

	state Start:

		buildPlant ();
		Kernel->wait (DEATH, Never);

	state Never:

		sleep;
}

// ============================================================================

void Oven::reset () {

	Target = Current = MIN_TEMP;
	Heating = Cooling = NO;
	LastUpdateTime = TIME_0;

	setSensor (THERMOCOUPLE_MIN);
	setActuator (OVEN_MIN);
}

void Oven::response () {

	DeltaT = ituToEtu (Time - LastUpdateTime);
	BaseTime += DeltaT;

	update_current ();
	update_setting ();

	LastUpdateTime = Time;
}

void Logger::setup (int log) {

	NextRunTime = Time;
	Delta = etuToItu (log);
}

Logger::perform {

	state Loop:

		S->showState ();
		NextRunTime += Delta;
		if (NextRunTime > Time)
			Timer->wait (NextRunTime - Time, Loop);
		else
			proceed Loop;
}

void Oven::showState () {

	if (!isConnected ()) {
		logmess ("not connected");
		return;
	}

	logmess ("CT = %6.2f, TT = %6.2f, SET = %4d, RDG = %4d (%c)",
		Current, Target, Actuator, Sensor,
			Heating ? 'h' : (Cooling ? 'c' : '-'));
}
