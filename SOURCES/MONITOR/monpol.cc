/* ooooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 2003-2017   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooooo */

// ---------------------------------------------------------------------
// This is the polling version of the monitor that assumes no select and
// doesn't fork. To be used with Windows/DOS.
// ---------------------------------------------------------------------

/* --- */

// #define	CDEBUG 1

#include        "../version.h"
#include        "../LIB/lib.h"
#include	"../LIB/socklib.h"

#include        "monitor.h"

#include        "../KERNEL/cdebug.h"
#include        "../KERNEL/display.h"
#include	"../KERNEL/sockbuf.cc"

long			MemoryUsed,     // Ignored -- for local library

                        CurrentDelay;   // Current polling interval

int                     Master;         // The master socket

// ------------------------------
// Thread states (0 means zombie)
// ------------------------------
#define	ST_READSIGNATURE              1
#define	ST_WAIT                       2
#define ST_WRITING                    3
#define ST_FLUSHANDDIE                4
#define ST_READING                    5
#define	ST_FLUSHINQUIRY               6
#define	ST_GETREPLY                   7
#define	ST_INQTIMEOUT                 8
#define	ST_MAKEINQUIRY                9

int NextHandle = 0, NextPID = 0;

int dscrhalf, dscrlast;    // For reading and writing numbers

class Thread {
 public:
  Thread *Next;
  int State, NPId;
  int InputSocket, OutputSocket;
  SockBuff *InputBuffer, *OutputBuffer;
  Long Timer;
  Thread ();
  virtual ~Thread ();
  virtual int run () { /* The code */ };
  void setRead (int s, SockBuff *sb) {
    InputSocket = s;
    InputBuffer = sb;
  };
  void setWrite (int s, SockBuff *sb) {
    OutputSocket = s;
    OutputBuffer = sb;
  };
  void kill () { State = 0; };
  void resetTimer ();
  int timeout (int);
  void sendText (const char*);
  void sendError (int, const char*);
  int getText (char*, int);
  int getDescriptor ();
  LONG getInt ();
  void sendDescriptor (int);
  void flushDescriptor ();
  void sendInt (LONG);
};

Thread *ThreadList;

// We need these for Template::send (from dshared), because that method
// assumes that sendOctet, etc. refer to a global buffer.
SockBuff *OutputBuffer;
Thread *ThisThread;

class ConnectThread : public Thread {
 SockBuff *SkBuffer, *InqBuffer;
 int Sock;
 // For sending out and collecting status requests
 SmDesc *CurrentStatus;  int StatusCount;
 public:
  ConnectThread (int sk) {
    Sock = sk;
    SkBuffer = new SockBuff (LSKBUFSIZE);
    InqBuffer = NULL;
    setRead (Sock, SkBuffer);
    InputBuffer->clear ();
    State = ST_READSIGNATURE;
  };
  virtual ~ConnectThread () {
    cdebugl ("ConnectThread destruct");
    if (SkBuffer != NULL) delete SkBuffer;
    if (InqBuffer != NULL) delete InqBuffer;
  };
  int run ();
  int signature ();
  int newSmurph ();
  void createInfo ();
  int createStatusInquiry ();
  int setupDisplayThreads ();
};

class GarbageThread : public Thread {
 SockBuff *SkBuffer;
 public:
  GarbageThread () {
    State = 1; // We are not a zombie!
    SkBuffer = new SockBuff (LSKBUFSIZE);
  };
  virtual ~GarbageThread () {
    cdebugl ("GarbageThread destruct");
    delete SkBuffer;
  };
  int run ();
};

class smdThread : public Thread {
 SockBuff *SkBuffer;
 int ISock, OSock;
 TmpList *TempHead;
 int ending;
 public:
  Thread *Other;
  smdThread (int isock, int osock) {
    ending = NO;
    ISock = isock;
    OSock = osock;
    TempHead = NULL;
    SkBuffer = new SockBuff (LRGBUFSIZE);
    setRead (ISock, SkBuffer);
    setWrite (OSock, SkBuffer);
    InputBuffer->clear ();
    State = ST_WAIT;
  };
  virtual ~smdThread () {
    cdebugl ("smdThread destruct");
    delete SkBuffer;
  };
  int run ();
  int parseSmurphBlock ();
  void sendTemplates ();
  void queueTemplates (const char*, const char*, const char*);
};

