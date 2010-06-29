#include "operator.h"

#define	ACCEPTOR OPDriver

#include "server.cc"

static AMapEntry  *AMap;

static int NAMapEntries = 0,
           NAMapMaxSize = 0;

static ALSender *ALS = NULL;       // Alert sender process

static TIME UpdateDelay,
            IdleTime;

static int NConnections = 0;       // The number of active connections

#ifdef	DEBUG
static void dumpBuf (const char *hdr, const char *buf, int len) {
  int i;
  Ouf << hdr << '\n';
  for (i = 0; i < len; i++) {
    Ouf << form ("%2x (%c) ", ((int) buf [i]) & 0xff,
        buf [i] >= 0x20 ? buf [i] : ' ');
    if (i > 0 && (i % 10 == 0) && i < len-1) Ouf << '\n';
  }
  Ouf << '\n' << '\n';
  Ouf.flush ();
}
#else
#define dumpBuf(a,b,c)
#endif

static void addMapEntry (int sid, Mailbox *sn, const char *id, int tp) {
  AMapEntry *nm;
  int i, ns;
  if (NAMapMaxSize <= NAMapEntries) {
    // Grow the map
    if (NAMapMaxSize == 0)
      ns = NAMAPINITSIZE;
    else
      ns = NAMapMaxSize + NAMapMaxSize;
    nm = AMap;     // Save the old map for a while
    AMap = new AMapEntry [ns];
    for (i = 0; i < NAMapMaxSize; i++) AMap [i] = nm [i];
    if (NAMapMaxSize) delete (nm);
    NAMapMaxSize = ns;
  }
  AMap [NAMapEntries] . AType = tp;
  AMap [NAMapEntries] . Al = sn;
  AMap [NAMapEntries] . Id = new char [strlen (id) + 1];
  strcpy (AMap [NAMapEntries] . Id, id);

  i = idToUId (sid, ns);
  if (i == NONE) excptn ("addMapEntry: Unit Id mismatch");
  AMap [NAMapEntries] . UId = i;
  AMap [NAMapEntries] . UType = ns;

  NAMapEntries++;
};

void Alert::mapNet (const char *aid) {
  addMapEntry (TheStation->getId (), this, aid, ALERT);
};

void Alert::notify (int level, const char *fmt, ...) {
  VA_TYPE ap;
  va_start (ap, fmt);
  vsprintf (txtbuf, fmt, ap);
  Value = level;
  ALS->signal (NULL);
};

void Override::mapNet (const char *oid) {
  addMapEntry (TheStation->getId (), this, oid, OVERRIDE);
};

 
static int generateSegmentUpdate (SSegment *S, int id, char *UpdateBuffer,
                                                                     int UBS) {
  Long iptr, dist;
  Belt *CB;
  Box *b, *h;
  TIME tt;
  int MotorStatus;

  UpdateBuffer [0] = CMD_UPDATE;
  iptr = 3;
  // Unit Id (2 bytes)
  UpdateBuffer [iptr++] = upper (id);
  UpdateBuffer [iptr++] = lower (id);
  // Unit type
  UpdateBuffer [iptr++] = (char) SSEGMENT;
  // Motor and sensor status
  UpdateBuffer [iptr++] = (char) (MotorStatus = S->Motor->getValue ());
  UpdateBuffer [iptr++] = (char) (S->In->getValue ());
  UpdateBuffer [iptr++] = (char) (S->Out->getValue ());
  // The total length of the belt in micrometres
  dist = (Long) (((double)(S->CB->EndToEndTime))/S->Speed);
  UpdateBuffer [iptr++] = byte0 (dist);
  UpdateBuffer [iptr++] = byte1 (dist);
  UpdateBuffer [iptr++] = byte2 (dist);
  UpdateBuffer [iptr++] = byte3 (dist);
  CB = S->CB;
  CB->Disabled = YES;  // Switch off inItem processing for the belt
  h = CB->get ();
  if (h != NULL) {
    for (b = h; ;) {
      // Check if have room for anything more
      if (iptr < UBS - SEGENTRYSIZE) {
        // Calculate the distance from the end of the belt
        tt = b->TTime + CB->MotorStoppedTime;
        if (MotorStatus == MOTOR_OFF) tt += (Time - CB->WhenStopped);
        if (tt < Time) tt = 0; else tt -= Time;
        // Convert time to metres, well micrometres anyway
        dist = (Long)(((double)(tt))/S->Speed);
        UpdateBuffer [iptr++] = byte0 (dist);
        UpdateBuffer [iptr++] = byte1 (dist);
        UpdateBuffer [iptr++] = byte2 (dist);
        UpdateBuffer [iptr++] = byte3 (dist);
        // Now take care of length
        dist = b->TLength;
        UpdateBuffer [iptr++] = byte0 (dist);
        UpdateBuffer [iptr++] = byte1 (dist);
        UpdateBuffer [iptr++] = byte2 (dist);
        UpdateBuffer [iptr++] = byte3 (dist);
        // Now the type
        dist = b->TP;
        UpdateBuffer [iptr++] = upper (dist);
        UpdateBuffer [iptr++] = lower (dist);
        // This is 10 bytes altogether (SEGENTRYSIZE)
      }
      CB->put (b);
      b = CB->first ();
      if (b == h) break;
      b = CB->get ();
    }
  }
  CB->Disabled = NO;
  // Stick the length into the string
  dist = iptr;
  iptr -= 3;  // Exclude the command byte and the length field
  UpdateBuffer [1] = upper (iptr);
  UpdateBuffer [2] = lower (iptr);
  return dist;
};

