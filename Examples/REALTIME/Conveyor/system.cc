#include "types.h"

int NSegments = 0, NDiverters = 0, NMergers = 0;

Segment  **Segments;
Diverter **Diverters;
Merger   **Mergers;

void Unit::setup (NetAddress &motor, double MotorInertia) {
  Exception = create Alert (this->getOName ());
  Motor     = create Actuator (motor, MOTOR_OFF);
  MD        = create MotorDriver (Motor, MotorInertia);
};

void Segment::setup (NetAddress &motor,   // Motor actuator
                     double MotorInertia, // So that we are not too jumpy
                     NetAddress &entry,   // Entrance sensor
                     double ESensorInertia,
                     NetAddress &exit,    // Exit sensor
                     double XSensorInertia,
                     double TransitTimeBound) {

  Segments [NSegments++] = this;
  Unit::setup (motor, MotorInertia);
  BoxesInTransit = 0;
  In    = create Sensor   (entry);
  Out   = create Sensor   (exit);
  SDIn  = create SensorDriver (In, ESensorInertia);
  SDOut = create SensorDriver (Out, XSensorInertia);
  create SegmentDriver (TransitTimeBound);
};

void Diverter::setup (NetAddress &motor,   // Motor actuator and inertia
                      double MotorInertia, // So that we are not too jumpy
                      NetAddress &full,    // Full sensor
                      double SensorInertia,
                      NetAddress &divert,  // Divert sensor
                      double DivertLatency) {

  Diverters [NDiverters++] = this;
  Unit::setup (motor, MotorInertia);
  Full   = create Sensor   (full);
  Divert = create Sensor   (divert);
  SDFull = create SensorDriver (Full, SensorInertia);
  create DiverterDriver (DivertLatency);
};

void Merger::setup (NetAddress &motor,     // Motor actuator and inertia
                      double MotorInertia, // So that we are not too jumpy
                      NetAddress &entry1,  // Entrance sensor 1
                      double ESensorInertia1,
                      NetAddress &entry2,  // Entrance sensor 2
                      double ESensorInertia2,
                      NetAddress &exit,    // Exit sensor
                      double XSensorInertia,
                      double TransitTimeBound) {
  Mergers [NMergers++] = this;
  Unit::setup (motor, MotorInertia);
  BoxesInTransit = 0;
  In1   = create Sensor   (entry1);
  In2   = create Sensor   (entry2);
  Out   = create Sensor   (exit);
  SDIn1  = create SensorDriver (In1, ESensorInertia1);
  SDIn2  = create SensorDriver (In2, ESensorInertia2);
  SDOut = create SensorDriver (Out, XSensorInertia);
  create MergerDriver (TransitTimeBound);
};

void MotorDriver::setup (Actuator *m, double inertia) {
  Motor = m;
  Inertia = (TIME) (Second * inertia);
};

void SensorDriver::setup (Sensor *s, double inertia) {
  Sense = s;
  Inertia = (TIME) (Second * inertia);
  LastValue = s->getValue ();
};

void SegmentDriver::setup (double TransitTimeBound) {
  Overrideable::setup (form ("SgmDrv@%s", S->getOName ()));
  MD     = S->MD;  // Make local copies for easier access
  SDIn   = S->SDIn;
  SDOut  = S->SDOut;
  Exception = S->Exception;
  EndToEndTime = (TIME) (Second * TransitTimeBound);
};

void MergerDriver::setup (double TransitTimeBound) {
  Overrideable::setup (form ("MrgDrv@%s", S->getOName ()));
  MD     = S->MD;  // Make local copies for easier access
  SDIn1  = S->SDIn1;
  SDIn2  = S->SDIn2;
  SDOut  = S->SDOut;
  Exception = S->Exception;
  EndToEndTime = (TIME) (Second * TransitTimeBound);
};

void DiverterDriver::setup (double dt) {
  Overrideable::setup (form ("DvtDrv@%s", S->getOName ()));
  DivertTime = (TIME) (Second * dt);
  MD     = S->MD;  // Make local copies for easier access
  SDFull = S->SDFull;
  Divert = S->Divert;
  Exception = S->Exception;
};

MotorDriver::perform {
  int NewStatus;
  state StatusWait:
    TheProcess->wait (SIGNAL, StatusChange, HIGH);
  state StatusChange:
    if ((int)TheSignal != Motor->getValue ()) {
      // A new status
      Motor->setValue ((int)TheSignal);
      Timer->wait (Inertia, StatusWait);
    } else
      TheProcess->wait (SIGNAL, StatusChange, HIGH);
};

