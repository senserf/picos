/* oooooooooooooooooooooooooooooooooo */
/* Copyright (C) 2003   P. Gburzynski */
/* oooooooooooooooooooooooooooooooooo */

/* --- */

// #define	CDEBUG 1

/* ---------------------------------------------------------- */
/* This  program monitors the execution of smurphs on a local */
/* network of computers  running  BSD  UNIX.  Each  simulator */
/* instance  is  supposed  to  report  to  the monitor, which */
/* operation  establishes  a  stream  socket  connecting  the */
/* simulator  to  the  monitor.  This socket is alive for the */
/* entire lifespan of the simulator instance and it  is  used */
/* by  the  monitor to identify the simulator instance within */
/* the domain of the local network.                           */
/* ---------------------------------------------------------- */

#include        "../version.h"
#include        "../LIB/lib.h"
#include	"../LIB/socklib.h"

#include        "monitor.h"

#include        "../KERNEL/cdebug.h"
#include        "../KERNEL/display.h"
#include	"../KERNEL/sockbuf.cc"


long			MemoryUsed;     // Ignored -- for local library

int                     Master,         // The master socket
			LSk,            // Last "local" socket
			NDesc;          // Maximum number of descriptors

jmp_buf                 restart,        // For timeouts
                        rechild;        // For detecting terminating children

int       InputSocket,  OutputSocket, NextHandle = 0;
SockBuff *InputBuffer, *OutputBuffer, *LSkBuffer;

void setRead (int s, SockBuff *sb) {
  InputSocket = s;
  InputBuffer = sb;
};

void setWrite (int s, SockBuff *sb) {
  OutputSocket = s;
  OutputBuffer = sb;
};

int SockBuff::read (int sock, int block) {
//
// Read the buffer
//
// Note that the second argument is unused. We leave it for compatibility
// with SockBuff used by DSD (we recycle here exactly the same SockBuff
// structure).
//
  int nr, nc;
  // Read the minimum you can safely expect
  ptr = 0;
  for (fill = 0; fill < 4; ) {
    nr = readSocket (sock, Buffer + fill, 4 - fill);
    if (nr <= 0) return ERROR;
    fill += nr;
  }
  if (Buffer [0] < PH_MIN) return ERROR;
  // Now we can tell the length
  if ((nc = tlength ()) > size) grow (nc);
  while (fill < nc) {
    nr = readSocket (sock, Buffer + fill, nc - fill);
    if (nr <= 0) return ERROR;
    fill += nr;
  }
  return OK;
};

int SockBuff::write (int sock, int block) {
//
// Write the buffer. Second argument unused.
//
  int nc, nr;
  if (fill == 0) return OK;
  for (nc = 0; nc < fill; ) {
    if ((nr = writeSocket (sock, Buffer + nc, fill - nc)) <= 0)
                                                                  return ERROR;
    nc += nr;
  }
  fill = 0;
  return OK;
};

int readSignatureBlock () {
//
// Read the signature block. We do it more carefully, so that we can do
// some sanity check -- to avoid allocating a 16MB buffer for possible
// garbage arriving from a spurious client.
//
  int nr, nc, ret;
  // Read the minimum you can safely expect
  cdebugl ("Reading signature block");
  for (nc = 0; nc < 4; ) {
    nr = readSocket (InputSocket, InputBuffer->Buffer + nc, 4 - nc);
    if (nr <= 0) return ERROR;
    nc += nr;
  }
  InputBuffer->fill = nc;
  cdebugl ("First four bytes:");
  dumpbuf (InputBuffer);
  if (InputBuffer->Buffer [0] != PH_SIG) return ERROR;
  if (InputBuffer->tlength () < 4 + 1 + SIGNLENGTH + 1) return ERROR;
  while (nc <  4 + 1 + SIGNLENGTH) {
    nr = readSocket (InputSocket, InputBuffer->Buffer + nc,
                                                      4 + 1 + SIGNLENGTH - nc);
    if (nr <= 0) return ERROR;
    nc += nr;
  }
  InputBuffer->fill = nc;
  cdebugl ("Signature:");
  dumpbuf (InputBuffer);
  if (strncmp (SMRPHSIGN, (char*)(InputBuffer->Buffer+4+1), SIGNLENGTH) == 0)
    ret = SMURPH_SIGNATURE;
  else
  if (strncmp (SERVRSIGN, (char*)(InputBuffer->Buffer+4+1), SIGNLENGTH) == 0)
    ret = DSD_SIGNATURE;
  else
    return ERROR;
  // Now we can safely read the rest
  InputBuffer->fill = InputBuffer->tlength ();
  while (nc < InputBuffer->fill) {
    nr = readSocket (InputSocket, InputBuffer->Buffer + nc,
                                                       InputBuffer->fill - nc);
    if (nr <= 0) return ERROR;
    nc += nr;
  }
  dumpbuf (InputBuffer);
  // This is the first byte behind the signature
  InputBuffer->ptr = 4 + SIGNLENGTH + 2;
  return ret;
};

