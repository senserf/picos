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
