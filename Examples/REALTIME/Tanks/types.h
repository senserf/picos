#include "sensor.h"

#ifndef	__types_h__
#define __types_h__

#define	PUMP_LEFT	(-1)
#define	PUMP_RIGHT	( 1)
#define	PUMP_OFF	( 0)

station Pump {
   Sensor *LeftLevelIndicator, *RightLevelIndicator;
   Actuator *Motor;
   void setup (NetAddress&, NetAddress&, NetAddress&);
};

process PumpDriver (Pump) {
  Sensor *LLI, *RLI;
  Actuator *M;
  void setup ();
  states {WaitStatusChange, StatusChange};
  perform;
};

void initSystem ();

extern int NTanks, NPumps;

extern Pump **Pumps;

#endif
