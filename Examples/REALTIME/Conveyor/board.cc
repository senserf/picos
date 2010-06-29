#include "io.h"

static NetMapEntry *NetMap;

static	int	NNetMapEntries = 0,
      		NetMapMaxSize  = 0;

#define NETMAPINITSIZE 256           // Initial size of the netmap

SSegment  **SSegments;
SDiverter **SDiverters;
SMerger   **SMergers;

// These three will end up being the same as NSegments, NDiverters, and NMergers
int NSSegments = 0, NSDiverters = 0, NSMergers = 0;;

static int NConnections;

// ------------------------------------------------------------------- //
// This function converts a SMURPH station Id into a Unit Id/type pair //
// ------------------------------------------------------------------- //
int idToUId (int sid, int &utype) {
  int i;
  utype = SEGMENT;
  for (i = 0; i < NSegments; i++)
    if (Segments [i] -> getId () == sid) return i;
  utype = DIVERTER;
  for (i = 0; i < NDiverters; i++)
    if (Diverters [i] -> getId () == sid) return i;
  utype = MERGER;
  for (i = 0; i < NMergers; i++)
    if (Mergers [i] -> getId () == sid) return i;
  utype = SSEGMENT;
  for (i = 0; i < NSSegments; i++)
    if (SSegments [i] -> getId () == sid) return i;
  utype = SDIVERTER;
  for (i = 0; i < NSDiverters; i++)
    if (SDiverters [i] -> getId () == sid) return i;
  utype = SMERGER;
  for (i = 0; i < NSMergers; i++)
    if (SMergers [i] -> getId () == sid) return i;
  utype = SOURCE;
  for (i = 0; i < NSources; i++)
    if (Sources [i] -> getId () == sid) return i;
  utype = SINK;
  for (i = 0; i < NSinks; i++)
    if (Sinks [i] -> getId () == sid) return i;
  return NONE;
};

// ------------------------------------------------------------------ //
// This function locates the station, given its unit Id and unit type //
// ------------------------------------------------------------------ //
Station *uidToStation (int uid, int utype) {

  if (uid < 0) return NULL;
  switch (utype) {
    case SEGMENT:
      if (uid >= NSegments) return NULL;
      return Segments [uid];
    case DIVERTER:
      if (uid >= NDiverters) return NULL;
      return Diverters [uid];
    case MERGER:
      if (uid >= NMergers) return NULL;
      return Mergers [uid];
    case SSEGMENT:
      if (uid >= NSSegments) return NULL;
      return SSegments [uid];
    case SDIVERTER:
      if (uid >= NSDiverters) return NULL;
      return SDiverters [uid];
    case SMERGER:
      if (uid >= NSMergers) return NULL;
      return SMergers [uid];
    case SOURCE:
      if (uid >= NSources) return NULL;
      return Sources [uid];
    case SINK:
      if (uid >= NSinks) return NULL;
      return Sinks [uid];
    default:
      return NULL;
  }
};

// -------------------------------------------------------------------- //
// This function returns the (internal) id of the station shadowing the //
// specified station                                                    //
// -------------------------------------------------------------------- //
int shadow (int uid, int utype) {
  Actuator *Motor;
  Unit *s;
  int NStats, i;
  SUnit **Stats;
  
  s = (Unit*) uidToStation (uid, utype);
  if (s == NULL) return NONE;
  Motor = s -> Motor;
  // Lookup the same motor among shadow units
  switch (utype) {
    case SEGMENT:
      Stats = (SUnit**)SSegments;
      NStats = NSSegments;
      break;
    case DIVERTER:
      Stats = (SUnit**)SDiverters;
      NStats = NSDiverters;
      break;
    case MERGER:
      Stats = (SUnit**)SMergers;
      NStats = NSMergers;
      break;
    default:
      return NONE;
  }
  for (i = 0; i < NStats; i++)
    if (Stats [i]->Motor == Motor) return i;
  return NONE;
};
  