class dsmThread : public Thread {
 SockBuff *SkBuffer;
 int ISock, OSock;
 public:
  Thread *Other;
  dsmThread (int isock, int osock) {
    ISock = isock;
    OSock = osock;
    SkBuffer = new SockBuff (LRGBUFSIZE);
    setRead (ISock, SkBuffer);
    setWrite (OSock, SkBuffer);
    OutputBuffer->reset (PH_DRQ);
    OutputBuffer->close ();
    State = ST_WRITING;
  };
  virtual ~dsmThread () {
    cdebugl ("dsmThread destruct");
    delete SkBuffer;
  };
  int run ();
};

Thread::Thread () {
  Thread *t;
  while (1) {
    // Allocate unique process ID. We need it for locking.
    if (++NextPID == 0) NextPID = 1;
    for (t = ThreadList; t != NULL; t = t->Next)
      if (t->NPId == NextPID) break;
    if (t == NULL) break;
  }
  resetTimer ();
  State = 0;
  Next = ThreadList;
  ThreadList = this;
  NPId = NextPID;
  cdebugl (form ("New thread: %1d", NPId));
};

void Thread::resetTimer () {
  struct timeval tm;
  gettimeofday (&tm, NULL);
  // We only use seconds
  Timer = tm.tv_sec;
};

int Thread::timeout (int secs) {
  struct timeval tm;
  gettimeofday (&tm, NULL);
  return tm.tv_sec >= Timer + secs;
};

int SockBuff::read (int sock, int block) {
//
// Read the buffer
//
// Note that the second argument is unused. We leave it for compatibility
// with SockBuff used by DSD (we recycle here exactly the same SockBuff
// structure). The I/O is always non-blocking.
//
// Return: OK,ERROR,NONE (NONE is for "try again")
//
// The buffer should be 'cleared' before reading to reset pointers
//
  int nr, nc, rl;
  while (fill < 4) {
    // We still don't know how much to expect
    nc = 4 - fill;
    if ((nr = readSocket (sock, Buffer + fill, nc)) <= 0)
      return (nr < 0 && errno == EAGAIN) ? NONE : ERROR;
    fill += nr;
  }
  if (fill == (rl = tlength ())) {
    ptr = 0;
    cdebugl (form ("Read (%1d) %1d, len = %1d", sock, Buffer [0], fill));
    return OK;  // Got it
  }
  if (rl > size) grow (rl);       // We have to grow the buffer
  // Now we check whether we can read more right away
  while (fill < rl) {
    nc = rl - fill;
    if ((nr = readSocket (sock, Buffer + fill, nc)) <= 0)
      return (nr < 0 && errno == EAGAIN) ? NONE : ERROR;
    fill += nr;
  }
  ptr = 0;
  cdebugl (form ("Read (%1d) %1d, len = %1d", sock, Buffer [0], fill));
  return OK;
};

int SockBuff::write (int sock, int block) {
//
// Write the buffer. Second argument unused.
//
  int nc, nr;
  while (1) {
    if ((nc = fill - ptr) > AIOGRANL) nc = AIOGRANL;
    // We only try this much at a time
    if ((nr = writeSocket (sock, Buffer + ptr, nc)) < 0)
      return (errno == EAGAIN) ? NONE : ERROR;
    if ((ptr += nr) == fill) {
      cdebugl (form ("Written (%1d) %1d, len = %1d", sock, Buffer [0], fill));
      fill = 0;
      return OK;
    }
  }
};

/* ------------------------------------------------------------ */
#define sendOctet(val)   OutputBuffer->out(val)
#define sendChar(val)    OutputBuffer->out((val)&0x7f)
#define peekOctet()      InputBuffer->peek()
#define getOctet()       InputBuffer->inp()
#define more()           InputBuffer->more()
/* ------------------------------------------------------------ */

void sendText (const char *t) { ThisThread->sendText (t);  };
void sendInt (LONG val)       { ThisThread->sendInt (val); };

void Thread::sendError (int err, const char *txt) {
  sendOctet (PH_ERR);
  sendChar (err);
  sendText (txt);
};

int Thread::getText (char *t, int len) {
  int go;
  if (peekOctet () != PH_TXT) return ERROR;
  getOctet ();
  while ((go = getOctet ()) >= 0) {
    if (go == 0) {
      *t = '\0';
      return OK;
    }
    if (len) {
      *t++ = (char) go;
      len--;
    }
  }
  return ERROR;
};

int Thread::getDescriptor () {
/* --------------------------------------------------- */
/* Gets one numeric descriptor from the input sequence */
/* --------------------------------------------------- */
  register int    rslt;
  if (dscrhalf) {
    // Take the second half
    dscrhalf = NO;
    rslt = dscrlast & 017;
  } else {
    // Get a new byte
    dscrlast = getOctet ();
    dscrhalf = YES;
    rslt = (dscrlast >> 4) & 017;
  }
  return (rslt);
};