SensorDriver::perform {
  int NewValue;
  state StatusChange:
    if ((NewValue = Sense->getValue ()) == LastValue) {
      // Not a change really -- ignore it
      Sense->wait (NEWITEM, StatusChange, HIGH);
      sleep;
    }
    signal ((void*)(LastValue = NewValue));  // Pass the change
  transient WaitResume:
    Timer->wait (Inertia, StatusChange);
};
    
SegmentDriver::perform {

  TIME EndToEndDelay;

  state WaitSensor:

    onOverride (ProcessOverride);

    SDIn->wait (SIGNAL, Input);
    SDOut->wait (SIGNAL, Output);
    if (S->Motor->getValue () == MOTOR_ON) {
      EndToEndDelay = Time - LastOutTime;
      if (EndToEndDelay < EndToEndTime) {
        Timer->wait (EndToEndTime-EndToEndDelay, WaitSensor);
      } else {
        LastOutTime = Time;      // Avoid looping on the same jam
        if (S->BoxesInTransit) {
          // Jam report
          Exception->notify (X_ERROR,
                       "Suspected jam involving %1d boxes", S->BoxesInTransit);
          // Keep the motor running just in case
        } else 
          MD->signal (MOTOR_OFF);
      }
    }

  state Input:

    if (TheSignal == SENSOR_ON) {
      if (S->Motor->getValue () == MOTOR_OFF)
        MD->signal ((void*)MOTOR_ON);    // Start up the motor
      LastOutTime = Time;                // For detecting jams
      S->BoxesInTransit ++;
    }
    proceed WaitSensor;

  state Output:

    if ((int)TheSignal == SENSOR_OFF) {
      if (S->BoxesInTransit)
        S->BoxesInTransit--;
      else
        Exception->notify (X_WARNING, "Unexpected box");
      LastOutTime = Time;
    }
    proceed WaitSensor;

  state ProcessOverride:

    overrideAcknowledge ();
    switch (overrideAction ()) {
      case OVR_MOTOR_CNTRL:
        S->Motor->setValue (overrideValue ());
        onOverride (ProcessOverride);  // Ignore everything else
        sleep;
      case OVR_SET_COUNT:
        S->BoxesInTransit = overrideValue ();
        onOverride (ProcessOverride);  // Ignore everything else
        sleep;
      case OVR_RESUME:
      default:
        S->In->setValue (S->In->getValue ());   // Force sensor events
        S->Out->setValue (S->Out->getValue ());   // Force sensor events
        proceed WaitSensor;
    }
};

MergerDriver::perform {

  TIME EndToEndDelay;

  state WaitSensor:

    onOverride (ProcessOverride);

    SDIn1->wait (SIGNAL, Input);
    SDIn2->wait (SIGNAL, Input);
    SDOut->wait (SIGNAL, Output);
    if (S->Motor->getValue () == MOTOR_ON) {
      EndToEndDelay = Time - LastOutTime;
      if (EndToEndDelay < EndToEndTime) {
        Timer->wait (EndToEndTime-EndToEndDelay, WaitSensor);
      } else {
        LastOutTime = Time;      // Avoid looping on the same jam
        if (S->BoxesInTransit) {
          // Jam report
          Exception->notify (X_ERROR,
                       "Suspected jam involving %1d boxes", S->BoxesInTransit);
          // Keep the motor running just in case
        } else 
          MD->signal (MOTOR_OFF);
      }
    }

  state Input:

    if (TheSignal == SENSOR_ON) {
      if (S->Motor->getValue () == MOTOR_OFF)
        MD->signal ((void*)MOTOR_ON);    // Start up the motor
      LastOutTime = Time;                // For detecting jams
      S->BoxesInTransit ++;
    }
    proceed WaitSensor;

  state Output:

    if ((int)TheSignal == SENSOR_OFF) {
      if (S->BoxesInTransit)
        S->BoxesInTransit--;
      else
        Exception->notify (X_WARNING, "Unexpected box");
      LastOutTime = Time;
    }
    proceed WaitSensor;

  state ProcessOverride:

    overrideAcknowledge ();
    switch (overrideAction ()) {
      case OVR_MOTOR_CNTRL:
        S->Motor->setValue (overrideValue ());
        onOverride (ProcessOverride);  // Ignore everything else
        sleep;
      case OVR_SET_COUNT:
        S->BoxesInTransit = overrideValue ();
        onOverride (ProcessOverride);  // Ignore everything else
        sleep;
      case OVR_RESUME:
      default:
        S->In1->setValue (S->In1->getValue ());   // Force sensor events
        S->In2->setValue (S->In1->getValue ());   // Force sensor events
        S->Out->setValue (S->Out->getValue ());   // Force sensor events
        proceed WaitSensor;
    }
};

