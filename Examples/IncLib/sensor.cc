/*
	Copyright 1995-2018, 2019 Pawel Gburzynski

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

#ifndef	__sensor_c__
#define	__sensor_c__

#include "sensor.h"

void Sensor::setValue (int v) { Value = v; this->put (); };

int Sensor::getValue () { return Value; };

void Sensor::setup (NetAddress &na) {
  Reference = na;
  setLimit (0);
  Value = 0;
  mapNet ();
};

int Actuator::getValue () { return Value; };

void Actuator::setValue (int v) { Value = v; this->put (); };

void Actuator::setup (NetAddress &na, int InitVal) {
  // Note that this time we care about the initial value of the actuator
  Reference = na;
  setLimit (0);
  Value = InitVal;
  mapNet ();
};

void Alert::setup (const char *id) {
  // Report the alert to the system
  setLimit (0);
  Value = NONE;
  mapNet (id);
};

int Alert::getValue () { return Value; };

char *Alert::getMessage () { return txtbuf; };

void Alert::clearValue () { Value = NONE; };

void Override::setup (const char *id) {
  // Report the override to the system
  setLimit (0);
  Action = Value = NONE;
  SerialNumber = 0;
  mapNet (id);
};

void Override::force (int action, int value) {
  Action = action;
  Value = value;
  SerialNumber++;
  this->put ();
};

void Overrideable::setup (const char *id) {
  OSN = 0;
  Reset = create Override (id);
};

int Overrideable::overridePending () {
  if (OSN != Reset->SerialNumber) {
    if (Reset->Action > 0)
      return Reset->Action;
    else
      OSN = Reset->SerialNumber;
  }
  return NO;
};

void readInNetAddress (NetAddress &na) {
  readIn (na.Domain);
  readIn (na.Net);
  readIn (na.Address);
};

#endif