void Thread::sendText (const char *t) {
/* ------------------------------ */
/* Send text string to the server */
/* ------------------------------ */
  sendOctet (PH_TXT);
  while (*t != '\0') sendChar ((int)(*t++));
  sendOctet (0);
};

LONG Thread::getInt () {
/* --------------------------------------------- */
/* Reads an integer number from the input stream */
/* --------------------------------------------- */
  LONG    res;
  int     des;
  // This shouldn't really happen
  if (peekOctet () != PH_INM) return (0);
  getOctet ();
  res = 0; dscrhalf = NO;
  while (1) {
    if ((des = getDescriptor ()) == DSC_ENP) // Positive number
      return (-res);
    else if (des == DSC_ENN)                 // Negative number
      return (res);
    else if (des > 9) { 
      // This is an error, but we don't care. Let us pretend everything is
      // all right.
      des = 0;
    }
    res = res * 10 - des;
  }
  return (res);
};

void Thread::sendDescriptor (int d) {
/* ----------------------------- */
/* Send one numerical descriptor */
/* ----------------------------- */
  if (dscrhalf) {
    sendOctet (dscrlast | (d & 017));
    dscrhalf = NO;
  } else {
    dscrhalf = YES;
    dscrlast = (d & 017) << 4;
  }
};

void Thread::flushDescriptor () {
/* ------------------------------------------------------ */
/* Flush a partially filled descriptor byte to the server */
/* ------------------------------------------------------ */
  if (dscrhalf) {
    sendOctet (dscrlast);
    dscrhalf = NO;
  }
};

void Thread::sendInt (LONG val) {
/* ------------------------------------ */
/* Send an integer number to the server */
/* ------------------------------------ */
  int     digits [16], ndigits, sig;
  sendOctet (PH_INM);
  dscrhalf = NO;
  if (val >= 0) {
    sig = NO;
    val = -val;
  } else
    sig = YES;
  for (ndigits = 0; val; ) {
    digits [ndigits++] = - (int)(val % 10);
    val /= 10;
  }
  while (ndigits--) sendDescriptor (digits [ndigits]);
  sendDescriptor (sig ? DSC_ENN : DSC_ENP);
  flushDescriptor ();
};

void excptn (const char *t) {
  char *s;
  write (2, t, strlen (t));
  write (2, "\n", 1);
  t = "Monitor aborted\n";
  write (2, t, strlen (t));
  exit (99);
};

#define tabt(t) excptn (t)

char *zz_TemplateFile = NULL;

SmDesc  *Smurphs = NULL;                // The list of smurphs running in the
					// network
SmDesc::SmDesc () {
/* --------------- */
/* The constructor */
/* --------------- */
  // Add the new item to the list
  if ((next = Smurphs) != NULL) {
    Smurphs->prev = this;
  }
  Smurphs = this;
  RPId = 0;
  prev = NULL;
};

SmDesc::~SmDesc () {
/* -------------- */
/* The destructor */
/* -------------- */
  if (next != NULL) next->prev = prev;
  if (prev != NULL) prev->next = next; else Smurphs = next;
};

Thread::~Thread () {
  // Remove this thread from the list
  Thread *th, *tq;
  SmDesc *sm;
  for (sm = Smurphs; sm != NULL; sm = sm->next)
    // Drop all locks pointing to yourself
    if (sm->RPId == NPId) sm->RPId = 0;
  for (tq = NULL, th = ThreadList; th != NULL; tq = th, th = th -> Next) {
    if (th == this) {
      if (tq == NULL)
        ThreadList = th->Next;
      else
        tq->Next = th->Next;
      cdebugl (form ("Thread exit: %1d", NPId));
      return;
    }
  }
};

#include "../KERNEL/dshared.cc"

void shortenText (char *t, int length) {
  int k, ul, st, i;
  if ((k = strlen (t)) <= length) return;
  ul = st = (length - 3) / 2;
  t [st++] = '.';
  t [st++] = '.';
  t [st++] = '.';
  if ((length & 1) == 0) t [st++] = '.';
  for (i = k - ul; i <= k; i++) t [st++] = t [i];
};

