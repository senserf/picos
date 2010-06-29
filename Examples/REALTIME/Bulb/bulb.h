#ifndef __bulb_h__
#define __bulb_h__

#include "sensor.h"
#include "board.h"

#define BOARD                   0   // Board number
#define BUS                     1   // Bus number
#define BULB_SENSOR_ID          1   // Sensor ID
#define BULB_ACTUATOR_ID        2   // Actuator ID

station Bulb {
  TIME Timeout;
  Sensor *BulbMonitor;
  Actuator *BulbSwitch;
  void setup (double);
};

process BulbProcess (Bulb) {
  states {WaitBulbStateChange, TimerGoesOff};
  perform;
};

void initSystem ();

#endif