static void addMapEntry (int id, void *sn, NetAddress &na) {
  NetMapEntry *nm;
  int i, ns;
  if (NetMapMaxSize <= NNetMapEntries) {
    // Grow the netmap
    if (NetMapMaxSize == 0)
      ns = NETMAPINITSIZE;
    else
      ns = NetMapMaxSize + NetMapMaxSize;
    nm = NetMap;     // Save the old map for a while
    NetMap = new NetMapEntry [ns];
    for (i = 0; i < NetMapMaxSize; i++) NetMap [i] = nm [i];
    if (NetMapMaxSize) delete (nm);
    NetMapMaxSize = ns;
  }
  NetMap [NNetMapEntries] . Used = NO;
  NetMap [NNetMapEntries] . Reference = na;
  NetMap [NNetMapEntries] . SN = (Sensor*) sn;

  if ((i = idToUId (id, ns)) == NONE)
    excptn ("addMapEntry: Unit Id mismatch");

  NetMap [NNetMapEntries] . UId = i;
  NetMap [NNetMapEntries] . UType = ns;

  NNetMapEntries++;
};

NetMapEntry *netMapLookup (NetAddress &na, int book) {
  int i;
  for (i = 0; i < NNetMapEntries; i++)
    if (NetMap [i] . Reference == na && (!book || !(NetMap [i] . Used))) {
      if (book) NetMap [i] . Used = YES;
      return NetMap + i;
    }
  return NULL;
};
    
// Report a sensor/actuator to the virtual network
void Sensor::mapNet () {
  Value = SENSOR_OFF;
  addMapEntry (TheStation->getId (), (void*)this, Reference);
};

void Actuator::mapNet () {
  addMapEntry (TheStation->getId (), (void*)this, Reference);
};

TIME Box::processingTime (double speed) {
  // Calculates the box passage time for the specified speed
  return (TIME) (TLength * speed);
}

void Belt::setup (DISTANCE eet) {
  setLimit (MAX_Long);
  EndToEndTime = eet;
  MotorStoppedTime = WhenStopped = TIME_0;
  Disabled = NO;
};

void Belt::inItem (Box *box) {
  // Called automatically when a box enters the belt
  if (Disabled) return; 
  // Occasionally, we use put and get for some counting operations. Then
  // we disable this, which should only be executed for a true put.
  box->TTime = (Time + EndToEndTime) - MotorStoppedTime;
  if (((SUnit*)TheStation)->Motor->getValue () == MOTOR_OFF)
    box->TTime -= (Time - WhenStopped);
  // This is used to tell when to remove the box from the other end.
  // The process responsible for that will add MotorStoppedTime to
  // the time stamp of the frontmost box and this will give the time
  // when the box is formally supposed to arrive there. This will work
  // if MotorStoppedTime has grown in the meantime, i.e., the belt was
  // stopped.
};

DISTANCE Belt::discard (int n) {
  // Discards the n-th box from the mailbox. This is more tricky than it
  // has to be, because we don't want to modify the SMURPH's concept of
  // what a mailbox is
  Box *b, *h;
  int i, k;
  DISTANCE length;
  length = 0;
  if (n == NONE) {
     // Everything -- this part is actually quite straightforward
     while (b = get ()) {
       length += b->TLength;
       delete (b);
     }
     return length;
  }
  Disabled = YES;    // Avoid inItem processing
  if ((k = getCount ()) <= n) {
    // Not that many items in the mailbox
    Disabled = NO;
    return 0;
  }
  for (i = 0; i < k; i++) {
    b = get ();
    if (i == n) {
      length += b->TLength;
      delete (b);
    } else
      put (b);
  }
  Disabled = NO;
  return length;
};

