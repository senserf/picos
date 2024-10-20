/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "model.h"

identify "A model of a robotic arm";

#include "plant.cc"

// ============================================================================

RArm *TheArm;

static void bad_args () {

	excptn ("bad model-specific arguments,"
		" usage: ... -- [-m mass] [-l logintrvl]");
}

void Root::buildPlant () {

	double mass;
	int log;

	mass = 0.0;
	log = 0;
	
	// Process the arguments: -m mass -l logging_intvl_in_seconds
	while (PCArgc--) {
		if (PCArgc == 0)
			bad_args ();
		if (strcmp (*PCArgv, "-m") == 0) {
			// Expect a double
			mass = atof (*(PCArgv+1));
			if (mass < 0.0)
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

	TheArm = create RArm (mass);
	if (log)
		create Logger (log);
}
	
// ============================================================================

Root::perform {

	state Start:

		// Just build the plant, the rest is taken care of by the
		// generic part
		buildPlant ();
		Kernel->wait (DEATH, Never);

	state Never:

		sleep;
}

// ============================================================================

void RArm::reset () {

	// On reset, we set the arm at position 0.0 with the motor turned off
	Position = 0.0;
	Force = 0.0;
	Speed = 0.0;
	Acceleration = 0.0;
	LastUpdateTime = TIME_0;

	setSensor (SENSOR_MIN);
	setActuator (ACTUATOR_ZERO);
}

void RArm::response () {

	// Called about 100 times per second; set DeltaT to the actual elapsed
	// time, in case we are lagging behind or something
	DeltaT = ituToEtu (Time - LastUpdateTime);

	update_position ();	// Based on the speed so far
	update_sensor ();	// Convey the position to the controller
	set_force ();		// Based on the actuator setting
	set_acceleration ();	// Based on the force, mass, and current speed
	update_speed ();	// Based on the acceleration

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

void RArm::showState () {

	if (!isConnected ()) {
		logmess ("not connected");
		return;
	}

	logmess ("p = %6.3f, s = %6.3f, a = %6.3f, f = %6.3f, A = %4d, S = %4d",
		Position, Speed, Acceleration, Force, Actuator, Sensor);
}