int ConnectThread::newSmurph () {
/* --------------------------------------- */
/* Process a new smurph connection request */
/* --------------------------------------- */
  char    HName [128],    // Hostname
          PArgs [128],    // Program call arguments
          DTime [32] ;    // Startup date/time
  Long    PId,            // Process ID
          dummy;
  int     i;
  char    c;
  SmDesc  *sm;

  cdebugl ("newSmurph called:");
  // Read the host name
  if (getText (HName, 127) == ERROR) return ERROR;
  cdebugl (".");
  // Read the program call line
  if (getText (PArgs, 127) == ERROR) return ERROR;
  // This can be occasionally long. Make sure to include beginning and end.
  shortenText (PArgs, SML_PARG);
  cdebugl (".");
  // Read the startup date/time
  if (getText (DTime,  31) == ERROR) return ERROR;
  cdebugl (".");
  // Read the process ID
  if ((PId = getInt ()) == 0) return ERROR;
  cdebugl (".");

  sm = new SmDesc;
  strcpy (sm->HName, HName);
  strcpy (sm->PArgs, PArgs);
  strcpy (sm->DTime, DTime);
  sm->PId = PId;
  sm->socket = Sock;
  sm->Handle = NextHandle++;
  cdebug ("New SMURPH: ");
  cdebugl (form ("%1d, %1d, %1d, %s, %s, %s", sm->PId, sm->socket, sm->Handle, sm->HName, sm->PArgs, sm->DTime));
  // Make sure handles are always nonnegative (I'm not sure if wee need it)
  if (NextHandle < 0) NextHandle = 0;
  return OK;
};

void smdThread::queueTemplates (const char *sn, const char *bn,
                                                               const char *nn) {
// ----------------------------------------------------------
// Queues templates matching the three names for transmission
// ----------------------------------------------------------
  Template *ct;
  TmpList *tl, *tq;
  for (ct = Templates; ct; ct = ct->next) {
    if (tmatch (ct, sn) || tmatch (ct, bn) || tmatch (ct, nn)) {
      // Check if the template is not queued already
      if ((tl = TempHead) != NULL) {
        while (1) {
          if (tl->tmplt == ct) goto Next;
          if (tl->next == NULL) break;
          tl = tl->next;
        }
      }
      tq = new TmpList (ct);
      if (tl == NULL)
        TempHead = tq;
      else
        tl->next = tq;
    }
Next: ;
  }
};

int smdThread::parseSmurphBlock () {
// ---------------------------------
// Locate templates and exit phrases
// ---------------------------------
  int go, ending;
  char *sn, *bn, *nn;
  InputBuffer->header ();   // Set up pointers
  TempHead = NULL;
  ending = NO;
  while (more ()) {
    if ((go = getOctet ()) == PH_END) ending = YES;
    if (go == PH_MEN) {
      // This is a list of menu objects. We have to locate all our templates
      // for those objects and send them to DSD.
      while (1) {
        if (getOctet () != PH_TXT) break;
        sn = (char*)(InputBuffer->Buffer + InputBuffer->ptr);
        while (getOctet () != 0);
        if (getOctet () != PH_TXT) break;
        bn = (char*)(InputBuffer->Buffer + InputBuffer->ptr);
        while (getOctet () != 0);
        if (getOctet () != PH_TXT) break;
        // We expect three of those
        nn = (char*)(InputBuffer->Buffer + InputBuffer->ptr);
        while (getOctet () != 0);
        queueTemplates (sn, bn, nn);
      }
    }
  }
  return ending;
};

void smdThread::sendTemplates () {
  TmpList *tl;
  OutputBuffer->reset (PH_BLK);
  // Template::send assumes that the buffer is global
  ::OutputBuffer = OutputBuffer;
  ::ThisThread = this;
  while (TempHead) {
    TempHead->tmplt->send ();
    TempHead = (tl = TempHead) -> next;
    delete tl;
  }
  OutputBuffer->close ();
};

int ConnectThread::setupDisplayThreads () {
/* ----------------------- */
/* Process display request */
/* ----------------------- */
  Long Handle;
  SmDesc *sp;
  Thread *th;
  // Read the handle
  Handle = getInt ();
  // Locate the handle
  for (sp = Smurphs; sp; sp = sp -> next) if (sp->Handle == Handle) break;
  if (sp == NULL) {
    // No such process
    setWrite (Sock, SkBuffer);
    OutputBuffer->reset (PH_MBL);  // Start monitor block
    sendError (DERR_ONF, "No such process");
    cdebugl (form ("D: No such process %1d", Handle));
    OutputBuffer->close ();
    return ERROR;
  }
  if (sp -> RPId) {
    // Process busy
    setWrite (Sock, SkBuffer);
    OutputBuffer->reset (PH_MBL);  // Start monitor block
    sendError (DERR_UNV, "Process busy");
    OutputBuffer->close ();
    return ERROR;
  }
  th = new smdThread (sp->socket, Sock);
  ((smdThread*)th)->Other = (Thread*) new dsmThread (Sock, sp->socket);
  ((dsmThread*)(((smdThread*)th)->Other)) -> Other = th;
#if 0
  (((dsmThread*)((smdThread*)th)->Other) = new dsmThread (Sock, sp->socket))
                                                                 -> Other = th;
#endif
  sp->RPId = th->NPId;
  return OK;
};