/* ------------------------------------------------------------ */
#define sendOctet(val)   OutputBuffer->out(val)
#define sendChar(val)    OutputBuffer->out((val)&0x7f)
#define peekOctet()      InputBuffer->peek()
#define getOctet()       InputBuffer->inp()
#define more()           InputBuffer->more()
/* ------------------------------------------------------------ */

void sendText (const char*);
void clearTimer ();
void setTimer (int delay = GTIMEOUT);

void sendError (int err, const char *txt) {
  sendOctet (PH_ERR);
  sendChar (err);
  sendText (txt);
};

void flushAndClose () {
  if (setjmp (restart)) {
    // After timeout
Done:
    clearTimer ();
    closeSocket (OutputSocket);
    return;
  }
  setTimer ();
  OutputBuffer->close ();
  cdebugl ("Sending");
  dumpbuf (OutputBuffer);
  OutputBuffer->write (OutputSocket);
  goto Done;
};

int getText (char *t, int len) {
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

int     dscrhalf, dscrlast;

inline  static  int     getDescriptor () {
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

void    sendText (const char *t) {
/* ------------------------------ */
/* Send text string to the server */
/* ------------------------------ */
  sendOctet (PH_TXT);
  while (*t != '\0') sendChar ((int)(*t++));
  sendOctet (0);
};

LONG    getInt () {
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

static  int     lastsdscr, lastshalf;

inline  static  void    sendDescriptor (int d) {
/* ----------------------------- */
/* Send one numerical descriptor */
/* ----------------------------- */
  if (lastshalf) {
    sendOctet (lastsdscr | (d & 017));
    lastshalf = NO;
  } else {
    lastshalf = YES;
    lastsdscr = (d & 017) << 4;
  }
};

inline  static  void    flushDescriptor () {
/* ------------------------------------------------------ */
/* Flush a partially filled descriptor byte to the server */
/* ------------------------------------------------------ */
  if (lastshalf) {
    sendOctet (lastsdscr);
    lastshalf = NO;
  }
};

static  void    sendInt (LONG val) {
/* ------------------------------------ */
/* Send an integer number to the server */
/* ------------------------------------ */
  int     digits [16], ndigits, sig;
  sendOctet (PH_INM);
  lastshalf = NO;
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

#include "../KERNEL/dshared.cc"

void timeOut ();

void    setTimer (int delay) {
/* ------------------- */
/* Set the alarm clock */
/* ------------------- */
  struct  itimerval       td;
  signal (SIGALRM, (SIGARG) timeOut);
  td.it_value.tv_sec = GTIMEOUT;
  td.it_value.tv_usec = 0;
  td.it_interval.tv_sec = 0;
  td.it_interval.tv_usec = 0;
  setitimer (ITIMER_REAL, &td, NULL);
};

void    clearTimer () {
/* ---------------------- */
/* Clears the alarm clock */
/* ---------------------- */
	struct  itimerval       td;
	td.it_value.tv_sec = 0;
	td.it_value.tv_usec = 0;
	td.it_interval.tv_sec = 0;
	td.it_interval.tv_usec = 0;
	setitimer (ITIMER_REAL, &td, NULL);
        signal (SIGALRM, SIG_DFL);
};

void    timeOut () {
/* --------------- */
/* Timeout service */
/* --------------- */
  longjmp (restart, 0);
};

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

void    processSmurph () {
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

  // Read the host name
  if (getText (HName, 127) == ERROR) longjmp (restart, 0);
  // Read the program call line
  if (getText (PArgs, 127) == ERROR) longjmp (restart, 0);
  // This can be occasionally long. Make sure to include beginning and end.
  shortenText (PArgs, SML_PARG);
  // Read the startup date/time
  if (getText (DTime,  31) == ERROR) longjmp (restart, 0);
  // Read the process ID
  if ((PId = getInt ()) == 0) longjmp (restart, 0);

  sm = new SmDesc;
  strcpy (sm->HName, HName);
  strcpy (sm->PArgs, PArgs);
  strcpy (sm->DTime, DTime);
  sm->PId = PId;
  sm->socket = LSk;
  sm->Handle = NextHandle++;
  // Make sure handles are always nonnegative (I'm not sure if wee need it)
  if (NextHandle < 0) NextHandle = 0;
  LSk = -1;
};

void doDSDSmurph (SmDesc *sp) {
/* -------------------------------- */
/* Sustain DSD-Smurph communication */
/* -------------------------------- */
  int Phrase, wc, nc;
  setRead (LSk, LSkBuffer);
  setWrite (sp->socket, LSkBuffer);
  if (setjmp (restart)) {
ST: // We get here after Smurph write timeout
    cdebugl ("DSD->Smurph: write timeout");
    clearTimer ();
    exit (0);            // The big brother will take care of this
  }
  while (1) {
    // We don't timeout here, because we don't know for how long DSD or
    // Smurph wants to be idle
    if (InputBuffer->read (InputSocket) == ERROR) {
      cdebugl ("DSD->Smurph: unexpected disconnection");
      // This looks like unexpected disconnection on DSD's part
      OutputBuffer->reset (PH_END);
      flushAndClose ();  // Don't wait too long this time
      // Give the other end a chance to clear its backlog
      sleep (STIMEOUT);
      exit (0);          // Our bigger brother will see us die
    }
    // Read OK. Send this stuff directly to Smurph. Note that we are using
    // the same buffer for input and output
    cdebugl ("DSD->Smurph: relaying");  dumpbuf (OutputBuffer);
    setTimer ();
    if (OutputBuffer->write (OutputSocket) < 0) goto ST;
    clearTimer ();
  }
};

TmpList *TempHead;

void queueTemplates (const char *sn, const char *bn, const char *nn) {
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

int parseSmurphBlock () {
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

void sendTemplates () {
  TmpList *tl;
  OutputBuffer->reset (PH_BLK);
  while (TempHead) {
    TempHead->tmplt->send ();
    TempHead = (tl = TempHead) -> next;
    delete tl;
  }
  OutputBuffer->close ();
};

void otherTerm () {
  // After termination of the other party, i.e., doDSDSmurph
  int Status;
  clearTimer ();
  while (wait3 (&Status, WNOHANG, NULL) > 0);
  longjmp (rechild, 0);
};

void doSmurphDSD (SmDesc *sp, int otherPID) {
/* -------------------------------- */
/* Sustain Smurph-DSD communication */
/* -------------------------------- */
  int ending;
  setRead (sp->socket, LSkBuffer);
  setWrite (LSk, LSkBuffer);
  if (setjmp (rechild)) {
    // After termination of the other party (Smurph has died)
    cdebugl ("Smurph->DSD: other process dies"); 
    closeSocket (OutputSocket);   
    exit (0);
  }
  signal (SIGCHLD, (SIGARG) otherTerm);
  while (1) {
    // One block has been read already when we are called
    cdebugl ("Smurph->DSD:"); dumpbuf (OutputBuffer);
    ending = parseSmurphBlock ();
    OutputBuffer->write (OutputSocket);
    // Notably, we don't care about errors here. If it happens that
    // DSD is dead, we hope the other party will detect this.
    // Now we will just ignore this and assume that everything is fine.
    // We want to get rid of all the backlog that may have
    // accumulated on Smurph's end.
    if (ending) {
      // Nothing more will arrive from either party
End:
      cdebugl (form ("Smurph->DSD died %1d", ending));
      signal (SIGCHLD, SIG_DFL);
      kill (otherPID, SIGKILL);
      closeSocket (OutputSocket);
      sleep (STIMEOUT);
      closeSocket (OutputSocket);   // Doesn't hurt to do it twice
      exit (0);
    }
    if (TempHead) {
      sendTemplates ();
      cdebugl ("Smurph->DSD (templates):"); dumpbuf (OutputBuffer);
      OutputBuffer->write (OutputSocket);
    }
    cdebugl ("Before read");
    if (InputBuffer->read (InputSocket) == ERROR) {
      // Smurph has died
      OutputBuffer->reset (PH_BLK);
      sendOctet (PH_END);
      OutputBuffer->close ();
      OutputBuffer->write (OutputSocket);
      sleep (STIMEOUT);
      goto End;
    }
    cdebugl ("After read");
  }
};
      
void    processDisplay () {
/* ----------------------- */
/* Process display request */
/* ----------------------- */
  Long Handle;
  SmDesc *sp;
  int NPId;
  // Read the handle
  Handle = getInt ();
  // Locate the handle
  for (sp = Smurphs; sp; sp = sp -> next) if (sp->Handle == Handle) break;
  // Create a separate process to handle the request
  if (NPId = fork ()) {
    cdebugl (form ("processDisplay spawning Smurph->DSD %1d", NPId));
    // We are the parent
    if (NPId < 0) longjmp (restart, 0);        // Fork has failed
    if (sp && sp->RPId == 0) sp->RPId = NPId;  // Make it unusable
    return;
  }
  // We are the big brother responsible for Smurph to DSD traffic
  delete LSkBuffer;
  LSkBuffer = new SockBuff (LRGBUFSIZE);
  setWrite (LSk, LSkBuffer);       // We are going to send something
  if (sp == NULL) {
    // No such process
    OutputBuffer->reset (PH_MBL);  // Start monitor block
    sendError (DERR_ONF, "No such process");
    flushAndClose ();
    exit (0);
  }
  if (sp -> RPId) {
    // Process busy
    OutputBuffer->reset (PH_MBL);  // Start monitor block
    sendError (DERR_UNV, "Process busy");
    flushAndClose ();
    exit (0);
  }
  setWrite (sp->socket, LSkBuffer);
  OutputBuffer->reset (PH_DRQ);
  OutputBuffer->close ();
  cdebugl ("Sending DRQ to Smurph");
  if (setjmp (restart)) {
    // After timeout
    cdebugl ("DRQ send timeout");
    clearTimer ();
    setWrite (LSk, LSkBuffer);
    OutputBuffer->reset (PH_MBL);  // Start monitor block
    sendError (DERR_NRP, "Reply timeout");
    flushAndClose ();
    exit (0);
  }
  setTimer ();
  if (OutputBuffer->write (OutputSocket) == ERROR) {
Disc:
    cdebugl ("DRQ reply disconnect");
    clearTimer ();
    setWrite (LSk, LSkBuffer);
    OutputBuffer->reset (PH_MBL);  // Start monitor block
    sendError (DERR_DSC, "Disconnected");
    flushAndClose ();
    exit (0);
  }
  setRead (sp->socket, LSkBuffer);
  if (InputBuffer->read (InputSocket) == ERROR) goto Disc;
  clearTimer ();
  cdebugl (form ("Reply received: %x", InputBuffer->header ()));
  if (InputBuffer->header () != PH_BLK) {
    cdebugl ("Illegal response");
    setWrite (LSk, LSkBuffer);
    OutputBuffer->reset (PH_MBL);  // Start monitor block
    sendError (DERR_ILS, "Illegal response");
    flushAndClose ();
    exit (0);
  }
  // BLK reply received from SMURPH -- setup two-way communication
  if (NPId = fork ()) {
    cdebugl (form ("Smurph->DSD: spawning DSD->Smurph %1d", NPId));
    // We are the parent
    if (NPId < 0) {
      // Fork has failed
      cdebugl ("Cannot fork");
      setWrite (LSk, LSkBuffer);
      OutputBuffer->reset (PH_MBL);  // Start monitor block
      sendError (DERR_UNV, "Monitor cannot fork");
      flushAndClose ();
      exit (0);
    }
    // Fork successful
    doSmurphDSD (sp, NPId);
    exit (0);
  }
  doDSDSmurph (sp);
  exit (0);
};

void	processInfo () {
/* -------------------- */
/* Process Info request */
/* -------------------- */
  Long Handle;
  SmDesc *sp;
  int NPId;
  // Spawn a child immediately
  if (NPId = fork ()) {
    // We are the parent
    if (NPId < 0) longjmp (restart, 0);        // Fork has failed
    return;
  }
  // We are the child
  cdebugl ("processInfo child");
  setWrite (LSk, LSkBuffer);     // We are going to send something
  OutputBuffer->reset (PH_MBL);  // Start monitor block
  for (sp = Smurphs; sp; sp = sp->next) {
    sendOctet (PH_MEN);
    sendInt (sp->Handle);
    sendInt (sp->PId);
    sendText (sp->HName);
    sendText (sp->PArgs);
    sendText (sp->DTime);
  }
  flushAndClose ();
  // Don't worry about errors
  exit (0);  // Child exit
};

SockBuff *InqBuffer;
    
void    processSingleStatus (SmDesc *sp) {
/* -------------------------- */
/* Process one status request */
/* -------------------------- */

  // The status information contains:
  // 
  //      The number of received messages
  //      The simulated time
  //      The CPU execution time

  cdebugl ("Status request");
  // Send the reply header
  sendOctet (PH_STA);
  sendInt (sp->Handle);
  setWrite (sp->socket, InqBuffer);
  // Issue a status request to the smurph
  OutputBuffer->reset (PH_STA);
  OutputBuffer->close ();
  // Setup timer -- this may time out
  if (setjmp (restart)) {
Disc:
    clearTimer ();
    setWrite (LSk, LSkBuffer);
    sendError (DERR_DSC, "Disconnected");
    return;
  }
  if (OutputBuffer->write (OutputSocket) == ERROR) goto Disc;
  setRead (sp->socket, InqBuffer);
  setWrite (LSk, LSkBuffer);
  if (InputBuffer->read (InputSocket) == ERROR) goto Disc;
  clearTimer ();
  if (InputBuffer->header () != PH_STA) {
    sendError (DERR_ILS, "Illegal response");
    return;
  }
  // Pass the rest of the spurt to DSD
  while (more ()) sendOctet (getOctet ());
};
    
void processStatus () {
  Long Handle;
  SmDesc *sp;
  int NPId;
  if (more ()) {
    // The request byte is followed by a handle, thus this is a single
    // status request
    Handle = getInt ();
    // Locate the handle
    for (sp = Smurphs; sp; sp = sp -> next) if (sp->Handle == Handle) break;
    // Create a separate process to handle the request
    if (NPId = fork ()) {
      // We are the parent
      if (NPId < 0) longjmp (restart, 0);        // Fork has failed
      if (sp && sp->RPId == 0) sp->RPId = NPId;  // Make it unusable
      return;
    }
    // We are the child
    setWrite (LSk, LSkBuffer);       // We are going to send something
    OutputBuffer->reset (PH_MBL);    // Start monitor block
    if (sp == NULL) {
      // No such process
      sendError (DERR_ONF, "No such process");
      flushAndClose ();
      exit (0);
    }
    if (sp -> RPId) {
      // Process busy
      sendError (DERR_UNV, "Process busy");
      flushAndClose ();
      exit (0);
    }
    // OK, we can try to handle the request
    InqBuffer = new SockBuff (SHTBUFSIZE);
    processSingleStatus (sp);
  } else {
    // This is a global request
    if (NPId = fork ()) {
      // We are the parent
      if (NPId < 0) longjmp (restart, 0);
      for (sp = Smurphs; sp; sp = sp -> next)
        // Mark them all as unusable
        if (sp->RPId == 0) sp->RPId = NPId;
      return;
    }
    // We are the child
    InqBuffer = new SockBuff (SHTBUFSIZE);
    for (sp = Smurphs; sp; sp = sp -> next)
      if (sp->RPId == 0)
        processSingleStatus (sp);
  }
  flushAndClose ();
  exit (0);
  // Child exit
};
	
void processServer () {
/* ------------------------------------- */
/* Process a request from display server */
/* ------------------------------------- */
  int Request;
  Request = getOctet ();
  // This byte tells us everything
  switch (Request) {
    case PH_BPR:
      cdebugl ("BPR");
      processInfo ();           // Simple info request
      break;
    case PH_STA:                // Status request
      cdebugl ("STA");
      processStatus ();
      break;
    case PH_DRQ:
      cdebugl ("DRQ");
      processDisplay ();
      break;
    default:
      longjmp (restart, 0);
  }
  closeSocket (LSk);
};

int zombies () {
  // Child termination
  SmDesc *sp;
  int PId, Status, zc;
  zc = 0;
  while ((PId = wait3 (&Status, WNOHANG, NULL)) > 0) {
    cdebugl (form ("Child terminated %1d", PId));
    // Unblock the simulators that have terminated their display sessions
    for (sp = Smurphs; sp != NULL; sp = sp->next) {
      if (sp->RPId == PId) {
        zc++;
        sp->RPId = 0;
      }
    }
  }
  return zc;
};

void childTerm () {
  // Child termination
  zombies ();
  longjmp (restart, 0);
};

main    (int argc, char *argv []) {
  const char  *st;
  char  pname [SIGNLENGTH+2];
  struct timeval seltim;
  // There is no use for stdin and stdout. We are going to operate as a
  // a daemon
  close (0);
  close (1);
  if (argc > 2) {
    st = "Usage: monitor [templatefile]\n"; 
    write (2, st, strlen (st));
    exit (1);
  }
  if (argc > 1) {
    zz_TemplateFile = argv [1];
    readTemplates ();
  } else
    Templates = NULL;

  Master = openServerSocket (ZZ_MONSOCK);

  if (Master < 0) {
    excptn (
"Monitor startup error: port probably taken, rebuild with another socket port\n"
    );
  }

  LSk = -1;
  NextHandle = 0;         // Start numbering smurphs
  // Create the LSk buffer
  LSkBuffer = new SockBuff (LSKBUFSIZE);

  NDesc = getdtablesize ();
  cdebugl (form ("NDesc: %1d", NDesc));

  setjmp (restart);       // For timeouts
  clearTimer ();

  closeSocket (LSk);            // Close last "local" socket

  while (Master >= 0) {
    fd_set  rr;     // Ready for reading
    SmDesc  *sp, *spp;
    int     dummy;
    char    cc;
    // Wait for a new connection or for a disappearance of a smurph
    FD_ZERO (&rr);
    FD_SET (Master, &rr);
    for (sp = Smurphs; sp != NULL; sp = sp->next)
      // Add those sockets that are not involved in display sessions
      if (sp->RPId == 0) FD_SET (sp->socket, &rr);
    cdebugl ("Before select");
    signal (SIGCHLD, (SIGARG) childTerm);
    seltim.tv_sec = 30;
    seltim.tv_usec = 0;
    if ((dummy = select (NDesc, &rr, NULL, NULL, &seltim)) < 0) {
      // Something wrong (shouldn't really happen)
      cdebugl (form ("Select failure %1d %1d", dummy, errno));
      sleep (2);
      continue;
    }
    if (zombies () || dummy == 0) continue;
    signal (SIGCHLD, SIG_DFL);
    cdebugl ("Select returns");
    // Check if a closing request
    for (sp = Smurphs; sp != NULL; sp = spp) {
      spp = sp->next;
      if (FD_ISSET (sp->socket, &rr)) {
        // We issue a tentative read request. This should return zero length
        // indicating that the simulator has closed its end of the socket.
        // Normally, Smurph doesn't initiate communication, so this is the
        // only possibility, unless things have been messed up badly.
        setTimer (STIMEOUT);
        if (LSkBuffer->read (sp->socket) == ERROR) {
          // This is what we expect
          clearTimer ();
          closeSocket (sp->socket);
          cdebugl (form ("Closing %1d", sp->socket));
          delete (sp);
        } else {
          cdebugl (form ("Block ignored %1d", LSkBuffer->Buffer [0]));
          // Otherwise we simply ignore the block. This will take care of the
          // situation when it is some kind of a late response to DSD.
        }
      }
    }
    setTimer ();
    if (FD_ISSET (Master, &rr)) {
      // Incoming connection
      if ((LSk = getSocketConnection (Master)) < 0)
        // Ignore if fails
        longjmp (restart, 0);
      setRead (LSk, LSkBuffer);
      // We do this more carefully than the rest, e.g., to avoid allocating
      // a large buffer just for reading some garbage arriving from an 
      // illegitimate peer. 
      if ((dummy = readSignatureBlock ()) == ERROR) longjmp (restart, 0);
      clearTimer ();
      if (dummy == SMURPH_SIGNATURE) {
        // A new smurph
        cdebugl (form ("New smurph %1d", LSk));
        processSmurph ();
      } else if (dummy == DSD_SIGNATURE) {
        // A dsd request
        cdebugl (form ("DSD %1d", LSk));
        processServer ();
      } else
        longjmp (restart, 0);
    }
    clearTimer ();
  }
};