static int generateMergerUpdate (SMerger *S, int id, char *UpdateBuffer,
                                                                     int UBS) {
  Long iptr, dist, len, LastPos;
  Belt *CB;
  Box *b, *h;
  TIME tt;
  int MotorStatus;

  UpdateBuffer [0] = CMD_UPDATE;
  iptr = 3;
  // Unit Id (2 bytes)
  UpdateBuffer [iptr++] = upper (id);
  UpdateBuffer [iptr++] = lower (id);
  // Unit type
  UpdateBuffer [iptr++] = (char) SMERGER;
  // Motor and sensor status
  UpdateBuffer [iptr++] = (char) (MotorStatus = S->Motor->getValue ());
  UpdateBuffer [iptr++] = (char) (S->In[0]->getValue ());
  UpdateBuffer [iptr++] = (char) (S->In[1]->getValue ());
  UpdateBuffer [iptr++] = (char) (S->Out->getValue ());
  // The total length of the belt in micrometres
  dist = (Long) (((double)(S->Buffer->EndToEndTime))/S->ISpeed);
  UpdateBuffer [iptr++] = byte0 (dist);
  UpdateBuffer [iptr++] = byte1 (dist);
  UpdateBuffer [iptr++] = byte2 (dist);
  UpdateBuffer [iptr++] = byte3 (dist);
  CB = S->Buffer;
  CB->Disabled = YES;  // Switch off inItem processing for the belt
  h = CB->get ();
  if (h != NULL) {
    LastPos = TIME_0;
    for (b = h; ;) {
      // Check if have room for anything more
      if (iptr < UBS - SEGENTRYSIZE) {
        // Calculate the distance from the end of the belt
        tt = b->TTime + CB->MotorStoppedTime;
        if (MotorStatus == MOTOR_OFF) tt += (Time - CB->WhenStopped);
        if (tt < Time) tt = 0; else tt -= Time;
        // Convert time to metres, well micrometres anyway
        dist = (Long)(((double)(tt))/S->Speed);
        if (dist < LastPos) dist = LastPos;
        UpdateBuffer [iptr++] = byte0 (dist);
        UpdateBuffer [iptr++] = byte1 (dist);
        UpdateBuffer [iptr++] = byte2 (dist);
        UpdateBuffer [iptr++] = byte3 (dist);
        // Now take care of length
        len = b->TLength;
        UpdateBuffer [iptr++] = byte0 (len);
        UpdateBuffer [iptr++] = byte1 (len);
        UpdateBuffer [iptr++] = byte2 (len);
        UpdateBuffer [iptr++] = byte3 (len);
        LastPos = dist + len;
        // Now the type
        dist = b->TP;
        UpdateBuffer [iptr++] = upper (dist);
        UpdateBuffer [iptr++] = lower (dist);
        // This is 10 bytes altogether (SEGENTRYSIZE)
      }
      CB->put (b);
      b = CB->first ();
      if (b == h) break;
      b = CB->get ();
    }
  }
  CB->Disabled = NO;
  // Stick the length into the string
  dist = iptr;
  iptr -= 3;  // Exclude the command byte and the length field
  UpdateBuffer [1] = upper (iptr);
  UpdateBuffer [2] = lower (iptr);
  return dist;
};

