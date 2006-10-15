#include "board.h"

static Sensor *BulbSensor;

static Actuator *BulbSwitch;

void Sensor::mapNet () {
  BulbSensor = this;
};

void Actuator::mapNet () {
  BulbSwitch = this;
};

// These are unused stubs:
void Alert::mapNet (const char *t) { t++; };
void Override::mapNet (const char *t) { t++; };

station BulbStation {
  SDSMailbox *STV, *VTS;
  void setup ();
};

observer BulbResponseMonitor {
  TIME MaxResponseTime;
  int LastActuatorValue;
  void setup (double rt) {
    MaxResponseTime = (TIME) (Second * rt);
    LastActuatorValue = BulbSwitch->getValue ();
  };
  states {WaitActuatorStateChange, WaitSensorStateChange, Error};
  perform {
    int nv;
    state WaitActuatorStateChange:
      inspect (ANY, VirtualToSDS, NewVirtualState, WaitSensorStateChange);
    state WaitSensorStateChange:
      if ((nv = BulbSwitch->getValue ()) != LastActuatorValue) {
        LastActuatorValue = nv;
        inspect (ANY, SDSToVirtual, WaitSDSStatus, WaitActuatorStateChange);
        timeout (MaxResponseTime, Error);
      } else
        proceed WaitActuatorStateChange;
    state Error:
      excptn ("Response time too long");
  };
};

void BulbStation::setup () {
  STV = create SDSMailbox;
  VTS = create SDSMailbox;
  create SDSToVirtual (BulbSensor, STV);
  create VirtualToSDS (BulbSwitch, VTS);
};

void initBoard () {
  double rt;
  create BulbStation;
  readIn (rt);
  create BulbResponseMonitor (rt);
};