int Belt::locate (Box *bx) {
  // Gives the location of box bx on the belt
  Box *b;
  int i, k, n;
  Disabled = YES;    // Switch off inItem processing
  k = getCount ();
  for (i = 0, n = NONE; i < k; i++) {
    if ((b = get ()) == bx) n = i;
    put (b);
  }
  Disabled = NO;
  return n;
};

void SUnit::setup (NetAddress &na, double sp) {
  NetMapEntry *nme;
  NU = NULL;
  if ((nme = netMapLookup (na)) == NULL)
    excptn ("SUnit->setup: Motor NetMapEntry not found");
  Motor = nme -> AC;
  MA = na;
  Speed = 1.0/sp;             // us/umetre (or seconds / metre)
  Exception = create Alert (form (
    "+SUnit@M(%1d,%1d,%1d)", MA.Domain, MA.Net, MA.Address));
};

void SUnit::setup (int sn) {
  // This is used when the station is created as a traffic source
  if (sn != NONE) {
    NU = SSegments [sn];
    Speed = NU->Speed;      // Equal to the speed of receiving belt
  } else {
    NU = NULL;
    Speed = 0.0;            // Will be set elsewhere
  }
  Motor = NULL;
  RP = NULL;                // No preceding Unit
};

void SSegment::setup (NetAddress &mo, NetAddress &in, NetAddress &ou,
                                                 double length, double speed) {
  DISTANCE EET;
  NetMapEntry *nme;
  SSegments [NSSegments++] = this;
  SUnit::setup (mo, speed);
  if ((nme = netMapLookup (in)) == NULL)
    excptn ("SSegment->setup: In NetMapEntry not found");
  In = nme -> SN;
  if ((nme = netMapLookup (ou)) == NULL)
    excptn ("SSegment->setup: Out NetMapEntry not found");
  Out = nme -> SN;
  EET = (DISTANCE) (Second * (length / speed));  // End-to-end time
  CB = create Belt (EET);
  create MotorMonitor (CB, Motor);
  RP = create Recipient;
  create Dispatcher;
};

void SDiverter::setup (NetAddress &mo, NetAddress &fu, NetAddress &rd,
                                                double speed, double rdspeed) {
  NetMapEntry *nme;
  SDiverters [NSDiverters++] = this;
  SUnit::setup (mo, speed);
  ReaderSpeed = 1.0/rdspeed;    // us/um
  NDU = NULL;
  if ((nme = netMapLookup (fu)) == NULL)
    excptn ("SDiverter->setup: Full NetMapEntry not found");
  Full = nme -> SN;
  if ((nme = netMapLookup (rd)) == NULL)
    excptn ("SDiverter->setup: Divert NetMapEntry not found");
  Divert = nme -> SN;
  RP = create Router;
};

void SMerger::setup (NetAddress &mo, NetAddress &in1, NetAddress &in2,
                  NetAddress &ou, double length, double ispeed, double ospeed) {
  DISTANCE EET;
  NetMapEntry *nme;
  SMergers [NSMergers++] = this;
  SUnit::setup (mo, ospeed);
  if ((nme = netMapLookup (in1)) == NULL)
    excptn ("SMerger->setup: In1 NetMapEntry not found");
  In [0] = nme -> SN;
  if ((nme = netMapLookup (in2)) == NULL)
    excptn ("SMerger->setup: In2 NetMapEntry not found");
  In [1] = nme -> SN;
  if ((nme = netMapLookup (ou)) == NULL)
    excptn ("SMerger->setup: Out NetMapEntry not found");
  Out = nme -> SN;
  EET = (DISTANCE) (Second * (length / ispeed));  // End-to-end time
  ISpeed = 1.0/ispeed;
  Buffer = create Belt (EET);
  BufSize = (DISTANCE)(length * Second);         // Buffer size in um
  Occupancy = 0;
  create MotorMonitor (Buffer, Motor);
  // These will be set upon connection
  Senders [0] = Senders [1] = NONE;
  RP   = create MRecipient;
  create MDispatcher;
};

