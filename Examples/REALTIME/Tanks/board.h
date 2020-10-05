/*
	Copyright 1995-2020 Pawel Gburzynski

	This file is part of SMURPH/SIDE.

	SMURPH/SIDE is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	SMURPH/SIDE is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SMURPH/SIDE. If not, see <https://www.gnu.org/licenses/>.
*/

#include "types.h"

#ifndef	__board_h__
#define __board_h__

#define	MAXSPTNK	2	// Maximum # of sensors per tank
#define TGRAN        5000       // Update granularity (5 msec)
#define	MGRAN       10000       // Monitor granularity

station Tank {
  double Fill;
  Sensor *Sensors [MAXSPTNK];
  int NSensors;
  Sensor *findSensor (NetAddress&);
  void addSensor (Sensor*);
  int discreteFill ();
  void setup ();
};

process TankMonitor (Tank) {
  void setup () { };
  states {Check};
  perform;
};

process PumpModel (Pump) {
  Tank *L, *R;
  Actuator *Motor;
  void setup (Tank*, Tank*);
  states {WaitMotor, DoPump};
  perform;
};

void initBoard ();

extern double TankCapacity, FlowRate;
extern double NLevels;
extern Tank **Tanks;
extern PumpModel **PumpModels;

#endif