static int generateDiverterUpdate (SDiverter *S, int id, char *UpdateBuffer,
                                                                     int UBS) {
  int iptr;

  // Buffer size (UBS) is ignored -- this update is very short

  UpdateBuffer [0] = CMD_UPDATE;
  iptr = 3;
  // Unit Id (2 bytes)
  UpdateBuffer [iptr++] = upper (id);
  UpdateBuffer [iptr++] = lower (id);
  // Unit type
  UpdateBuffer [iptr++] = (char) SDIVERTER;
  // Motor and sensor status
  UpdateBuffer [iptr++] = (char) (S->Motor->getValue ());
  UpdateBuffer [iptr++] = (char) (S->Full->getValue ());
  UpdateBuffer [iptr  ] = (char) (S->Divert->getValue ());
  iptr -= 2;
  UpdateBuffer [1] = upper (iptr);
  UpdateBuffer [2] = lower (iptr);
  return iptr+3;
};

static int generateSourceUpdate (Source *S, int id, char *UpdateBuffer,
                                                                     int UBS) {
  int iptr, i, bc, mc, bx, mx;
  Message *m;

  UpdateBuffer [0] = CMD_UPDATE;
  // Unit Id (2 bytes)
  iptr = 3;
  UpdateBuffer [iptr++] = upper (id);
  UpdateBuffer [iptr++] = lower (id);
  UpdateBuffer [iptr++] = (char) SOURCE;

  // Queue sizes: counts + total length
  for (i = 0; i < NTraffics; i++) {
    mc = bc = 0;
    if (S->MQHead != NULL) {
      for (m = S->MQHead [i]; m != NULL; m = m->next) {
        bc++;  // Increment box count
        mc += (m->Length / 10000);  // Total length in centimetres
      }
    }
    if (S->NXmBoxes != NULL) {
      bx = S->NXmBoxes [i];
      mx = S->NXmCmtrs [i];
    } else
      bx = mx = 0;
    if (iptr < UBS - SRENTRYSIZE) {
      UpdateBuffer [iptr++] = byte0 (bx);
      UpdateBuffer [iptr++] = byte1 (bx);
      UpdateBuffer [iptr++] = byte2 (bx);
      UpdateBuffer [iptr++] = byte3 (bx);

      UpdateBuffer [iptr++] = byte0 (mx);
      UpdateBuffer [iptr++] = byte1 (mx);
      UpdateBuffer [iptr++] = byte2 (mx);
      UpdateBuffer [iptr++] = byte3 (mx);

      UpdateBuffer [iptr++] = byte0 (bc);
      UpdateBuffer [iptr++] = byte1 (bc);
      UpdateBuffer [iptr++] = byte2 (bc);
      UpdateBuffer [iptr++] = byte3 (bc);

      UpdateBuffer [iptr++] = byte0 (mc);
      UpdateBuffer [iptr++] = byte1 (mc);
      UpdateBuffer [iptr++] = byte2 (mc);
      UpdateBuffer [iptr++] = byte3 (mc);
    }
  }
  iptr -= 3;
  UpdateBuffer [1] = upper (iptr);
  UpdateBuffer [2] = lower (iptr);
  return iptr+3;
};

static int generateSinkUpdate (Sink *S, int id, char *UpdateBuffer, int UBS) {
  int iptr, i, bc, mc;

  UpdateBuffer [0] = CMD_UPDATE;
  // Unit Id (2 bytes)
  iptr = 3;
  UpdateBuffer [iptr++] = upper (id);
  UpdateBuffer [iptr++] = lower (id);
  UpdateBuffer [iptr++] = (char) SINK;
  for (i = 0; i < NTraffics; i++) {
    if (S->NRcvBoxes != NULL) {
      bc = S->NRcvBoxes [i];
      mc = S->NRcvCmtrs [i];
    } else
      bc = mc = 0;
    if (iptr < UBS - SNENTRYSIZE) {  // This is the same as above
      UpdateBuffer [iptr++] = byte0 (bc);
      UpdateBuffer [iptr++] = byte1 (bc);
      UpdateBuffer [iptr++] = byte2 (bc);
      UpdateBuffer [iptr++] = byte3 (bc);
      UpdateBuffer [iptr++] = byte0 (mc);
      UpdateBuffer [iptr++] = byte1 (mc);
      UpdateBuffer [iptr++] = byte2 (mc);
      UpdateBuffer [iptr++] = byte3 (mc);
    }
  }
  iptr -= 3;
  UpdateBuffer [1] = upper (iptr);
  UpdateBuffer [2] = lower (iptr);
  return iptr+3;
};