DiverterDriver::perform {

  int OValue;

  state WaitSensor:

    onOverride (ProcessOverride);
    Divert->setValue (NONE);
    SDFull->wait (SIGNAL, Input);

  state Input:

    if (TheSignal != SENSOR_ON) proceed WaitSensor;
    Timer->wait (DivertTime, DivertDecision);

  state DivertDecision:

    if (S->Full->getValue () != SENSOR_ON) {
      Exception->notify (X_WARNING,
        "Object disappeared before divert decision was made");
      proceed WaitSensor;
    }
    if ((OValue = Divert->getValue ()) == NONE) {
      Exception->notify (X_WARNING, "Divert decision timeout");
      proceed WaitSensor;
    }
    if (OValue != SENSOR_ON) proceed WaitSensor;
    // Divert the box
    MD->signal ((void*)MOTOR_ON);
    skipto EndDivert;

  state EndDivert:

    // Turn the motor off immediately -- it will be postponed by the
    // inertia time -- enough to divert the object
    MD->signal ((void*)MOTOR_OFF);
    proceed WaitSensor;

  state ProcessOverride:

    overrideAcknowledge ();
    switch (overrideAction ()) {
      case OVR_MOTOR_CNTRL:
        S->Motor->setValue (overrideValue ());
        onOverride (ProcessOverride);  // Ignore everything else
        sleep;
      case OVR_RESUME:
      default:
        S->Full->setValue (S->Full->getValue ());  // Force sensor event
        proceed WaitSensor;
    }
};

// ----------------------------------------------------------------------------


void initSystem () {
  int i, nsegments, ndiverters, nmergers;
  NetAddress a1, a2, a3, a4;
  double d1, d2, d3, d4, d5;
  readIn (nsegments);
  readIn (ndiverters);
  readIn (nmergers);

  // NSegments and NDiverters will be updated by the setup methods of
  // Segment and Diverter

  if (nsegments) Segments = new Segment* [nsegments];
  if (ndiverters) Diverters = new Diverter* [ndiverters];
  if (nmergers) Mergers = new Merger* [nmergers];

  for (i = 0; i < nsegments; i++) {
    readInNetAddress (a1);   // Motor
    readIn (d1);             // Motor inertia
    readInNetAddress (a2);   // Entry sensor
    readIn (d2);             // Sensor inertia
    readInNetAddress (a3);   // Exit sensor
    readIn (d3);             // Sensor inertia
    readIn (d4);             // End-to-end trip time
    Segments [i] = create Segment, form ("Sgm#%1d", i),
                                                   (a1, d1, a2, d2, a3, d3, d4);
  }

  for (i = 0; i < ndiverters; i++) {
    readInNetAddress (a1);   // Motor
    readIn (d1);             // Motor inertia
    readInNetAddress (a2);   // Full sensor
    readIn (d2);             // Sensor inertia
    readInNetAddress (a3);   // Divert sensor
    readIn (d3);             // Reader latency (divert sensor inertia)
    Diverters [i] = create Diverter, form ("Dvt#%1d", i),
                                                       (a1, d1, a2, d2, a3, d3);
  }

  for (i = 0; i < nmergers; i++) {
    readInNetAddress (a1);   // Motor 
    readIn (d1);             // Motor inertia
    readInNetAddress (a2);   // Entry sensor1
    readIn (d2);             // Sensor inertia
    readInNetAddress (a3);   // Entry sensor2
    readIn (d3);             // Sensor inertia
    readInNetAddress (a4);   // Exit sensor
    readIn (d4);             // Sensor inertia
    readIn (d5);             // Latency time bound
    Mergers [i] = create Merger, form ("Mrg#%1d", i), 
                                           (a1, d1, a2, d2, a3, d3, a4, d4, d5);
  }

};