void MDispatcher::setup () {
  Overrideable::setup (form ("+SDsM@M(%1d,%1d,%1d)",
    S->MA.Domain, S->MA.Net, S->MA.Address)); 
  Out = S->Out;
  Buffer = S->Buffer;
  CurrentBox = NULL;
};

void MRecipient::setup () {
  Overrideable::setup (form ("+SRcM@M(%1d,%1d,%1d)",
    S->MA.Domain, S->MA.Net, S->MA.Address)); 
  CurrentBox = NULL;
  Ins = S->In;
};

void MotorMonitor::setup (Belt *b, Actuator *a) {
  CB = b;
  Motor = a;
  LastState = Motor->getValue ();
};

void Recipient::setup () {
  Overrideable::setup (form ("+SRcp@M(%1d,%1d,%1d)",
    S->MA.Domain, S->MA.Net, S->MA.Address));
  In = S->In;
  CurrentBox = NULL;
};

void Dispatcher::setup () {
  Overrideable::setup (form ("+SDsp@M(%1d,%1d,%1d)",
    S->MA.Domain, S->MA.Net, S->MA.Address));
  Out = S->Out;
  CB = S->CB;
};

void Router::setup () {
  Overrideable::setup (form ("+SRtr@M(%1d,%1d,%1d)",
    S->MA.Domain, S->MA.Net, S->MA.Address));
  Full = S->Full;
  Divert = S->Divert;
};

MotorMonitor::perform {
  int ns;
  state WaitStateChange:
    if ((ns = Motor->getValue ()) != LastState) {
      if ((LastState = ns) == MOTOR_ON)
        CB->MotorStoppedTime += (Time - CB->WhenStopped);
      CB->WhenStopped = Time;
    }
    Motor->wait (NEWITEM, WaitStateChange, URGENT);
};
    
Recipient::perform {

  state WaitBox:

    // We will get here only if we are done with the previous box; thus
    // boxes stacked at our entrance will jam the preceding unit

    onOverride (ProcessOverride);
    this->wait (SIGNAL, NewBox);

  state NewBox:

    CurrentBox = (Box*) TheSignal;
    // Trigger the sensor
    In->setValue (SENSOR_ON);
    // Determine for how long the sensor will remain on
    BPT = CurrentBox->processingTime (S->Speed);
    // Put the box onto the belt
    S->CB->put (CurrentBox);
    BPS = Time;
    Processing = YES;

  transient ProcessBox:

    if (Processing) BPT -= (Time - BPS);  // Keep track of progress

    if (S->Motor->getValue () == MOTOR_OFF) {
      Processing = NO;
      // We stop processing imediately when the motor goes off
    } else {
      // The motor is on -- continue processing
      Processing = YES;
      BPS = Time;
      Timer->wait (BPT, BoxDone);
    }

    onOverride (ProcessOverride);

    S->Motor->wait (NEWITEM, ProcessBox);

  state BoxDone:

    CurrentBox = NULL;
    // The sensor goes off
    In->setValue (SENSOR_OFF);
    proceed WaitBox;

  state ProcessOverride:

    overrideAcknowledge ();
    switch (overrideAction ()) {
      case OVR_CLEAR:
        S->CB->discard (overrideValue ());
        if (overrideValue () == NONE) {
          CurrentBox = NULL;
          proceed WaitBox;
        } 
        // Selective discard
        if (CurrentBox) {
          // Check if current box has been deleted
          if (S->CB->locate (CurrentBox) == NONE) {
            CurrentBox = NULL;
            proceed WaitBox;
          }
          // It's still there -- continue processing it
          proceed ProcessBox;
        }
        proceed WaitBox;
      default:
        // Do nothing
        if (CurrentBox)
          proceed ProcessBox;
        else
          proceed WaitBox;
    }
};