void ALSender::setup () {
  SokList = NULL;
};

void ALSender::report (Socket *s) {
  ASock *as;
  for (as = SokList; as != NULL; as = as->Next)
    if (as->Sok == s) return;  // Already reported
  as = new ASock (s);
  as -> Next = SokList;
  SokList = as;
};

void ALSender::remove (Socket *s) {
  ASock *as;
  for (as = SokList; as != NULL; as = as->Next) {
    if (as->Sok == s) {
      // The actual deletion will be done by ALSender -- to avoid races
      as->Sok = NULL;
      return;
    }
  }
};
  
ALSender::perform {
  // ----------- //
  // Send Alerts //
  // ----------- //
  AMapEntry *ae;
  Alert *al;
  char *txt;
  ASock *as, *bs;

  state WaitAlerts:

    Index = 0;
    this->wait (SIGNAL, NextAlert);

  state NextAlert:

    for ( ; Index < NAMapEntries; Index++)
      if ((ae = AMap + Index)->AType == ALERT &&
                         (al = (Alert*)(ae->Al))->getValue () != NONE)
                             break;
    if (Index >= NAMapEntries) proceed WaitAlerts, HIGH;

    // We have a pending alert
    txt = al->getMessage ();
    if (SokList != NULL) {
      makeAlertMessage (Index, al->getValue (), txt);
      al->clearValue ();
    } else {
      Station *s;
      s = uidToStation (ae->UId, ae->UType);
      cerr << "Alert";
      Ouf << "Alert";
      if (s != NULL) {
        cerr << " @" << s->getOName ();
        Ouf << " @" << s->getOName ();
      }
      cerr << ": " << ae->Id << '/' << txt << '\n';
      Ouf << ": " << ae->Id << '/' << txt << '\n';
      cerr.flush ();
      al->clearValue ();
      Index++;
      proceed NextAlert, HIGH;
    }

  transient SendAlert:

    CurSok = SokList;

  transient NextSocket:

    if (CurSok == NULL) {
      // Done with all sockets
      Index++;
      proceed NextAlert, HIGH;
    }

  transient SendCopy:

    if (CurSok->Sok == NULL) {
      // Deleted
      as = CurSok;
      // We have to do it this way because the front of the list may
      // have changed
      if (CurSok == SokList) {
        SokList = CurSok = CurSok->Next;
      } else {
        for (CurSok = SokList; CurSok->Next != as; CurSok = CurSok->Next);
        CurSok->Next = as->Next;
        CurSok = CurSok->Next;
      }
      delete as;
      proceed NextSocket, HIGH;
    }

    if (CurSok->Sok->write (AlertBuffer, AlertMessageLength)) {
      if (CurSok->Sok->isActive ()) {
        CurSok->Sok->wait (OUTPUT, SendCopy);
        sleep;
      }
      // It's not our business to detect and diagnose errors
    }
    dumpBuf ("Sending:", AlertBuffer, AlertMessageLength);

    CurSok = CurSok->Next;
    proceed NextSocket, HIGH;
};

void ALSender::makeAlertMessage (int index, int urgency, const char *txt) {
  int ix, l1;

/*@@@*/fprintf (stderr, "Alert msg: %1d %1d: %s\n", index, urgency, txt);
  AlertBuffer [0] = (char) CMD_ALERT;
  ix = 3;
  AlertBuffer [ix++] = upper (index);
  AlertBuffer [ix++] = lower (index);
  AlertBuffer [ix++] = (char) urgency;

  strncpy (AlertBuffer+ix, tDate () + 11, 8);
  ix += 8;
  strncpy (AlertBuffer+ix, ":", 1);
  ix++;

  if ((l1 = strlen (txt)) > MAXALERTSIZE - ix) l1 = MAXALERTSIZE - ix;
  strncpy (AlertBuffer+ix, txt, l1);
  ix += l1;
  AlertMessageLength = ix;
  ix -= 3;
  AlertBuffer [1] = upper (ix);
  AlertBuffer [2] = lower (ix);
};

