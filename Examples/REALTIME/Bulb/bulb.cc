#include "bulb.h"

void Bulb::setup (double tmout) {
  NetAddress NaS, NaA;
  Timeout = (TIME) (tmout * Second);
  NaS . Domain = NaA . Domain = BOARD;
  NaS . Net = NaA . Net = BUS;
  NaS . Address = BULB_SENSOR_ID;
  NaA . Address = BULB_ACTUATOR_ID;
  BulbMonitor = create Sensor (NaS);
  BulbSwitch = create Actuator (NaA);
  create BulbProcess;
};

BulbProcess::perform {
  // This process makes sure that the bulb is never on for more than the
  // timeout
  state WaitBulbStateChange:
    if (S->BulbMonitor->getValue () == SENSOR_ON) {
      Timer->wait (S->Timeout, TimerGoesOff);
    }
    S->BulbMonitor->wait (UPDATE, WaitBulbStateChange);
  state TimerGoesOff:
    S->BulbSwitch->setValue (ACTUATOR_OFF);
    proceed WaitBulbStateChange;
};

void initSystem () {
  double tm;
  readIn (tm);
  create Bulb (tm);
};
