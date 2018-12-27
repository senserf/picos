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

#include "board.h"

double TankCapacity,  // Litres -- to keep the Americans confused
       FlowRate;      // Pump performance: litres per second

double NLevels;       // Number of discrete sensor levels

double Increment;

Tank **Tanks;

PumpModel **PumpModels;

void Tank::setup () {
  // All tanks start half filled
  Fill = TankCapacity / 2.0;
  NSensors = 0;
};

Sensor *Tank::findSensor (NetAddress &na) {
  for (int i = 0; i < NSensors; i++)
    if (Sensors [i] -> Reference == na) return Sensors [i];
  return NULL;
};

void Tank::addSensor (Sensor *s) {
  Assert (NSensors < MAXSPTNK, "Tank->addSensor: too many sensors per tank");
  Sensors [NSensors++] = s;
  s->setValue (discreteFill ());
};

int Tank::discreteFill () {
  // Calculates the fill discretized into NLevels levels
  return (int) ((NLevels * Fill) / TankCapacity);
};

void PumpModel::setup (Tank *l, Tank *r) {
  L = l;
  R = r;
  Motor = NULL;
};

TankMonitor::perform {
  int nv, i;
  state Check:
    nv = S->discreteFill ();
    for (i = 0; i < S->NSensors; i++)
      if (S->Sensors [i] -> getValue () != nv) S->Sensors [i] -> setValue (nv);
    Timer->wait (MGRAN, Check);
};

PumpModel::perform {
  double dv;
  state WaitMotor:
    if (Motor->getValue () != PUMP_OFF)
      proceed DoPump;
    Motor->wait (UPDATE, WaitMotor);
  state DoPump:
    dv = Increment;
    if (Motor->getValue () == PUMP_LEFT) {
      if (R->Fill < dv) dv = R->Fill;
      R->Fill -= dv;
      if ((L->Fill += dv) > TankCapacity) L->Fill = TankCapacity;
      Timer->wait (TGRAN, DoPump);
      Motor->wait (UPDATE, WaitMotor);
    } else if (Motor->getValue () == PUMP_RIGHT) {
      if (L->Fill < dv) dv = L->Fill;
      L->Fill -= dv;
      if ((R->Fill += dv) > TankCapacity) R->Fill = TankCapacity;
      Timer->wait (TGRAN, DoPump);
      Motor->wait (UPDATE, WaitMotor);
    } else
      proceed WaitMotor;
};
    
void Sensor::mapNet () {
  // These will be called in the order of pumps
  int i;
  Tank *t;
  for (i = 0; i < NTanks; i++) {
    t = Tanks [i];
    if (t->NSensors == 0) break;
    if (t->findSensor (Reference) != NULL) {
      // trace (form ("Sensor %1d, %1d, %1d added to tank %1d\n", 
      //  Reference.Domain, Reference.Net, Reference.Address, i)); Ouf.flush();
      t->addSensor (this);
      return;
    }
  }
  Assert (i < NTanks, "Sensor->mapNet: too many sensors");
  t->addSensor (this);
  // trace (form ("Sensor %1d, %1d, %1d added to tank %1d\n", 
  //   Reference.Domain, Reference.Net, Reference.Address, i)); Ouf.flush();
};

void Actuator::mapNet () {
  // These will be called in the order of pumps
  int i;
  PumpModel *p;
  for (i = 0; i < NPumps; i++)
    if ((p = PumpModels [i]) -> Motor == NULL) {
      p->Motor = this;
      // trace (form ("Actuator %1d, %1d, %1d added to pump %1d\n", 
      //   Reference.Domain, Reference.Net, Reference.Address, i)); Ouf.flush();
      return;
    }
  excptn ("Actuator->mapNet: too many actuators");
};

// These are unused stubs:
void Alert::mapNet (const char *t) { t++; };
void Override::mapNet (const char *t) { t++; };


void initBoard () {
  int i;
  readIn (NPumps);
  NTanks = NPumps + 1;
  readIn (TankCapacity);
  readIn (FlowRate);
  readIn (NLevels);
  Increment = (FlowRate * TGRAN) / Second;
  NLevels += 0.5;
  Tanks = new Tank* [NTanks];
  PumpModels = new PumpModel* [NPumps];
  Tanks [0] = create Tank;
  create TankMonitor;
  for (i = 0; i < NPumps; i++) {
    Tanks [i+1] = create Tank;
    create TankMonitor;
    TheStation = System;
    PumpModels [i] = create PumpModel (Tanks [i], Tanks [i+1]);
  }
};