Updater::perform {
  // --------------------- //
  // Send periodic updates //
  // --------------------- //
  PUEntry *cu, *pr;

  state Update:

    if ((Current = PUList) != NULL) proceed NextEntry;

  // This is transient to avoid races
  transient Terminate:

    // Deallocate PUList
    while (PUList != NULL) {
      PUList = (cu = PUList)->Next;
      delete cu;
    }
    OPD->MyUpdater = NULL;
    terminate;
  
  state NextEntry:

    if (Current == NULL) {
      Timer->wait (UpdateDelay, Update);
      this->wait (SIGNAL, Terminate);
      sleep;
    }
    if (Current->UType == DELETED) {
      // Remove this entry. We have to start from the head to avoid
      // race with the command processor that may be appending entries
      // at the head.
      for (cu = PUList, pr = NULL; cu != Current; pr = cu, cu = cu->Next);
      if (pr == NULL) PUList = cu->Next; else pr->Next = cu->Next;
      Current = cu->Next;
      delete cu;
      proceed NextEntry, HIGH;
    }

    // Note: a complete update must be sent as a single packet
    switch (Current->UType) {
      case SEGMENT:
      case SSEGMENT:
        UpdateLength = generateSegmentUpdate ((SSegment*)(Current->UN),
                                     Current->UId, UpdateBuffer, MAXUPDATESIZE);
        break;
      case DIVERTER:
      case SDIVERTER:
        UpdateLength = generateDiverterUpdate ((SDiverter*)(Current->UN),
                                     Current->UId, UpdateBuffer, MAXUPDATESIZE);
        break;
      case MERGER:
      case SMERGER:
        UpdateLength = generateMergerUpdate ((SMerger*)(Current->UN),
                                     Current->UId, UpdateBuffer, MAXUPDATESIZE);
        break;
      case SOURCE:
        UpdateLength = generateSourceUpdate ((Source*)(Current->UN),
                                     Current->UId, UpdateBuffer, MAXUPDATESIZE);        break;
      case SINK:
        UpdateLength = generateSinkUpdate ((Sink*)(Current->UN),
                                     Current->UId, UpdateBuffer, MAXUPDATESIZE);
        break;
      default:
        excptn ("Updater: invalid update type");
    }

  transient SendUpdate:


    if (OP->write (UpdateBuffer, UpdateLength)) {
      if (OP->isActive ()) {
        this->wait (SIGNAL, Terminate);
        OP->wait (UpdateLength, SendUpdate);
        sleep;
      }
      OPD->signal ();            // Make sure OPDriver knows about this
      Current->UType = DELETED;  // Will be deleted in the next turn
    }
    dumpBuf ("Sending:", UpdateBuffer, UpdateLength);
    Current = Current->Next;
    proceed NextEntry, HIGH;
};

void Updater::setup (OPDriver *op) {
  OP = (OPD = op) -> OP;
  PUList = Current = NULL;
  //trace ("New Updater");
};

PUEntry::PUEntry (SUnit *u, int uid, int tp, OPDriver *pr) {
  // Add a new periodic update entry to the list
  UN = u;
  UType = tp;
  UId = uid;
  if (pr->MyUpdater == NULL)
    pr->MyUpdater = create Updater (pr);
  Next = pr->MyUpdater->PUList;  // Must add at front to avoid race condition
  pr->MyUpdater->PUList = this;
};
 
void OPDriver::setup (Socket *master) {
  OP = create Socket;
  MyUpdater = NULL;
  Terminating = NO;
  //trace ("New OPDriver connecting");
  if (OP->connect (master, OPBUFSIZE) != OK) {
    // We have failed -- rather unlikely
    delete OP;
    terminate ();
    return;
  }
  //trace ("New OPDriver succeeded");
  // Report the mailbox to ALSender
  NConnections++;
  setLimit (0, TIME_0);
  ALS->report (OP);
};

