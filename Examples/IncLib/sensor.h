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

#ifndef	__sensor_h__
#define	__sensor_h__

class NetAddress {
  public:
    int Domain, Net, Address;
  NetAddress (int d, int n, int a) {
    Domain = d;
    Net = n;
    Address = a;
  };
  NetAddress (int n, int a) { NetAddress (0, n, a); };
  NetAddress (int a) { NetAddress (0, a); };
  NetAddress () { NetAddress (0); };
};

inline  int  operator== (NetAddress &a, NetAddress &b) {
  return (a.Domain == b.Domain && a.Net == b.Net && a.Address == b.Address);
};


// The length of a buffer kept in an alert mailbox to accomodate a
// maximum-length  alert string.
// -------------------------------------
#define  ALERT_STRING_LENGTH_LIMIT   256

mailbox Sensor {
  private:
    int Value;
    void mapNet ();
  public:
    NetAddress Reference;
    void setValue (int);
    int getValue ();
    void setup (NetAddress&);
};

// This is a generic type describing an actuator

mailbox Actuator {
  private:
    int Value;
    void mapNet ();
  public:
    NetAddress Reference;
    void setValue (int);
    int getValue ();
    void setup (NetAddress&, int iv = 0);
};

// This mailbox type is used for sending alerts to the operator

mailbox Alert {
  private:
    char txtbuf [ALERT_STRING_LENGTH_LIMIT];
    void mapNet (const char*);
    int Value;
  public:
    int getValue ();
    char *getMessage ();
    void clearValue ();
    void notify (int, const char *fmt, ...);
    void setup (const char*);
};

process Overrideable;

// This mailbox type is used for receiving override signals
mailbox Override {
  friend class Overrideable;
  private:
    void mapNet (const char*);
    int Value, Action, SerialNumber;
  public:
    void force (int, int);
    void setup (const char*);
};

process Overrideable {
  int OSN;
  Override *Reset;
  void setup (const char*);
  void overrideAcknowledge () { OSN = Reset->SerialNumber; };
  int overridePending ();
  int overrideAction () { return Reset->Action; };
  int overrideValue () { return Reset->Value; };
};

// Some status values
// ------------------------
#define SENSOR_ON         0   // This is how it works for the bulb sensor
#define SENSOR_OFF        1   // in SDS
#define ACTUATOR_ON       1
#define ACTUATOR_OFF      0
#define MOTOR_ON          ACTUATOR_ON
#define MOTOR_OFF         ACTUATOR_OFF

// Alerts
// ------------------------
#define X_ERROR         001
#define X_WARNING       002
#define X_BADCMD        003

// Event priorities
// -----------------------------
#define URGENT          (-1024)
#define HIGH            (-256)
#define LOW             1024
#define STANDARD        0

// Some override actions
// ------------------------
#define	OVR_MOTOR_CNTRL 001
#define OVR_SET_COUNT   002
#define OVR_CLEAR       003
// ... this room for rent ...
#define OVR_HOLD        254
#define OVR_RESUME      255

void readInNetAddress (NetAddress&);

// This macro is a standard sequence for detecting override signals

#define onOverride(state) \
           do { \
                if (overridePending ()) proceed (state, URGENT); \
                Reset->wait (NEWITEM, state, URGENT); \
              } while (0)

#endif