Dispatcher::perform {

  TIME RTime;

  state GetBox:

    if (S->Motor->getValue () == MOTOR_OFF) {
      // Don't do anything if the motor is OFF
      S->Motor->wait (NEWITEM, GetBox);
      sleep;
    }
    // Look at the frontmost box on the belt
    if ((CurrentBox = CB->first ()) == NULL) {
      CB->wait (NEWITEM, GetBox);
      sleep;
    }
    // Determine when the box should be rightfully received
    if ((RTime = CurrentBox->TTime + CB->MotorStoppedTime) > Time) {
      Timer->wait (RTime - Time, GetBox);
      sleep;
    }
    // OK, we are ready to handle the box
    Out->setValue (SENSOR_ON);
    Processing = YES;
    // Determine for how long the sensor will remain on
    BPT = CurrentBox->processingTime (S->Speed);
    BPS = Time;

  transient ProcessBox:

    if (CB->first () != CurrentBox) {
      // The box has been removed by Recipient override operation
      Out->setValue (SENSOR_OFF);
      proceed GetBox;
    }
    if (Processing) BPT -= (Time - BPS);  // How far we have processed the box
    if (S->Motor->getValue () == MOTOR_ON) {
      Processing = YES;
      BPS = Time;
      Timer->wait (BPT, BoxDone);
    } else
      Processing = NO;
    S->Motor->wait (NEWITEM, ProcessBox);
    CB->wait (PUT, ProcessBox);
    CB->wait (GET, ProcessBox);
    // We are monitoring all belt events that may result from Recpient's
    // override. This way we will learn when CurrentBox disappears from
    // the belt because of an override.

  state BoxDone:

    if (CB->first () != CurrentBox) proceed ProcessBox;  // One last chance
    CB->get ();  // Remove the box from the belt
    // Notify the next recipient
    if (S->NU->RP -> signal (CurrentBox) == REJECTED) {
      // We have a (possibly temporary) jam -- the sensor remains ON
      S->NU->RP -> wait (CLEAR, BoxDone);
      onOverride (ProcessOverride);
      S->Exception->notify (X_WARNING, "+Jam at Dispatcher");
      sleep;
    }
    Out->setValue (SENSOR_OFF);
    proceed GetBox;

  state ProcessOverride:

    overrideAcknowledge ();
    switch (overrideAction ()) {
      case OVR_RESUME:
        proceed BoxDone;
      case OVR_CLEAR:
        // We have to pass this to the next Recipient
        ((Overrideable*)(S->NU->RP)) -> Reset ->
                                            force (OVR_CLEAR, overrideValue ());
        proceed BoxDone, LOW;
    }
};