int smdThread::run () {
/* ------------------------ */
/* The Smurph to DSD thread */
/* ------------------------ */

  int rv;
  Thread *th;

Redo:

  switch (State) {

    case ST_WAIT:
      // Expect handshake message

      if (timeout (STIMEOUT)) {
        // This is the only timeout in this thread. Other than that, we
        // never know for how long we should wait.
        cdebugl ("smdThread ST_WAIT timeout");
        OutputBuffer->reset (PH_MBL);
        sendError (DERR_NRP, "Reply timeout");
        OutputBuffer->close ();
        resetTimer ();
        State = ST_FLUSHANDDIE;
        goto Redo;
      }

      if ((rv = InputBuffer->read (InputSocket)) == NONE) return NONE;
      if (rv == ERROR) {
        cdebugl ("smdThread ST_WAIT ERROR");
Disconnect:
        cdebugl ("smdThread Disconnect");
        OutputBuffer->reset (PH_BLK);
        sendOctet (PH_END);
        OutputBuffer->close ();
        State = ST_FLUSHANDDIE;
        resetTimer ();
        goto Redo;
      }

      // This must be a BLK message for a handshake

      cdebugl ("sdm buffer read"); // dumpbuf (InputBuffer);

      if (InputBuffer->header () != PH_BLK) {
        cdebugl ("smdThread ST_WAIT Illegal block");
        OutputBuffer->reset (PH_MBL);
        sendError (DERR_ILS, "Illegal response");
        OutputBuffer->close ();
        State = ST_FLUSHANDDIE;
        resetTimer ();
        goto Redo;
      }

      cdebugl ("smdThread ST_WAIT block OK");
      State = ST_WRITING;

      ending = parseSmurphBlock ();
      // Reset pointer for writing
      OutputBuffer->close ();

    case ST_WRITING:
      // Write a block to DSD

      cdebugl ("sdm buffer written"); // dumpbuf (OutputBuffer);
      if ((rv = OutputBuffer->write (OutputSocket)) == NONE) return NONE;
      if (rv == ERROR) goto Kill;
      cdebugl ("smdThread ST_WRITING OK");
      if (ending) {
        // This is the end -- nothing more will arrive from either party
        cdebugl ("smdThread ST_WRITING ending");
        goto Kill;
      }
      if (TempHead) {
        // We have to send some templates
        sendTemplates ();
        cdebugl ("smdThread ST_WRITING re-sending for templates");
        // sendTemplates sets the buffer pointer correctly
        goto Redo;
      }

      // Read next buffer

      InputBuffer->clear ();
      State = ST_READING;

    case ST_READING:

      if ((rv = InputBuffer->read (InputSocket)) == NONE) return NONE;
      if (rv == ERROR) goto Disconnect;
      cdebugl ("smdThread ST_READING OK");

      ending = parseSmurphBlock ();

      OutputBuffer->close ();

      State = ST_WRITING;
      goto Redo;

    case ST_FLUSHANDDIE:

      if (!timeout (STIMEOUT)) {
        if (OutputBuffer->write (OutputSocket) == NONE) return NONE;
      }
Kill:
      // Close our output end -- the DSD socket
      cdebugl ("smdThread ST_FLUSHANDDIE Closing");
      closeSocket (OutputSocket);

      for (th = ThreadList; th != NULL; th = th->Next) {
        if (th == Other) {
          Other->kill ();
          break;
        }
      }
      return OK;

    default:
      excptn ("Illegal state in smdThread");
  }
};

