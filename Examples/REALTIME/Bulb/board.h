#ifndef __board_h__
#define __board_h__

#include "port.h"
#include "sdsclient.h"
#include "sensor.h"

#define SBUFSIZE                20  // SDS mailbox buffer size

void initBoard ();

#define requestChangeReports(m,s)\
          do { (m)->put (CRT_AIS);\
               (m)->put (0);\
               (m)->put (2);\
               (m)->put ((s)->Reference.Net);\
               (m)->put ((s)->Reference.Address);       } while (0)

#define makeActuatorMessage(m,s)\
          do { (m)[0] = CRT_WOU;\
               (m)[1] = 0;\
               (m)[2] = 4;\
               (m)[3] = (s)->Reference.Net;\
               (m)[4] = (s)->Reference.Address;\
               (m)[5] = 0;\
               (m)[6] = (s)->getValue ();   } while (0)

#define CONFMESSIZE  4              // Size of the initial ACK message
#define STATMESSIZE  5              // Size of status update message
#define STATMESSTAT  2              // Location of the status byte
#define ACTSMESSIZE  7              // Actuator message size
#define ACTSMESSTAT  6              // Location of the status byte

mailbox SDSMailbox (int);

process SDSToVirtual {
  Sensor *Sn;
  SDSMailbox *Sm;
  void setup (Sensor *s, SDSMailbox *m) {
    Sn = s;
    Sm = m;
    Sm->connect (INTERNET+CLIENT, SERVNAME, PORT, SBUFSIZE);
    requestChangeReports (Sm, Sn);
  };
  states {WaitSDSInit, WaitSDSStatus};
  perform {
    char msg [STATMESSIZE];
    state WaitSDSInit:
      if (Sm->read (msg, CONFMESSIZE) != OK) {
        Sm->wait (CONFMESSIZE, WaitSDSInit);
        sleep;
      }
    transient WaitSDSStatus:
      if (Sm->read (msg, STATMESSIZE) == OK) {
        // A status update message has arrived
        Sn->setValue ((int)(msg [STATMESSTAT]));
        proceed WaitSDSStatus;
      } else
        Sm->wait (STATMESSIZE, WaitSDSStatus);
  };
};
      
process VirtualToSDS {
  Actuator *An;
  SDSMailbox *Sm;
  char Msg [ACTSMESSIZE];
  void setup (Actuator *s, SDSMailbox *m) {
    An = s;
    Sm = m;
    Sm->connect (INTERNET+CLIENT, SERVNAME, PORT, SBUFSIZE);
    makeActuatorMessage (Msg, s);
    Sm->write (Msg, ACTSMESSIZE);
  };
  states {WaitVirtualStatus, NewVirtualState};
  perform {
    int Val;
    state WaitVirtualStatus:
      if (An->pending ()) {
        Msg [ACTSMESSTAT] = (char) (An->getValue ());
        proceed NewVirtualState;
      }
      An->wait (UPDATE, WaitVirtualStatus);
    state NewVirtualState:
      Sm->erase (); // Clear the input buffer (we don't care about ACKs)
      if (Sm->write (Msg, ACTSMESSIZE) == OK)
        proceed WaitVirtualStatus;
      else
        proceed NewVirtualState;
  };
};

#endif