Router::perform {
  int i;

  state WaitBox:

    this->wait (SIGNAL, NewBox);
    OU = NULL;
    RRT = DCT = TIME_0;
    onOverride (ProcessOverride);

  state NewBox:

    CurrentBox = (Box*) TheSignal;
    // Trigger the sensor
    Full->setValue (SENSOR_ON);
    // Determine when the reader is going to respond
    RRT = CurrentBox->processingTime (S->ReaderSpeed) + Time;
    // And how much time we need to divert the box
    DCT = CurrentBox->processingTime (S->Speed) + Time;
    // Note: we assume that RRT < DCT

  transient ProcessReader:

    Timer->wait (RRT - Time, ReaderDone);
    onOverride (ProcessOverride);

  state ReaderDone:

    for (i = 0; S->DTypes [i] != NONE; i++)
      if (S->DTypes [i] == CurrentBox->TP)
        break;
    Divert->setValue (S->DTypes [i] == NONE ? SENSOR_OFF : SENSOR_ON);
    OU = S->NU;   // Default output unit
  
  transient ProcessDivert:

    Timer->wait (DCT-Time, DivertDone);
    onOverride (ProcessOverride);
    // If the motor is run in the meantime, we divert the box
    if (S->Motor->getValue () == MOTOR_ON)
      OU = S->NDU;                        // Divert unit
    else
      S->Motor->wait (NEWITEM, ProcessDivert);

  state DivertDone:

    // Notify the next recipient
    if (OU->RP -> signal (CurrentBox) == REJECTED) {
      // A jam
      OU->RP -> wait (CLEAR, DivertDone);
      onOverride (ProcessOverride);
      S->Exception->notify (X_WARNING, "+Jam at Router");
      sleep;
    }
    OU = NULL;
    Full->setValue (SENSOR_OFF);
    proceed WaitBox;

  state ProcessOverride:

    overrideAcknowledge ();
    switch (overrideAction ()) {
      case OVR_RESUME:
        break;
      case OVR_CLEAR:
        // We have to pass this to the next Recipient
        if (OU != NULL)
          ((Overrideable*)(OU->RP)) -> Reset ->
                                            force (OVR_CLEAR, overrideValue ());
        if (CurrentBox) delete CurrentBox;
        CurrentBox = NULL;
        proceed WaitBox, LOW;
    }
    // Determine where to return
    if (RRT == TIME_0) proceed WaitBox, LOW;
    if (Time <= RRT) proceed ProcessReader;
    if (Time <= DCT) proceed ProcessDivert;
    if (OU != NULL) proceed DivertDone;
    proceed WaitBox, LOW;
};

MRecipient::perform {

// This is a single process servicing both input ports of a merger station

  int SenderId;

  state WaitBox:

    this->wait (SIGNAL, NewBox);
    onOverride (ProcessOverride);

  state NewBox:

    CurrentBox = (Box*) TheSignal;
    // Determine the port to which the box is addressed
    SenderId = (TheSender->getOwner ())->getId ();
    // This is the absolute station Id
    if (SenderId == S->Senders [0]) {
      In = Ins [0];
    } else if (SenderId == S->Senders [1]) {
      In = Ins [1];
    } else if (SenderId != getId ()) {
      // If we are the sender, it means that the values have been set
      // already
      excptn ("MEntry: illegal sender");
    }

    // Trigger the sensor
    In->setValue (SENSOR_ON);
    if (S->BufSize - S->Occupancy < CurrentBox->TLength) {
      this->signal (TheSignal);  // Keep the repository full
      CurrentBox == NULL;
      S->Buffer->wait (GET, WaitBox, HIGH);
      onOverride (ProcessOverride);
      sleep;
    }
    S->Occupancy += CurrentBox->TLength;
    BPT = CurrentBox->processingTime (S->ISpeed);
    S->Buffer->put (CurrentBox);
    BPS = Time;
    Processing = YES;

  transient ProcessBox:

    if (Processing) BPT -= (Time - BPS);  // Keep track of progress
    if (S->Motor->getValue () == MOTOR_OFF) {
      Processing = NO;
      // We stop processing imediately when the motor goes off
    } else {
      // The motor is on -- continue processing
      Processing = YES;
      BPS = Time;
      Timer->wait (BPT, BoxDone);
    }

    onOverride (ProcessOverride);

    S->Motor->wait (NEWITEM, ProcessBox);

  state BoxDone:

    CurrentBox = NULL;
    // The sensor goes off
    In->setValue (SENSOR_OFF);
    proceed WaitBox;

  state ProcessOverride:

    overrideAcknowledge ();
    switch (overrideAction ()) {
      case OVR_CLEAR:
        S->Occupancy -= S->Buffer->discard (overrideValue ());
        if (overrideValue () == NONE) {
          CurrentBox = NULL;
          proceed WaitBox;
        } 
        // Selective discard
        if (CurrentBox) {
          // Check if current box has been deleted
          if (S->Buffer->locate (CurrentBox) == NONE) {
            CurrentBox = NULL;
            proceed WaitBox;
          }
          // It's still there -- continue processing it
          proceed ProcessBox;
        }
        proceed WaitBox;
      default:
        // Do nothing
        if (CurrentBox)
          proceed ProcessBox;
        else
          proceed WaitBox;
    }
};