int dsmThread::run () {
/* ------------------------ */
/* The DSD to Smurph thread */
/* ------------------------ */

  int rv;
  Thread *th;

Redo:

  switch (State) {

    case ST_WRITING:
      // Write a block to Smurph

      if (timeout (STIMEOUT)) {
        cdebugl ("dsmThread ST_WRITING timeout");
        goto Kill;
      }
      if ((rv = OutputBuffer->write (OutputSocket)) == NONE) return NONE;
      if (rv == ERROR) {
        cdebugl ("dsmThread ST_WRITING ERROR");
        perror ("ERROR:");
        goto Kill;
      }

      cdebugl ("dsmThread ST_WRITING OK");

      InputBuffer->clear ();
      State = ST_READING;

    case ST_READING:

      if ((rv = InputBuffer->read (InputSocket)) == NONE) return NONE;
      if (rv == ERROR) {
        cdebugl ("dsmThread ST_READING ERROR");
        OutputBuffer->reset (PH_END);
        OutputBuffer->close ();
        resetTimer ();
        State = ST_FLUSHANDDIE;
        goto Redo;
      }
      cdebugl ("dsmThread ST_READING OK");

      // read leaves PTR at 0
      State = ST_WRITING;
      resetTimer ();
      goto Redo;

    case ST_FLUSHANDDIE:

      if (!timeout (STIMEOUT)) {
        if (OutputBuffer->write (OutputSocket) == NONE) return NONE;
      }
Kill:
      // Close our input end -- the DSD socket
      cdebugl ("dsmThread ST_FLUSHANDDIE Closing");
      closeSocket (InputSocket);

      for (th = ThreadList; th != NULL; th = th->Next) {
        if (th == Other) {
          Other->kill ();
          break;
        }
      }
      return OK;

    default:
      excptn ("Illegal state in dsmThread");

  }
};

void ConnectThread::createInfo () {
/* -------------------- */
/* Process Info request */
/* -------------------- */
  Long Handle;
  SmDesc *sp;

  setWrite (Sock, SkBuffer);      // We are going to send something
  OutputBuffer->reset (PH_MBL);   // Start monitor block
  for (sp = Smurphs; sp; sp = sp->next) {
    sendOctet (PH_MEN);
    sendInt (sp->Handle);
    cdebugl (form ("Sending info handle: %1d", sp->Handle));
    sendInt (sp->PId);
    sendText (sp->HName);
    sendText (sp->PArgs);
    sendText (sp->DTime);
  }
  OutputBuffer->close ();
};

int ConnectThread::createStatusInquiry () {
  Long Handle;
  SmDesc *sp;


  if (more ()) {
    // The request byte is followed by a handle, thus this is a single
    // status request
    Handle = getInt ();
    // Locate the handle
    setWrite (Sock, SkBuffer);       // We are going to send something
    OutputBuffer->reset (PH_MBL);    // Start monitor block
    for (sp = Smurphs; sp; sp = sp -> next) if (sp->Handle == Handle) break;
    if (sp == NULL) {
      // No such process
      sendError (DERR_ONF, "No such process");
      cdebugl (form ("I: No such process %1d", Handle));
      OutputBuffer->close ();
      return ERROR;
    }
    if (sp -> RPId) {
      // Process busy
      sendError (DERR_UNV, "Process busy");
      OutputBuffer->close ();
      return ERROR;
    }
    sp -> RPId = NPId;  // Interlock
    // OK, we can try to handle the request
    CurrentStatus = sp;
    StatusCount = 1;
  } else {
    // This is a global request
    setWrite (Sock, SkBuffer);       // We are going to send something
    OutputBuffer->reset (PH_MBL);    // Start monitor block
    for (sp = Smurphs; sp; sp = sp -> next)
      if (sp->RPId == 0) sp->RPId = NPId;
    CurrentStatus = Smurphs;
    StatusCount = MAX_int;
  }
  return OK;
};

