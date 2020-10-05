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

#include "sensor.h"

// #define DEBUG           1

// Unit types
// ------------------------
#define SEGMENT           0
#define DIVERTER          1
#define SSEGMENT          2
#define SDIVERTER         3
#define SOURCE            4
#define SINK              5
#define MERGER            6
#define SMERGER           7

#define	TheValue	((int)(IPointer)TheSignal)

process MotorDriver;    // Just announcing
process SensorDriver;

station Unit {
  Actuator *Motor;      // The motor switch
  MotorDriver *MD;
  Alert *Exception;     // Mailbox For sending alerts to the operator
  void setup (NetAddress&, double);
  // This is the base of all stations in our system. We assume that each
  // station is equipped with at least one motor (and possibly a number
  // of sensors).
  // The two arguments of the setup method specify the network coordinates
  // of the motor switch and the motor inertia, i.e., the amount of time
  // during which its status shouldn't change.
};

station Segment : Unit {
  // This station represents a segment of the conveyor belt. Such a segment
  // is equipped with one sensor at the entrance, one sensor at the exit,
  // and one motor driving the belt.
  Sensor *In, *Out;     // Entry and exit
  SensorDriver *SDIn, *SDOut;
  int BoxesInTransit;   // Number of items in transit
  void setup (NetAddress&, double, NetAddress&, double, NetAddress&,
                                                                double, double);
  // The arguments are:
  //      -- network coordinates of the motor switch (for Unit)
  //      -- motor inertia (for Unit)
  //      -- network coordinates of the entrance sensor
  //      -- entrance sensor inertia
  //      -- network coordinates of the exit sensor
  //      -- exit sensor inertia
  //      -- maximum end-to-end trip time (upper bound)
};

station Diverter : Unit {
  // This is a diverter station. It is equipped with a motor and a pair
  // of sensors. One sensor tells whether an object is present, the other
  // indicates whether it should be diverted.
  Sensor *Full, *Divert;
  SensorDriver *SDFull;
  // We don't need a driver for the second sensor, because we assume that
  // it is only a reader that doesn't trigger any events on its own.
  void setup (NetAddress&, double, NetAddress&, double, NetAddress&, double);
  // The arguments:
  //      -- motor parameters for Unit
  //      --
  //      -- network coordinates of the (Full) entrance sensor
  //      -- sensor inertia (Full)
  //      -- network coordinates of the reader (Divert) sensor
  //      -- reader latency, i.e., the amount of time elapsing from the moment
  //         we detect the presence of an object, until the reader (Divert) is
  //         ready with its decision
};

station Merger : Unit {
  Sensor *In1, *In2, *Out;
  SensorDriver *SDIn1, *SDIn2, *SDOut;
  int BoxesInTransit;
  void setup (NetAddress&, double, NetAddress&, double, NetAddress&, double,
                                                   NetAddress&, double, double);
  // The arguments are:
  //      -- network coordinates of the motor switch (for Unit)
  //      -- motor inertia (for Unit)
  //      -- network coordinates of the entrance sensor 1
  //      -- entrance sensor 1 inertia
  //      -- network coordinates of the entrance sensor 2
  //      -- entrance sensor 2 inertia
  //      -- network coordinates of the exit sensor
  //      -- maximum relay latency
};

process MotorDriver (Unit) {
  Actuator *Motor;
  TIME Inertia;
  void setup (Actuator*, double);
  states {StatusWait, StatusChange};   // We get away with a single state
  perform;
};

process SensorDriver (Unit) {
  Sensor *Sense;
  int LastValue;
  TIME Inertia, Resume;
  void setup (Sensor*, double);
  states {StatusChange, WaitResume};
  perform;
};

process SegmentDriver : Overrideable (Segment) {
  MotorDriver *MD;
  SensorDriver *SDIn, *SDOut;
  Alert *Exception;
  TIME LastOutTime, EndToEndTime;
  void setup (double);
  states {WaitSensor, Input, Output, ProcessOverride};
  perform;
};

process MergerDriver : Overrideable (Merger) {
  MotorDriver *MD;
  SensorDriver *SDIn1, *SDIn2, *SDOut;
  Alert *Exception;
  TIME LastOutTime, EndToEndTime;
  void setup (double);
  states {WaitSensor, Input, Output, ProcessOverride};
  perform;
};

process DiverterDriver : Overrideable (Diverter) {
  MotorDriver *MD;
  SensorDriver *SDFull;
  Sensor *Divert;
  Alert *Exception;
  TIME DivertTime;
  void setup (double);
  states {WaitSensor, Input, DivertDecision, EndDivert, ProcessOverride};
  perform;
};

void initSystem ();

extern int NSegments, NDiverters, NMergers;

extern Segment  **Segments;
extern Diverter **Diverters;
extern Merger   **Mergers;
