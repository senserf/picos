#ifndef	__sensor_c__
#define	__sensor_c__

#include "sensor.h"

void Sensor::setValue (int v) { Value = v; this->put (); Pending = YES; };

int Sensor::getValue () { Pending = NO; return Value; };

void Sensor::setup (NetAddress &na) {
  Reference = na;
  setLimit (0);
  Value = 0;
  Pending = NO;
  mapNet ();
};

int Actuator::getValue () { Pending = NO; return Value; };

void Actuator::setValue (int v) { Value = v; this->put (); Pending = YES; };

void Actuator::setup (NetAddress &na, int InitVal) {
  // Note that this time we care about the initial value of the actuator
  Reference = na;
  setLimit (0);
  Value = InitVal;
  Pending = YES;
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