int ConnectThread::run () {
  int rv;
  if (timeout (STIMEOUT)) {
    cdebugl ("ConnectThread timeout");
    if (State == ST_FLUSHINQUIRY || State == ST_GETREPLY) {
      State = ST_INQTIMEOUT;
    } else {
      // Disconnect and terminate the thread
Disconnect:
      closeSocket (Sock);
      return OK;
    }
  }
  // Continue thread
Redo:
  switch (State) {

    case ST_READSIGNATURE:
      // We start from here

      // cdebug ("ConnectThread ST_SIGNATURE: ");
      switch (signature ()) {
        case NONE:
      //  cdebugl ("NONE");
          return NONE;  // Continue thread
        case ERROR:
          cdebugl ("ERROR");
          goto Disconnect;
        case DSD_SIGNATURE:

          cdebugl ("DSD");
          // Determine the request type
          switch (getOctet ()) {
            case PH_BPR:
              createInfo ();
              resetTimer ();
              State = ST_FLUSHANDDIE;
              goto Redo;
            case PH_STA:
              InqBuffer = new SockBuff (SHTBUFSIZE);
              rv = createStatusInquiry ();
              resetTimer ();
              State = (rv == ERROR) ? ST_FLUSHANDDIE : ST_MAKEINQUIRY;
              goto Redo;
            case PH_DRQ:
              if (setupDisplayThreads () == ERROR) {
                // There is an error essage
                State = ST_FLUSHANDDIE;
                goto Redo;
              }
              return OK;
            default:
              goto Disconnect;
          }

          // Should never get here
        case SMURPH_SIGNATURE:
          cdebugl ("SMURPH");
          // This we can handle immediately
          if (newSmurph () == ERROR) goto Disconnect;
          // We close the buffer, but the socket is left for the record
          return OK;
        default:
          goto Disconnect;  // Just in case -- cannot get here
      }

    case ST_FLUSHANDDIE:
      // Send back status info

      if (OutputBuffer->write (OutputSocket) == NONE) return NONE;
      goto Disconnect;

    case ST_MAKEINQUIRY:
      // Processing one status inquiry: SkBuffer accumulates responses,
      // while InqBuffer is used for polling

      while (StatusCount) {
        if (CurrentStatus == NULL) {
          StatusCount = 0;
        } else if (CurrentStatus -> RPId != NPId) {
          CurrentStatus = CurrentStatus->next;
          StatusCount --;
        } else 
          break;
      }

      if (StatusCount) {
        // Issue a single inquiry request
        resetTimer ();
        // This goes back to the inquirer
        setWrite (Sock, SkBuffer);
        sendOctet (PH_STA);
        sendInt (CurrentStatus->Handle);    // This will accumulate
        // This goes to the inquired process
        setWrite (CurrentStatus->socket, InqBuffer);
        OutputBuffer->reset (PH_STA);
        OutputBuffer->close ();
        State = ST_FLUSHINQUIRY;
        goto Redo;
      }

      // We get here when we are done with all inquiries

      setWrite (Sock, SkBuffer);
      resetTimer ();
      OutputBuffer->close ();
      State = ST_FLUSHANDDIE;
      goto Redo;

    case ST_FLUSHINQUIRY:
      // Send a single inquiry request

      if ((rv = OutputBuffer->write (OutputSocket)) == NONE) return NONE;
      if (rv == ERROR) {
        // Ignore this inquiry
SkipInq:
        CurrentStatus = CurrentStatus->next;
        StatusCount--;
        State = ST_MAKEINQUIRY;
        goto Redo;
      }

      // The inquiry was sent successfully

      setRead (CurrentStatus->socket, InqBuffer);
      setWrite (Sock, SkBuffer);
      InqBuffer->clear ();
      State = ST_GETREPLY;
      goto Redo;

    case ST_GETREPLY:
      // Receive a reply to the inquiry

      if ((rv = InputBuffer->read (InputSocket)) == NONE) return NONE;
      if (rv == ERROR) goto SkipInq;
      if (InputBuffer->header () != PH_STA) {
        sendError (DERR_ILS, "Illegal response");
        goto SkipInq;
      }
      while (more ()) sendOctet (getOctet ());
      goto SkipInq;

    case ST_INQTIMEOUT:
      // Inquiry timeout

      goto SkipInq;

    default:
      excptn ("Illegal state in ConnectThread");
  }
};

int ConnectThread::signature () {
  // Read signature from the connecting party
  int nc, nr, rl, ret;
  // We do it more carefully than other reads -- to avoid allocating
  // a 16 MB buffer for garbage arriving from a random client
  while (InputBuffer->fill < 4) {
    // We have no clue how much to expect
    nc = 4 - InputBuffer->fill;
    if ((nr = readSocket (InputSocket, InputBuffer->Buffer +
                                            InputBuffer->fill, nc)) <= 0) {
      if (nr == 0 || errno != EAGAIN) return ERROR;
      return NONE;  // Continue later
    }
    InputBuffer->fill += nr;
  }
  // We have acquired the header -- do some sanity checks
  if (InputBuffer->Buffer [0] != PH_SIG) return ERROR;
  if (InputBuffer->tlength () < 4 + 1 + SIGNLENGTH + 1) return ERROR;
  // Read the rest. Note that this code, including the two checks
  // above, may be re-entered a number of times, but we don't care.
  while (InputBuffer->fill < 4 + 1 + SIGNLENGTH) {
    nr = readSocket (InputSocket, InputBuffer->Buffer +
                 InputBuffer->fill, 4 + 1 + SIGNLENGTH - InputBuffer->fill);
    if (nr <= 0) {
      if (nr == 0 || errno != EAGAIN) return ERROR;
      return NONE;
    }
    InputBuffer->fill += nr;
  }
  // We have read the signature. Now we can recognize it.
  if (strncmp (SMRPHSIGN, (char*)(InputBuffer->Buffer+4+1), SIGNLENGTH)
                                                                       == 0)
    ret = SMURPH_SIGNATURE;
  else if (strncmp (SERVRSIGN, (char*)(InputBuffer->Buffer+4+1),
                                                           SIGNLENGTH) == 0)
    ret = DSD_SIGNATURE;
  else
    return ERROR;
  // Now we can safely read the rest
  if ((nc = InputBuffer->tlength ()) > InputBuffer->size)
    InputBuffer->grow (nc);
  while (InputBuffer->fill < nc) {
    nr = readSocket (InputSocket, InputBuffer->Buffer +
                                 InputBuffer->fill, nc - InputBuffer->fill);
    if (nr <= 0) {
      if (nr == 0 || errno != EAGAIN) return ERROR;
      return NONE;
    }
    InputBuffer->fill += nr;
  }
  // Set the pointer for interpreting buffer contents
  InputBuffer->ptr = 4 + SIGNLENGTH + 2;
  return ret;
};
          