MDispatcher::perform {

  TIME RTime;

  // Note: at the moment, this process is exactly the same as Segment
  // dispatcher, except for one statement at state BoxDone

  state GetBox:

    if (S->Motor->getValue () == MOTOR_OFF) {
      // Don't do anything if the motor is OFF
      S->Motor->wait (NEWITEM, GetBox);
      sleep;
    }
    // Look at the frontmost box on the belt
    if ((CurrentBox = Buffer->first ()) == NULL) {
      Buffer->wait (NEWITEM, GetBox);
      sleep;
    }
    // Determine when the box should be rightfully received
    if ((RTime = CurrentBox->TTime + Buffer->MotorStoppedTime) > Time) {
      Timer->wait (RTime - Time, GetBox);
      sleep;
    }
    // OK, we are ready to handle the box
    Out->setValue (SENSOR_ON);
    Processing = YES;
    // Determine for how long the sensor will remain on
    BPT = CurrentBox->processingTime (S->Speed);
    BPS = Time;

  transient ProcessBox:

    if (Buffer->first () != CurrentBox) {
      // The box has been removed by Recipient override operation
      Out->setValue (SENSOR_OFF);
      proceed GetBox;
    }
    if (Processing) BPT -= (Time - BPS);  // How far we have processed the box
    if (S->Motor->getValue () == MOTOR_ON) {
      Processing = YES;
      BPS = Time;
      Timer->wait (BPT, BoxDone);
    } else
      Processing = NO;
    S->Motor->wait (NEWITEM, ProcessBox);
    Buffer->wait (PUT, ProcessBox);
    Buffer->wait (GET, ProcessBox);

  state BoxDone:

    if (Buffer->first () != CurrentBox) proceed ProcessBox;  // One last chance
    Buffer->get ();  // Remove the box from the belt
    // The following statement makes the process different from Segment's
    // dispatcher
    S->Occupancy -= CurrentBox->TLength;
    // Notify the next recipient
    if (S->NU->RP -> signal (CurrentBox) == REJECTED) {
      // We have a (possibly temporary) jam -- the sensor remains ON
      S->NU->RP -> wait (CLEAR, BoxDone);
      onOverride (ProcessOverride);
      S->Exception->notify (X_WARNING, "+Jam at Dispatcher");
      sleep;
    }
    Out->setValue (SENSOR_OFF);
    proceed GetBox;

  state ProcessOverride:

    overrideAcknowledge ();
    switch (overrideAction ()) {
      case OVR_RESUME:
        proceed BoxDone;
      case OVR_CLEAR:
        // We have to pass this to the next Recipient
        ((Overrideable*)(S->NU->RP)) -> Reset ->
                                            force (OVR_CLEAR, overrideValue ());
        proceed BoxDone, LOW;
    }
};
  
