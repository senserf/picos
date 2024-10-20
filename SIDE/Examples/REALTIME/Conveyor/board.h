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

#include "types.h"

// This version of board.h is for a virtual board

// Connection types
// ------------------------------------------------------------
#define CONN_SS      01   // Segment->segment
#define CONN_SDSS    02   // Segment->diverter->segment,segment
#define CONN_SSMS    03   // Segment,segment->merger->segment

class NetMapEntry {
  public:
    NetAddress Reference;
    int Used;
    int UId, UType;
    union {
      Sensor *SN;
      Actuator *AC;
    };
};

void initBoard ();

packet Box {

    // The standard attributes of Packet are used in the following way:
    //    QTime   - the time the box entered the system
    //    TTime   - the time the box entered the last segment
    //    Sender  - the station Id of the sending SUnit
    //    TP      - box type (traffic pattern Id)
    //    TLength - length in micrometers

    // This way we don't seem to be needing any specific parameters for
    // now, but this may change in the future

    void setup (Message *m) { /* Nothing yet */ };
    void setup ()           { /* For raw create */ };

    TIME processingTime (double);
};

// This mailbox models the conveyor belt at SSegment

mailbox Belt (Box*) {
  TIME MotorStoppedTime,  // The total time during which the motor was stopped
       WhenStopped;       // Time when the motor was last stopped
  DISTANCE EndToEndTime;  // Actual time, not an inflated upper bound
  int Disabled;           // For special actions
  void inItem (Box*);
  DISTANCE discard (int n = NONE);
  int locate (Box*);
  void setup (DISTANCE);
};

process Recipient;

// This is a shadow Unit station responsible for modelling the behaviour of
// a Unit

station SUnit {
  NetAddress  MA;    // Motor address -- for identification
  SUnit *NU;         // Next unit
  Actuator *Motor;   // These will be mirrored from Unit
  Alert *Exception;
  double Speed;
  Process *RP;      // Process receiving stuff from the preceding unit
  void setup (NetAddress&, double);    // As a shadow station
  void setup (int sn=NONE);            // As a source/sink
  // The arguments:
  //                 -- Unit pointer
  //                 -- speed in us/metre, i.e., the inverse of actual speed
};

// This is a shadow Segment station responsible for modelling the
// behaviour of a Segment

station SSegment : SUnit {
  Sensor *In, *Out;       // These will be mirrored from Segment
  Belt *CB;               // The conveyor belt
  void setup (NetAddress&, NetAddress&, NetAddress&, double, double);
  // The arguments:
  //                 -- motor address
  //                 -- entrance sensor address
  //                 -- exit sensor address
  //                 -- length in metres
  //                 -- belt speed in metres/sec
};

// And this is a shadow diverter

station SDiverter :SUnit {
  Sensor *Full, *Divert;
  SUnit *NDU;             // Next unit after divert
  int *DTypes;            // Array of diverted box types
  double ReaderSpeed;
  void setup (NetAddress&, NetAddress&, NetAddress&, double, double);
  // The arguments:
  //                 -- motor address
  //                 -- presence sensor address
  //                 -- reader sensor address
  //                 -- speed in metres/sec
};

station SMerger : SUnit {
  Sensor *In [2], *Out;
  int Senders [2];          // Station Id's of senders
  Belt *Buffer;             // The buffer is modelled as a belt
  DISTANCE BufSize,         // This is in micrometres
           Occupancy;
  double ISpeed;            // Input speed
  void setup (NetAddress&, NetAddress&, NetAddress&, NetAddress&, double,
                                                                double, double);
  // The arguments:
  //                 -- motor address
  //                 -- entrance sensor1 address
  //                 -- entrance sensor2 address
  //                 -- exit sensor address
  //                 -- length in metres (latency)
  //                 -- input speed (boxes enter at this speed, should be
  //                    no less than output speed
  //                 -- output speed in metres/sec (the buffer is emptied at
  //                    this speed)
};

process Recipient : Overrideable (SSegment) {
  Box *CurrentBox;       // The incoming box
  TIME BPT,              // Remaining box processing time
       BPS;              // Time processing started
  Boolean Processing;
  Sensor *In;            // Entrance sensor
  void setup ();
  states {WaitBox, NewBox, ProcessBox, BoxDone, ProcessOverride};
  perform;
};

process Dispatcher : Overrideable (SSegment) {
  Box *CurrentBox;       // The current box
  Sensor *Out;           // Exit sensor
  Belt *CB;
  TIME BPT,              // Remaining box processing time
       BPS;              // Time processing started
  Boolean Processing;
  void setup ();
  states {GetBox, ProcessBox, BoxDone, ProcessOverride};
  perform;
};

process MRecipient : Overrideable (SMerger) {
  Box *CurrentBox;           // The incoming box
  TIME BPT,                  // Remaining box processing time
       BPS;                  // Time processing started
  Boolean Processing;
  Sensor **Ins,              // Entrance sensors
         *In;                // Current entrance sensor
  void setup ();
  states {WaitBox, NewBox, ProcessBox, BoxDone, ProcessOverride};
  perform;
};

process MDispatcher : Overrideable (SMerger) {
  Box *CurrentBox;       // The current box
  Sensor *Out;           // Exit sensor
  Belt *Buffer;
  TIME BPT,              // Remaining box processing time
       BPS;              // Time processing started
  Boolean Processing;
  void setup ();
  states {GetBox, ProcessBox, BoxDone, ProcessOverride};
  perform;
};


process MotorMonitor (SSegment) {
  Belt *CB;
  Actuator *Motor;
  int LastState;
  void setup (Belt*, Actuator*);
  states {WaitStateChange};
  perform;
};

process Router : Overrideable (SDiverter) {
  Box *CurrentBox;       // The box being handled
  Sensor *Full, *Divert; // Copied from Diverter
  SUnit *OU;             // The actual output unit
  TIME RRT, DCT;
  void setup ();
  states {WaitBox, NewBox, ProcessReader, ReaderDone, ProcessDivert,
                                                   DivertDone, ProcessOverride};
  perform;
};

extern SSegment **SSegments;
extern SDiverter **SDiverters;
extern int NSSegments, NSDiverters;

NetMapEntry *netMapLookup (NetAddress&, int book = YES);

int idToUId (int, int&);

Station *uidToStation (int, int);

int shadow (int, int);