void processNew () {
  // Check if there is a new incoming connection
  int sk;
  while ((sk = getSocketConnection (Master)) >= 0) {
    setSocketUnblocking (sk);
    new ConnectThread (sk);
  }
};

void processThreads () {
  int tc;
  Thread *th, *thh;
  for (tc = 0, th = ThreadList; th != NULL; th = thh) {
    thh = th->Next;
    if (th->State == 0 || th->run () == OK)
      delete th;
    else
      tc++;
  }
  CurrentDelay = (tc > 1) ? POLLDELAY_RUN : POLLDELAY_DRM;
};

int GarbageThread::run () {
  // Check if some of the smurphs haven't disappeared
  SmDesc *sp, *spp;
  for (sp = Smurphs; sp != NULL; sp = spp) {
    spp = sp->next;
    if (sp->RPId) continue;  // Ignore locked smurphs
    setRead (sp->socket, SkBuffer);
    SkBuffer->clear ();
    if (SkBuffer->read (InputSocket) == ERROR) {
      // The smurph is no longer there
      cdebugl (form ("SMURPH deleted: %1d", sp->Handle));
      closeSocket (sp->socket);
      delete sp;
    }
    // Do nothing otherwise. If the read succeeds, we are getting rid of
    // some late responses to DSD.
  }
  // We are too pretty to die
  return NONE;
};

void sigserv () {
  // Close all sockets. I am not sure if this is going to help, but apparently,
  // at least some sockets remain open when a DOS program terminates abnormally
  closeAllSockets ();
  excptn ("Signal received");
  exit (0);
};

void signalService () {
  // Intercept whatever you can
#ifdef	SIGHUP
	signal (SIGHUP , (SIGARG) sigserv);
#endif
#ifdef	SIGILL
	signal (SIGILL , (SIGARG) sigserv);
#endif
#ifdef	SIGIOT
	signal (SIGIOT , (SIGARG) sigserv);
#endif
#ifdef  SIGEMT
	signal (SIGEMT , (SIGARG) sigserv);
#endif
#ifdef  SIGSYS
	signal (SIGSYS , (SIGARG) sigserv);
#endif
#ifdef	SIGFPE
	signal (SIGFPE , (SIGARG) sigserv);
#endif
#ifdef  SIGBUS
	signal (SIGBUS , (SIGARG) sigserv);
#endif
#ifdef	SIGSEGV
	signal (SIGSEGV, (SIGARG) sigserv);
#endif
#ifdef	SIGKILL
	signal (SIGKILL, (SIGARG) sigserv);
#endif
};

main (int argc, char *argv []) {

  // There is no use for stdin and stdout. We are going to operate as a
  // a daemon.

  const char *st;

  signalService ();

  // close (0);
  // close (1);

  if (argc > 2) {
    st = "Usage: monitor [templatefile]\n"; 
    write (2, st, strlen (st));
    exit (1);
  }

  if (argc > 1) {
    zz_TemplateFile = argv [1];
    cdebug ("Reading templates ... ");
    readTemplates ();
    cdebugl ("done\n");
  } else
    Templates = NULL;

  ThreadList = NULL;

  Master = openServerSocket (ZZ_MONSOCK);

  if (Master < 0) {
    excptn (
"Monitor startup error: port probably taken, rebuild with another socket port\n"
           );
  }
  cdebugl ("Master socket opened\n");

  NextHandle = 0;         // Start numbering smurphs
  // Create the LSk buffer

  setSocketUnblocking (Master);

  cdebugl ("Unblocked\n");

  // We start dormant
  CurrentDelay = POLLDELAY_DRM;

  new GarbageThread ();

  cdebugl ("Entering main loop\n");

  while (Master >= 0) {
    processNew ();
    processThreads ();
    usleep (CurrentDelay);
  }
  closeAllSockets ();
};
