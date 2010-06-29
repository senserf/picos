#include "types.h"

int NTanks, NPumps;

Pump **Pumps;

void Pump::setup (NetAddress &ls, NetAddress &rs, NetAddress &m) {
  LeftLevelIndicator  = create Sensor   (ls);
  RightLevelIndicator = create Sensor   (rs);
  Motor               = create Actuator ( m);
  create PumpDriver;
};

void PumpDriver::setup () {
  LLI = S->LeftLevelIndicator;
  RLI = S->RightLevelIndicator;
  M = S->Motor;
};

PumpDriver::perform {
  state WaitStatusChange:
    LLI->wait (UPDATE, StatusChange);
    RLI->wait (UPDATE, StatusChange);
  state StatusChange:
    if (LLI->getValue () < RLI->getValue ())
      M->setValue (PUMP_LEFT);
    else if (LLI->getValue () > RLI->getValue ())
      M->setValue (PUMP_RIGHT);
    else
      M->setValue (PUMP_OFF);
    proceed WaitStatusChange;
};

void initSystem () {
  int i;
  NetAddress a1, a2, a3;
  Pumps = new Pump* [NPumps];
  for (i = 0; i < NPumps; i++) {
    readInNetAddress (a1);
    readInNetAddress (a2);
    readInNetAddress (a3);
    // trace (form ("Creating pump: %1d\n", i)); Ouf.flush ();
    Pumps [i] = create Pump (a1, a2, a3);
  }
};
