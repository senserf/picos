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

#ifndef __operator_h__

#define	__operator_h__

#include "server.h"

#include "io.h"

// Operator command format
//
//     -- command byte
//     -- length 0
//     -- length 1
//     -- length bytes command specific

#define  CMDHDRL    3          // Header length
#define  COMMAND(Cmd)    (((int)((unsigned char)(Cmd [0]))) & 0xff)
#define  CMDLEN0(Cmd)    (((int)(Cmd [1])) & 0xff)
#define  CMDLEN1(Cmd)    (((int)(Cmd [2])) & 0xff)
#define  CMDLENGTH(Cmd)  ((CMDLEN0(Cmd) << 8) + CMDLEN1(Cmd))

// ------------- //
// Command codes //
// ------------- //
#define  CMD_UPDATE      0
#define  CMD_ALERT       1
#define  CMD_GETMAP      2
#define  CMD_GETGRAPH    3
#define  CMD_GETSTAT     4
#define  CMD_PERIODIC    5
#define  CMD_UNPERIODIC  6
#define  CMD_OVERRIDE    7
#define  CMD_DISCONNECT  8
#define  CMD_START       254
#define  CMD_STOP        255

#define  upper(a)   ((unsigned char)((a) >> 8))
#define  lower(a)   ((unsigned char)((a) & 0xff))

#define  byte0(a)      ((unsigned char)(((a) >> 24) & 0xff))
#define  byte1(a)      ((unsigned char)(((a) >> 16) & 0xff))
#define  byte2(a)      ((unsigned char)(((a) >>  8) & 0xff))
#define  byte3(a)      ((unsigned char)( (a)        & 0xff))

#define  extshort(ix) \
           (((((int)(Cmd[(ix)]))&0xff)<<8)+(((int)(Cmd[(ix)+1]))&0xff))

#define  NAMAPINITSIZE         256  // Initial size of alert map
#define  OPBUFSIZE            8192  // Socket buffer size
#define  CMDBUFSIZE           4096  // Command buffer size
#define  DEFAULT_UPDATE_DELAY  0.2  // Seconds
#define  SEGENTRYSIZE           10  // Size of a segment update entry
#define  SRENTRYSIZE            16  // Size of a source update entry
#define  SNENTRYSIZE             8  // Size of a sink update entry
#define  MAXALERTSIZE          256  // Maximum total size of an alert message
#define  MAXUPDATESIZE        4096  // Size of the update buffer
#define  DISCONNDELAY          1.0  // Disconnect retry delay in seconds

// Alert mailbox types

#define  ALERT     0
#define  OVERRIDE  1

class AMapEntry {
  public:
    char *Id;
    int AType;
    Mailbox *Al;
    int UId, UType;
};

// PUEntry types = SEGMENTS, DIVERTER, etc +
// -----------------------------------------
#define DELETED   -1

process OPDriver;

class PUEntry {
  public:
    SUnit *UN;
    int UType, UId;
    PUEntry *Next;
    PUEntry (SUnit*, int, int, OPDriver*);
};

class ASock {
  public:
    Socket *Sok;
    ASock *Next;
    ASock (Socket *s) { Sok = s; };
};

process ALSender {
  char AlertBuffer [MAXALERTSIZE];
  int AlertMessageLength, Index;
  ASock *SokList, *CurSok;
  void makeAlertMessage (int, int, const char*);
  void report (Socket*);
  void remove (Socket*);
  void setup ();
  states {WaitAlerts, NextAlert, SendAlert, NextSocket, SendCopy};
  perform;
};

process Updater {
  int UpdateLength;
  PUEntry *PUList;     // Periodic update list
  PUEntry *Current;    // Currently processed entry
  char UpdateBuffer [MAXUPDATESIZE];
  OPDriver *OPD;
  Socket *OP;
  void setup (OPDriver*);
  states {Update, Terminate, NextEntry, SendUpdate};
  perform;
};

#define readRequest(where,length,notready) \
         do { \
                if (OP->read (where, length)) { \
                  if (!(OP->isActive ())) proceed (Disconnect, URGENT); \
                  OP->wait (length, notready); \
                  sleep; \
                } \
         } while (0)

#define sendData(where,length,notready) \
         do { \
                if (OP->write (where, length)) { \
                  if (!(OP->isActive ())) proceed (Disconnect, URGENT); \
                  OP->wait (OUTPUT, notready); \
                  sleep; \
                } \
         } while (0)
                    
process OPDriver {
  char Cmd [CMDBUFSIZE];
  int CommandLength, OutPtr, Index;
  Boolean Terminating;
  Updater *MyUpdater;
  Socket *OP;
  void makeErrorMessage (const char*);
  void setup (Socket*);
  states {WaitCommand, WaitTail, DoGetMap, ContinueGetMap, SendGetMap,
          DoGetStatus, SendStatus, DoPeriodic, DoUnPeriodic, DoOverride,
          DoGetGraph, ContinueGetGraph, SendGetGraph, DoStart, DoStop,
          DoDisconnect, Disconnect, CloseConnection, DoDefault};
  perform;
};

extern Process *TheRoot;

void initOperator ();

#include  <stdarg.h>

/*
#ifdef __GNUC_VA_LIST
#define VA_TYPE __gnuc_va_list
#else
#if     ZZ_CXX
#define VA_TYPE va_list
#else
#define VA_TYPE char*
#endif
#endif
*/


#endif