void initBoard () {
  // This is called after initSystem, so the primary stations and
  // processes are already there
  double length, speed, speed1;
  NetAddress a1, a2, a3, a4;
  int CType, i, s1, s2, s3, d1;
  TheStation = System;

  if (NSegments) SSegments = new SSegment* [NSegments];
  if (NDiverters) SDiverters = new SDiverter* [NDiverters];
  if (NMergers) SMergers = new SMerger* [NMergers];

  for (i = 0; i < NSegments; i++) {
    // Create shadow segments
    readInNetAddress (a1);   // The motor
    readInNetAddress (a2);   // In sensor
    readInNetAddress (a3);   // Out sensor
    readIn (length);
    readIn (speed);
    create SSegment, form ("SSgm#%1d", i), (a1, a2, a3, length, speed);
  }
  for (i = 0; i < NDiverters; i++) {
    // Create shadow diverters
    readInNetAddress (a1);   // The motor
    readInNetAddress (a2);   // Full sensor
    readInNetAddress (a3);   // The reader
    readIn (speed);
    readIn (length);  // This is the reader speed
    Assert (speed < length, "initBoard: readerSpeed <= routing speed");
    create SDiverter, form ("SDvt#%1d", i), (a1, a2, a3, speed, length);
  }
  for (i = 0; i < NMergers; i++) {
    // Create shadow mergers
    readInNetAddress (a1);   // The motor
    readInNetAddress (a2);   // First In sensor
    readInNetAddress (a3);   // Second In sensor
    readInNetAddress (a4);   // Out sensor
    readIn (length);
    readIn (speed);          // Input speed
    readIn (speed1);         // Output speed
    Assert (speed >= speed1, "initBoard: input speed < output speed");
    create SMerger, form ("SMrg#%1d", i), (a1, a2, a3, a4, length, speed,
                                                                        speed1);
  }

  // Now we are going to connect the Units
  readIn (NConnections);   
  for (i = 0; i < NConnections; i++) {
    readIn (CType);
    switch (CType) {
      case CONN_SS:
        // Segment to segment
        readIn (s1);   // The first segment
        readIn (s2);   // The second segment
        Assert (s1 >= 0 && s1 < NSSegments && s2 >= 0 && s2 < NSSegments,
                                           "initBoard: illegal segment number");
        Assert (SSegments [s1] -> NU == NULL,
                                        "initBoard: segment already connected");
        SSegments [s1] -> NU = SSegments [s2];
        break;
      case CONN_SDSS:
        // Segment diverter segment,segment
        readIn (s1);   // The segment
        readIn (d1);   // The diverter
        readIn (s2);   // The "straight" segment
        readIn (s3);   // The "divert" segment
        Assert (s1 >= 0 && s1 < NSSegments && s2 >= 0 && s2 < NSSegments &&
                s3 >= 0 && s3 < NSSegments,"initBoard: illegal segment number");
        Assert (d1 >= 0 && d1 < NSDiverters,
                                          "initBoard: illegal diverter number");
        Assert (SSegments [s1] -> NU == NULL,
                                        "initBoard: segment already connected");
        SSegments [s1] -> NU = SDiverters [d1];
        Assert (SDiverters [d1] -> NU == NULL && SDiverters [d1] -> NDU == NULL,
                                       "initBoard: diverter already connected");
        SDiverters [d1] -> NU = SSegments [s2];
        SDiverters [d1] -> NDU = SSegments [s3];
        // Now take care of the box types to be diverted
        readIn (s1);   // Number of types
        Assert (s1 > 0, "initBoard: negative or zero box types");
        SDiverters [d1] -> DTypes = new int [s1+1];
        for (i = 0; i < s1; i++) readIn (SDiverters [d1] -> DTypes [i]);
        SDiverters [d1] -> DTypes [s1] = NONE;
        break;
      case CONN_SSMS:
        // Segment, segment, merger, segment
        readIn (s1);
        readIn (s2);
        readIn (d1);
        readIn (s3);
        Assert (s1 >= 0 && s1 < NSSegments && s2 >= 0 && s2 < NSSegments &&
                s3 >= 0 && s3 < NSSegments,"initBoard: illegal segment number");
        Assert (d1 >= 0 && d1 < NSDiverters,
                                            "initBoard: illegal merger number");
        Assert (SSegments [s1] -> NU == NULL && SSegments [2] -> NU == NULL,
                                        "initBoard: segment already connected");
        SSegments [s1] -> NU = SMergers [d1];
        SSegments [s2] -> NU = SMergers [d1];
        // Tell the two sources apart at the merger
        SMergers [d1] -> Senders [0] = SSegments [s1] -> getId ();
        SMergers [d1] -> Senders [1] = SSegments [s2] -> getId ();
        SMergers [d1] -> NU = SSegments [s3];
        break;
    }
  }
};