void OPDriver::makeErrorMessage (const char *txt) {

  int ix, l1;

  Cmd [0] = (char) CMD_ALERT;
  Cmd [1] = (char) 0;          // Ignored -- for compatibility with alerts
  Cmd [2] = (char) 0;
  Cmd [3] = (char) X_BADCMD;

  ix = 4;
  if ((l1 = strlen (txt)) > CMDBUFSIZE - ix) l1 = CMDBUFSIZE - ix;
  strncpy (Cmd+ix, txt, l1);
  ix += l1;
  OutPtr = ix;
  ix -= 3;
  Cmd [1] = upper (ix);
  Cmd [2] = lower (ix);
};

OPDriver::perform {

  // ------------------- //
  // Command interpreter //
  // ------------------- //

  int l1, ix, act, val, uid, utype;
  SUnit *su;
  PUEntry *pu;
  AMapEntry *ae;

  state WaitCommand:

    this->wait (SIGNAL, Disconnect);
    readRequest (Cmd, CMDHDRL, WaitCommand);

  transient WaitTail:

    if ((CommandLength = CMDLENGTH (Cmd)) > 0)
      readRequest (Cmd+CMDHDRL, CommandLength, WaitTail);

    dumpBuf ("Receiving:", Cmd, CommandLength + 3);

    // We are ready to process the command

// ---------------- //
// -- CMD_GETMAP -- //
// ---------------- //
  transient DoGetMap:

    if (COMMAND (Cmd) != CMD_GETMAP) proceed DoGetStatus, URGENT;

    // Send in response the contents of the alert map
    Index = 0;

  transient ContinueGetMap:

    if (Index == NAMapEntries) proceed WaitCommand;

    Cmd [0] = CMD_GETMAP;
    OutPtr = 3;
    Cmd [OutPtr++] = upper (Index);  // Entry number -- for future reference
    Cmd [OutPtr++] = lower (Index);
    ae = AMap + Index;

    // Convert real stations to shadow stations, so that all alerts
    // appear uniformly owned

    switch (ae->UType) {
      case SEGMENT:
        utype = SSEGMENT;
        uid = shadow (ae->UId, ae->UType);
        break;
      case DIVERTER:
        utype = SDIVERTER;
        uid = shadow (ae->UId, ae->UType);
        break;
      case MERGER:
        utype = SMERGER;
        uid = shadow (ae->UId, ae->UType);
        break;
      default:
        uid = ae->UId;
        utype = ae->UType;
    }
    Assert (uid != NONE, "Shadow station not found");

    Cmd [OutPtr++] = upper (uid);     // Owning station
    Cmd [OutPtr++] = lower (uid);
    Cmd [OutPtr++] = (char) (utype);  // And its type

    Cmd [OutPtr++] = (char) (ae->AType);  // Alert type
    l1 = strlen (ae->Id);
    if (l1 > CMDBUFSIZE - OutPtr) l1 = CMDBUFSIZE - OutPtr;
    strncpy (Cmd+OutPtr, ae->Id, l1);
    OutPtr += l1;
    ix = OutPtr - 3;
    Cmd [1] = upper (ix);
    Cmd [2] = lower (ix);

  transient SendGetMap:

    sendData (Cmd, OutPtr, SendGetMap);
    dumpBuf ("Sending:", Cmd, OutPtr);
    Index++;
    proceed ContinueGetMap, HIGH;

// ----------------- //
// -- CMD_GETSTAT -- //
// ----------------- //
  state DoGetStatus:

    if (COMMAND (Cmd) != CMD_GETSTAT) proceed DoPeriodic, URGENT;
    // Send in response the unit status (a one-time update)

    uid = extshort (3);
    utype = (int) Cmd [5];
    switch (utype) {
      // Convert user-visible types to internal
      case SEGMENT:    utype = SSEGMENT;  break;
      case DIVERTER:   utype = SDIVERTER; break;
      case MERGER:     utype = SMERGER;   break;
      default: ;
    }
    su = (SUnit*) uidToStation (uid, utype);
    if (su == NULL) {
      // No such device, create something looking like an alert message
      makeErrorMessage ("Unit not found");
    } else {
      switch (utype) {
        case SSEGMENT:
         OutPtr = generateSegmentUpdate ((SSegment*)su, uid, Cmd, CMDBUFSIZE);
         break;
        case SDIVERTER:
         OutPtr = generateDiverterUpdate ((SDiverter*)su, uid, Cmd, CMDBUFSIZE);
         break;
        case SMERGER:
         OutPtr = generateMergerUpdate ((SMerger*)su, uid, Cmd, CMDBUFSIZE);
         break;
        case SOURCE:
         OutPtr = generateSourceUpdate ((Source*)su, uid, Cmd, CMDBUFSIZE);
         break;
        case SINK:
         OutPtr = generateSinkUpdate ((Sink*)su, uid, Cmd, CMDBUFSIZE);
         break;
        default:
         // Not very likely
         makeErrorMessage ("Unit not found");
      }
    }

  transient SendStatus:

    sendData (Cmd, OutPtr, SendStatus);
    dumpBuf ("Sending:", Cmd, OutPtr);
    proceed WaitCommand;

// ------------------ //
// -- CMD_PERIODIC -- //
// ------------------ //
  state DoPeriodic:

    if (COMMAND (Cmd) != CMD_PERIODIC) proceed DoUnPeriodic, URGENT;
    // There is no response, unless we hit an error
    uid = extshort (3);
    utype = (int) Cmd [5];
    switch (utype) {
      // Convert user-visible types to internal
      case SEGMENT:    utype = SSEGMENT;  break;
      case DIVERTER:   utype = SDIVERTER; break;
      case MERGER:     utype = SMERGER;   break;
      default: ;
    }
    su = (SUnit*) uidToStation (uid, utype);
    if (su == NULL) {
      // No such device, create something looking like an alert message
      makeErrorMessage ("Unit not found");
      proceed SendStatus, URGENT;
    } 
    pu = new PUEntry (su, uid, utype, this);
    proceed WaitCommand;

// -------------------- //
// -- CMD_UNPERIODIC -- //
// -------------------- //
  state DoUnPeriodic:

    if (COMMAND (Cmd) != CMD_UNPERIODIC) proceed DoOverride, URGENT;
    // There is no response, unless we hit an error
    uid = extshort (3);
    utype = (int) Cmd [5];
    switch (utype) {
      // Convert user-visible types to internal
      case SEGMENT:    utype = SSEGMENT;  break;
      case DIVERTER:   utype = SDIVERTER; break;
      case MERGER:     utype = SMERGER;   break;
      default: ;
    }
    su = (SUnit*) uidToStation (uid, utype);
    if (su == NULL) {
      // No such device, create something looking like an alert message
      makeErrorMessage ("Unit not found");
      proceed SendStatus, URGENT;
    }  
    for (pu = MyUpdater->PUList; pu != NULL; pu = pu->Next)
      if (pu->UN == su) pu->UType = DELETED;
      // The actual job will be done by the updater process. We cannot
      // do it ourselves, because this action might cause a race condition
      // with the updater.
    proceed WaitCommand;

// ------------------ //
// -- CMD_OVERRIDE -- //
// ------------------ //
  state DoOverride:

    if (COMMAND (Cmd) != CMD_OVERRIDE) proceed DoGetGraph, HIGH;
    ix  = extshort (3);  // Override id (index)
    act = extshort (5);  // Action
    val = extshort (7);  // Value

    if (ix < 0 || ix >= NAMapEntries) {
      makeErrorMessage ("Illegal override/alert index");
      proceed SendStatus, URGENT;
    }
    ae = AMap + ix;
    if (ae->AType != OVERRIDE) {
      makeErrorMessage ("Not an override");
      proceed SendStatus, URGENT;
    }
    ((Override*)(ae->Al)) -> force (act, val);
    proceed WaitCommand;

// ------------------ //
// -- CMD_GETGRAPH -- //
// ------------------ //
  state DoGetGraph:

    if (COMMAND (Cmd) != CMD_GETGRAPH) proceed DoStart, HIGH;

    Index = 0;  // This will go through all stations in the network

  transient ContinueGetGraph:

    do {
      if (Index == NStations) proceed WaitCommand;  // Done
      uid = idToUId (Index, utype);
      if (uid != NONE && utype != SEGMENT && utype != DIVERTER &&
                                                         utype != MERGER) break;
      Index++;
      // We ignore "real" stations and only deal with the shadow ones
    } while (1);

    Cmd [0] = CMD_GETGRAPH;
    OutPtr = 3;
    Cmd [OutPtr++] = upper (uid);     // Unit Id
    Cmd [OutPtr++] = lower (uid);
    Cmd [OutPtr++] = lower (utype);   // And its type

    // Now, take care of outgoing links
    if (utype != SINK) {
      // Only sinks don't have outgoing links
      su = ((SUnit*)(idToStation (Index))) -> NU;
      ix = idToUId (su->getId (), act);
      Assert (ix != NONE, "OPDriver: inconsistent graph structure");
      // This is the primary link
      Cmd [OutPtr++] = upper (ix);     // Unit Id
      Cmd [OutPtr++] = lower (ix);
      Cmd [OutPtr++] = lower (act);    // And its type
      if (utype == SDIVERTER) {
        // This one has a secondary link
        su = ((SDiverter*)(idToStation (Index))) -> NDU;
        ix = idToUId (su->getId (), act);
        Assert (ix != NONE, "OPDriver: inconsistent graph structure");
        // This is the secondary link
        Cmd [OutPtr++] = upper (ix);     // Unit Id
        Cmd [OutPtr++] = lower (ix);
        Cmd [OutPtr++] = lower (act);    // And its type
      }
    }

    ix = OutPtr - 3;
    Cmd [1] = upper (ix);
    Cmd [2] = lower (ix);

  transient SendGetGraph:

    sendData (Cmd, OutPtr, SendGetGraph);
    dumpBuf ("Sending:", Cmd, OutPtr);
    Index++;
    proceed ContinueGetGraph, HIGH;

// --------------- //
// -- CMD_START -- //
// --------------- //
  state DoStart:

    if (COMMAND (Cmd) != CMD_START) proceed DoDisconnect, HIGH;
    TheRoot->signal ();
    proceed WaitCommand;

// -------------------- //
// -- CMD_DISCONNECT -- //
// -------------------- //
  state DoDisconnect:

    if (COMMAND (Cmd) != CMD_DISCONNECT) proceed DoStop, HIGH;
    proceed Disconnect;

// -------------- //
// -- CMD_STOP -- //
// -------------- //
  state DoStop:

    if (COMMAND (Cmd) != CMD_STOP) proceed DoDefault, HIGH;
    Terminating = YES;

  transient Disconnect:

    // Terminate the other process

    //trace ("OPDriver disconnecting");
    if (MyUpdater != NULL) {
      MyUpdater->signal ();
      MyUpdater->wait (DEATH, Disconnect);
      sleep;
    }
    //trace ("OPDriver MyUpdater gone");

    // Check if the mailbox is still alive
    if (OP->isActive ()) {
      // Confirm disconnection
      Cmd [0] = CMD_DISCONNECT;
      Cmd [1] = Cmd [2] = (char) 0;
      OutPtr = 3;
      if (OP->write (Cmd, OutPtr)) {
        OP->wait (OUTPUT, Disconnect);
        sleep;
      }
    }

  transient CloseConnection:

    //trace ("OPDriver closing");
    OP->erase ();
    if (OP->disconnect (CLIENT) == ERROR) {
      Timer->wait ((TIME)(DISCONNDELAY * Second), CloseConnection);
      sleep;
    }

    //trace ("OPDriver cleaning up");
    // CLeanup
    ALS->remove (OP);
    delete OP;
    if (Terminating)
      Kernel->terminate ();
    //trace ("OPDriver terminating");
    if (--NConnections <= 0) {
      // The last one turns out the lights
      setLimit (0, Time+IdleTime);
    }
    terminate;

  state DoDefault:

    makeErrorMessage ("Unimplemented command");
    proceed SendStatus, URGENT;
};

void initOperator () {
  int PortNumber;
  double Idle;
  readIn (PortNumber);
  readIn (Idle);
  UpdateDelay = (TIME)(Second * DEFAULT_UPDATE_DELAY);
  IdleTime = (TIME)(Second * Idle);
  TheStation = System;
  create Server (PortNumber);
  ALS = create ALSender;
  setLimit (0, IdleTime);
};
