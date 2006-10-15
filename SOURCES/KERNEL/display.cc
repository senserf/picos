/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-06   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

/* --- */

//#define  CDEBUG  1

#include        "system.h"
#include	"../LIB/socklib.h"

#ifdef	ZZ_MONHOST
// Otherwise we don't want the display part

/* -------------------------------------------------- */
/* Functions connecting smurph to the display program */
/* -------------------------------------------------- */

#include        "display.h"

#include	"cdebug.h"

#define itemcheck   do { --eitemcount; if (sitemcount-- > 0) return; \
                         if (eitemcount < 0) return; } while (0)

// This is passive (for regions)
#define itemtest    do { if (sitemcount >= 0 || eitemcount < 0) return; } \
                       while (0);

#include	"sockbuf.cc"

static SockBuff  *MIB, *MOB;

static  int             monsock = -1,      // Monitor socket
                        ctimeout,
  // We use a simple credit-based scheme to determine how many updates we can
  // send to the monitor before we should get something in response.
                        Credits,
                        RefreshDelay = 1;  // Delay after refresh failure

static void monitorRequest ();
static void queueTemplates (const char*, const char*, const char*);

static jmp_buf         tmoutenv;     // Timeout exception environment for
		      	             // setjmp
static  int      initializing,  // Flag = initial handshake
		 withinwindow,  // True if within expose
                 withinregion,
                 terminating;   // END phrase sent

static  int     sitemcount,     // Limit for the number of window items
                eitemcount;

/* ------------------------------------------------------------ */
#define sendOctet(val)   MOB->out(val)
#define sendChar(val)    MOB->out((val)&0x7f)
#define peekOctet()      MIB->peek()
#define getOctet()       MIB->inp()
#define more()           MIB->more()
/* ------------------------------------------------------------ */

static void sendText (const char*);
static void getResponse (int);
static void tabt (const char *msg);
static void processEWindowPhrase ();
static void processEStepMode ();
static void serviceEND ();

ZZ_WINDOW::ZZ_WINDOW (ZZ_Object *o, int md, int stat, int scnt, int ecnt) {
/* --------------------------------- */
/* Constructor for window list items */
/* --------------------------------- */
	obj = o;
	mode = md;
	flag = WF_OPENING;              // Newly-created window
	stid = stat;
	scount = scnt;
	ecount = ecnt;
	frame = NULL;                   // No associated object
}

class ORequest {
// A pending outgoing message
  public:
    int type;
    IPointer par1;  // This is occasionally used as a pointer
    Long par2;
    char *contents;
    ORequest *next;
    ORequest (int, const char *t = NULL, IPointer p1 = NONE, Long p2 = NONE);
    ~ORequest () { if (contents) delete contents; };
};

static ORequest  *ORQ = NULL;

ORequest::ORequest (int tp, const char *cn, IPointer p1, Long p2) {
  ORequest *orq;
  if (cn) {
    contents = new char [strlen (cn)+1];
    strcpy (contents, cn);
  } else {
    contents = NULL;
  }
  type = tp;
  par1 = p1;
  par2 = p2;
  next = NULL;
  if (ORQ == NULL) {
    ORQ = this;
  } else {
    for (orq = ORQ; orq->next != NULL; orq = orq->next);
    orq->next = this;
  }
};

int SockBuff::read (int sock, int block) {
// Read the buffer -- must be clear'ed first. Applicable only to buffers
// used for input
  int nc, nr, rl;
  if (fill >= 4 && fill == tlength ())
    // The buffer hasn't been cleared or it has been filled -- leave it alone
    return OK;
  if (block) {
    setSocketBlocking (sock);
    while (fill < 4) {
      // We still haven't got the header -- so we read just the header
      nc = 4 - fill;
      if ((nr = readSocket (sock, RWCAST (Buffer + fill), nc)) <= 0) {
        cdebugl (form ("Blocking read (h) error: %1d %1d", nr, errno));
        clear ();
        return ERROR;
      }
      fill += nr;
    }
    rl = tlength ();
    // The minimum length of a message arriving from the monitor is four
    // bytes. For such a message, bytes 1, 2, 3 always tell the total length
    // the message in bytes.
    if (size < rl) grow (rl);
    while (fill != rl) {
      nc = rl - fill;
      if ((nr = readSocket (sock, RWCAST (Buffer + fill), nc)) <= 0) {
        cdebugl (form ("Blocking read (b) error: %1d %1d", nr, errno));
        clear ();
        return ERROR;
      }
      fill += nr;
    }
    ptr = 0;
    cdebugl (form ("Blocking read OK: %1d, len = %1d bytes", Buffer [0], fill));
    return OK;
  }
  setSocketUnblocking (sock);
  while (fill < 4) {
    // We still don't know how much to expect
    nc = 4 - fill;
    if ((nr = readSocket (sock, RWCAST (Buffer + fill), nc)) < 0)
      return (errno == EAGAIN) ? NONE : ERROR;
    fill += nr;
  }
  if (fill == (rl = tlength ())) {
    ptr = 0;
    cdebugl (form ("Nonblocking read OK: %1d, len = %1d bytes", Buffer [0], fill));
    return OK;  // Got it
  }
  if (rl > size) grow (rl);       // We have to grow the buffer
  // Now we check whether we can read more right away
  while (fill < rl) {
    nc = rl - fill;
    if ((nr = readSocket (sock, RWCAST (Buffer + fill), nc)) < 0)
      return (errno == EAGAIN) ? NONE : ERROR;
    fill += nr;
  }
  ptr = 0;
  cdebugl (form ("Nonblocking read OK: %1d, len = %1d bytes", Buffer [0], fill));
  return OK;
};

int SockBuff::write (int sock, int block) {
// Write the buffer (tlength must be set)
  int nc, nr, rl;
  if (fill == 0) return OK;
  if (block) {
    setSocketBlocking (sock);
    rl = 0;
    while (rl < fill) {
      if ((nr = writeSocket (sock, RWCAST (Buffer + rl), fill-rl)) <= 0) {
        cdebugl (form ("Blocking write fails %1d, %1d", nr, errno));
        return ERROR;
      }
      rl += nr;
    }
    cdebugl (form ("Blocking write OK: %1d, len = %1d bytes", Buffer [0], fill));
    fill = 0;
    return OK;
  }
  setSocketUnblocking (sock);
  while (1) {
    if ((nc = fill - ptr) > AIOGRANL) nc = AIOGRANL;
    // We only try this much at a time
    if ((nr = writeSocket (sock, RWCAST (Buffer + ptr), nc)) < 0)
      return (errno == EAGAIN) ? NONE : ERROR;
    if ((ptr += nr) == fill) {
      cdebugl (form ("Nonblocking write OK: %1d, len = %1d bytes", Buffer [0], fill));
      fill = 0;
      return OK;
    }
  }
};

static void dropMonitorConnection () {
// ----------------------------------
// We call this when the monitor dies
// ----------------------------------
  cdebugl ("Monitor connection dropped");
  if (DisplayActive) serviceEND ();
  zz_closeMonitor ();
};

static void monitorRequest ();
  
/* ----------------------------- */
/* Polled asynchronous I/O check */
/* ----------------------------- */
void zz_check_asyn_io () {      // To be called from main loop
  int st;
  if (monsock < 0) return;      // Do nothing if no connection
  if ((st = MIB->read (monsock)) == OK) {
    monitorRequest ();
  }
/*
#ifdef CDEBUG
  if (st != ERROR) {
    int sf;
    if ((sf = MOB->fill) && (st = MOB->write (monsock)) == OK) {
      cdebugl ("Written:"); MOB->fill = sf; dumpbuf (MOB); MOB->fill = 0;
    }
  }
  if (st == ERROR) dropMonitorConnection ();
#else
*/
  if (st == ERROR || MOB->write (monsock) == ERROR) {
    // Connection lost
    cdebugl ("Connection lost on write");
    dropMonitorConnection ();
  }
/*#endif*/
};

int zz_msock_stat (int &wr) {
  // Returns the fd of the monitor socket and its "write" pending status
  if (monsock >= 0 && MOB)
    wr = !(MOB->empty ());
  else 
    wr = NO;
  return monsock;
};

void    zz_closeMonitor () {
/* ----------------------------- */
/* Closes the monitor connection */
/* ----------------------------- */
  cdebugl ("Closing monitor socket");
  closeSocket (monsock);
  delete MIB;
  delete MOB;
  monsock = NONE;
}

static  void    timeout () {
/* ---------------------- */
/* Timeout signal service */
/* ---------------------- */
  longjmp (tmoutenv, YES);
}

static void settimer (Long delay) {
/* ------------------ */
/* Set interval timer */
/* ------------------ */
  struct  itimerval       td;
  td.it_value.tv_sec = delay;
  td.it_value.tv_usec = 0;
  td.it_interval.tv_sec = 0;
  td.it_interval.tv_usec = 0;
  signal (SIGALRM, (SIGARG) timeout);
  signal (SIGPIPE, (SIGARG) timeout);
  setitimer (ITIMER_REAL, &td, NULL);
}

static void cleartimer () {
/* -------------------- */
/* Clear interval timer */
/* -------------------- */
  struct  itimerval       td;
  td.it_value.tv_sec = 0;
  td.it_value.tv_usec = 0;
  td.it_interval.tv_sec = 0;
  td.it_interval.tv_usec = 0;
  setitimer (ITIMER_REAL, &td, NULL);
  signal (SIGALRM, SIG_DFL);
  signal (SIGPIPE, SIG_DFL);
}

static void monitorError (int en, const char* tx, int send = NO) {
  cdebugl (form ("Monitor error %1d %s %1d", en, tx, send));
  if (send) {
    // Send right away
    sendOctet (PH_ERR);
    sendChar (en);
    sendText (tx);
  } else {
    // Store pending error status
    new ORequest (en, tx);
    zz_events_to_display = 0;
    // Assume the buffer contains garbage
  }
};

static  int     dscrhalf, dscrlast;

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

static  void    sendText (const char *t) {
/* ------------------------------ */
/* Send text string to the server */
/* ------------------------------ */
  sendOctet (PH_TXT);
  while (*t != '\0') sendChar ((int)(*t++));
  sendOctet (0);
};

static  LONG    getInt () {
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
    else if (des > 9) {                      // Spurt error
      monitorError (DERR_ILS, "Illegal integer number");
      MIB->clear ();
      return 0;
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

static  BIG     getBIG () {
/* ---------------------------------------- */
/* Reads a BIG number from the input stream */
/* ---------------------------------------- */
  BIG     res;
  int     des;
  // This shouldn't happen
  if (peekOctet () != PH_BNM) return (BIG_0);
  getOctet ();
  res = BIG_0; dscrhalf = NO;
  while (1) {
    if ((des = getDescriptor ()) == DSC_ENP) // Positive number
      return (res);    // Note: no negative big numbers allowed
    else if (des > 9) {   // Spurt error -- not a decimal digit
      monitorError (DERR_ILS, "Illegal BIG number");
      MIB->clear ();
      return (BIG_0);
    }
    res = res * 10 + des;
  }
  return (res);
};

static  void    sendBIG (BIG val) {
/* ------------------------------- */
/* Send a BIG number to the server */
/* ------------------------------- */
  int     digits [256], ndigits;
  sendOctet (PH_BNM);
  lastshalf = NO;
  for (ndigits = 0; val != BIG_0; ) {
    digits [ndigits++] = (int)(val % 10);
    val /= 10;
  }
  while (ndigits--) sendDescriptor (digits [ndigits]);
  sendDescriptor (DSC_ENP);
  flushDescriptor ();
}

static  void    sendFloat (double val) {
/* ------------------------------------------ */
/* Send a floating point number to the server */
/* ------------------------------------------ */
#if ZZ_NFP
  zz_nfp ("sendFloat");
#else
  char    *digits;
  int     sign, esign;
  char    tmp [32];               // Do not use 'dtoa'

  sendOctet (PH_FNM);
  lastshalf = NO;
  sprintf (tmp, "%g", val);
  digits = tmp;
  sign = NO;

  while (*digits != '\0') {
    if (*digits == ' ' || *digits == '+') {
      digits++;
      continue;
    }
    if (*digits == '-') {
      sign = YES;
      digits++;
      continue;
    }
    if (*digits == '.' || *digits == 'e' || *digits == 'E') {
      // No leading part
      if (*digits != '.') {
	sendDescriptor (DSC_ENP);
	flushDescriptor ();
	return;
      } else 
        sendDescriptor (DSC_DOT);
      break;          // Go process fractional part
    }
    // Process leading part
    while (*digits <= '9' && *digits >= '0') {
      sendDescriptor ((*digits++) - '0');
    }
    break;
  }
  // The integer part is done
  if (*digits == '\0') {
    sendDescriptor (sign ? DSC_ENN : DSC_ENP);
    flushDescriptor ();
    return;
  }
  if (*digits == '.') {
    // Process fractional part
    sendDescriptor (DSC_DOT);
    digits++;
    while (*digits <= '9' && *digits >= '0') {
      sendDescriptor ((*digits++) - '0');
    }
  }
  // Now the exponent
  if (*digits != 'e' && *digits != 'E') {
    sendDescriptor (sign ? DSC_ENN : DSC_ENP);
    flushDescriptor ();
    return;
  }
  sendDescriptor (DSC_EXP);
  digits++;
  esign = NO;
  while (*digits != '\0') {
    if (*digits == '+') {
      digits++;
      continue;
    }
    if (*digits == '-') {
      esign = YES;
      digits++;
      continue;
    }
    while (*digits <= '9' && *digits >= '0')
      sendDescriptor ((*digits++) - '0');
    break;
  }
  sendDescriptor (esign ? DSC_ENN : DSC_ENP);
  sendDescriptor (sign ? DSC_ENN : DSC_ENP);
  flushDescriptor ();
#endif
};

int     zz_connectToMonitor () {
/* ---------------------------------------------------------- */
/* Tries  to  connect the simulator to the monitor; returns a */
/* negative value if the attempt fails                        */
/* ---------------------------------------------------------- */
  char                    txt [128], **args;
  int                     argc, cnt;
  Long                    dummy;
  
  monsock = openClientSocket (ZZ_MONHOST, ZZ_MONSOCK);
  cdebugl ("Connecting to monitor");
  if (monsock < 0) return (monsock = -1);
  cdebugl (form ("Connected to monitor %1d", monsock));
  // Allocate the buffers
  if (MIB == NULL) MIB = new SockBuff (MINMIBSI);
  if (MOB == NULL) MOB = new SockBuff (MINMOBSI);
  MIB->clear ();
  MOB->reset (PH_SIG);
  sendText (SMRPHSIGN);

  // Send the hostname
  cdebugl ("Before hostname");
  if (myHostName (txt, 128) < 0) goto MonitorDead;
  txt [127] = '\0';               // Just in case
  cdebugl (txt);

  sendText (txt);
  // Now for the program call line
  args = zz_callArgs;
  argc = zz_callArgc;
  for (cnt = 0; argc > 0 && cnt < 127; argc--, args++) {
    int l;
    l = strlen (*args);     // The length of the arg
    if (l > 127 - cnt) l = 127 - cnt;
    strncpy (&(txt[cnt]), *args, l);
    cnt += l;
    if (cnt < 127 && argc > 1) txt [cnt++] = ' ';
  }
  txt [cnt] = '\0';
  sendText (txt);
  // Startup date time
  strcpy (txt, tDate ());
  sendText (txt);
  // Process ID
  sendInt (zz_processId);
  MOB->close ();
  if (setjmp (tmoutenv)) {
    // After write timeout
    cdebugl ("After write timeout");
MonitorDead:
    cdebugl ("Dead");
    cleartimer ();
    zz_closeMonitor ();
    return (-1);
  }
  settimer (STIMEOUT);
  if (MOB->write (monsock, YES) == ERROR) goto MonitorDead;
  cleartimer ();
  // OK, this part is done
  return (0);
}

static  void    monitorRequest () {
/* ----------------------- */
/* Process monitor command */
/* ----------------------- */
  int Command, st;
  Command = MIB->header ();
  cdebugl (form ("Monitor request %1d", Command));
  if (DisplayActive) {
    // We are processing DSD requests and ignoring monitor requests
    // (not that we expect many of those).
    if (Command == PH_CRE) {
      // This is just a credit phrase
      cdebugl ("Credit");
      if (more ()) {
        // The message specifies the number of credits
        Credits += getInt ();
      } else {
        // This counts for a single credit
        Credits++;
      }
      if (Credits > MAXXCRED) Credits = MAXXCRED;
    } else {
      // Force immediate display
      cdebugl ("Reply forced immediately");
      zz_events_to_display = 0;
      getResponse (Command);
    }
  } else {
    // The output buffer must be empty
    if ((st = MOB->write (monsock)) == ERROR) {
      cdebugl ("Dropping - output error");
      dropMonitorConnection ();
      return;
    } else if (st == NONE) {
      cdebugl ("Postponing - buffer nonempty");
      return;
      // If the output buffer is full, we have to wait until it becomes empty.
      // We do it by ignoring the request, but leaving the input buffer filled
      // with the command. This way we will keep getting here at every event
      // until the output buffer finally becomes empty.
    }
    switch (Command) {
      case PH_STA:
        cdebugl ("Status");
        MOB->reset (PH_STA);
#if	ZZ_NOC
        sendInt (zz_NRMessages);
#else
        sendInt (0);
#endif
        sendBIG (Time);
        sendInt ((Long)(cpuTime () * 1000.0));  // Cpu time in milliseconds
        MOB->close ();
        // The buffer will be flushed out automatically
        break;
      case PH_DRQ:
        cdebugl ("Display request");
        DisplayActive = YES;
        Credits = INITCRED;
	// Flag initial handshake
	initializing = YES;
        terminating = NO;
	// Force immediate attention of smurph
	zz_events_to_display = 0;
	withinregion = withinwindow = NO;
        // Send some stuff for the first time
        refreshDisplay ();
        break;
        // This is all we have at the moment
      default: ;
        cdebugl ("Garbage -- ignored");
        // Just ignore
    }
  }
  // Make sure the buffer is ready to accomodate another request
  MIB->clear ();
};

static  void    serviceEND () {         // Process END phrase
// -----------------------------
// Terminate the display session
// -----------------------------
  ZZ_WSIT    *wst;
  ZZ_SIT     *st;
  ZZ_WINDOW  *w;
  ORequest   *orq;

  cdebugl ("serviceEND");
  DisplayActive = NO;
  DisplayInterval = DEFDINTVL;
  zz_send_step_phrase = NO;
  terminating = NO;
  // Deallocate lists

  while (ORQ) {
    // List of outgoing requests
    orq = ORQ;
    ORQ = ORQ->next;
    delete orq;
  }
  withinwindow = YES;
  for (zz_the_window = zz_windows; zz_the_window != NULL; ) {
    w = zz_the_window->next;
    DisplayClosing = YES;
    DisplayOpening = NO;
    sitemcount = eitemcount = 0;
    if (zz_the_window->frame != NULL)
      zz_the_window->obj->displayOut (zz_the_window->mode, zz_the_window->stid);
      Assert (withinregion == NO,
        "displayOut: %s, %1d, startRegion not matched by endRegion",
   	  zz_the_window->obj->getSName (), zz_the_window->mode);
      delete (zz_the_window);
      zz_the_window = w;
    }
    withinwindow = NO;
    while (zz_steplist != NULL) {
      st = zz_steplist->next;
      delete (zz_steplist);
      zz_steplist = st;
    }
    while (zz_steplist_delayed != NULL) {
      wst = zz_steplist_delayed->next;
      delete (zz_steplist_delayed);
      zz_steplist_delayed = wst;
    }
    zz_windows = NULL;              // Active windows
    zz_steplist = NULL;             // Stepped windows
    zz_steplist_delayed = NULL;     // Waiting to be stepped
    zz_commit_event = MAX_Long;
    zz_commit_time = TIME_inf;
    initializing = NO;
}

static int waitForDisplay () {
// ------------------------
// Waits for DSD to come in
// ------------------------
  while (!DisplayActive) {
    // Just in case there was some strange request
    if (MOB->write (monsock, YES) == ERROR) return ERROR;
    if (MIB->read (monsock, YES) == ERROR) return ERROR;
    cdebugl ("waitForDisplay");
    monitorRequest ();
    // We start stepped, so that the monitor can have a look at things
    zz_send_step_phrase = YES;
    zz_events_to_display = 0;
  }
  return OK;
};

#endif   // ZZ_MONHOST

// Shared stuff for templates

#include "dshared.cc"

#ifdef	ZZ_MONHOST

void    zz_init_windows () {            // Initialize display module
  int     f;

  f = zz_connectToMonitor ();
  if (f >= 0 && !Templates && zz_TemplateFile) readTemplates ();
  if (zz_drequest) {              // Requested by '-d'
    if (f < 0) {
CERR:
      excptn ("initWindows/waitForDisplay: cannot connect to monitor");
    }
    // Now we just wait until we get a display request from the
    // monitor
    if (waitForDisplay () == ERROR) goto CERR;
  }
}

int     requestDisplay (const char *msg) {
/* -------------------------------- */
/* Display request from the program */
/* -------------------------------- */
  if (!zz_flg_started)
    excptn ("requestDisplay: illegal in the first state of Root");
  if (monsock < 0) {
CERR:
    excptn ("requestDisplay: display impossible -- no connection to monitor");
  }
  if (DisplayActive) return (ERROR);   // Display already active
  if (waitForDisplay () == ERROR) goto CERR;
  if (msg) monitorError (DERR_NOT, msg);
  return OK;
}

static void processMenuRequest (int up) {
/* -------------------- */
/* Process menu request */
/* -------------------- */
  char sname [256];
  int i;
  if (more ()) {
    // The standard name is present
    if (getOctet () != PH_TXT) {
      // If empty, it means the root
      monitorError (DERR_ILS, "Standard name missing from menu request");
      MIB->clear ();
      return;
    }
    for (i = 0; i < 256; i++) if ((sname [i] = getOctet ()) == '\0') break;
    if (i >= 256) {
      monitorError (DERR_ILS,
        "Standard name in menu request is longer than 256 characters");
      MIB->clear ();
      return;
    }
  } else
    sname [0] = '\0';
  new ORequest (up ? ORQ_UPMENU : ORQ_MENU, sname);
  // Well, these requests are not really processed here. They are just queued
  // and will be processed by refreshDisplay
};

static  void    getResponse (int Cmd) {
/* -------------------------------------------------- */
/* Get the monitor's response and process the request */
/* -------------------------------------------------- */
  // Note: we only consider one command at a time. If there is anything
  // more waiting to be read, it will be taken care of at the next event.
  int   d;
  switch (Cmd) {
      case  PH_DIN:         // New display interval
        cdebugl ("getResponse DIN");
        if ((d = peekOctet ()) == PH_INM) {
          if ((d = (int) getInt ()) < 0)
            monitorError (DERR_NSI, "Negative display interval");
	  else
	    DisplayInterval = d;
	} else
	    DisplayInterval = DEFDINTVL;
	break;
      case  PH_MEN:         // Object menu request
        cdebugl ("getResponse MEN");
        processMenuRequest (NO);
        break;
      case  PH_UPM:         // Up menu request
        cdebugl ("getResponse UPM");
        processMenuRequest (YES);
        break;
      case  PH_WND:         // A window request phrase
        cdebugl ("getResponse WND");
        zz_processWindowPhrase ();
        break;
      case  PH_EWN:         // End-window phrase
        cdebugl ("getResponse EWN");
        processEWindowPhrase ();
        break;
      case  PH_SRL:         // Step release
        cdebugl ("getResponse SRL");
        zz_send_step_phrase = NO;
        break;
      case  PH_ESM:         // Global exit from step mode
        cdebugl ("getResponse ESM");
        processEStepMode ();
        break;
      case  PH_END:         // Terminate display
        cdebugl ("getResponse END");
        new ORequest (ORQ_END);
        return;
      case  PH_EXI:         // Terminate simulation
        cdebugl ("getResponse EXI");
        zz_max_Time = Time;
        break;
      default: ;            // Ignore
  }
};

void zz_sendObjectMenu (ZZ_Object *o) {
/* --------------------------------------------------------------------- */
/* Sends to the server the list of objects owned by the indicated object */
/* --------------------------------------------------------------------- */
  ZZ_Object  *clist;
  char *sn, *bn, *nn;
  // Get to the child list. Unfortunately, this depends on object type.
  switch (o->Class) {
    case    OBJ_station:
      clist = ((Station*)o)->ChList;  break;
#if	ZZ_NOC
    case    AIC_traffic:
      clist = ((Traffic*)o)->ChList;  break;
#endif
    case    AIC_process:
      clist = ((Process*)o)->ChList;  break;
    case    AIC_observer:
      clist = ((Observer*)o)->ChList; break;
    default:
      clist = NULL;
  }
  // Start from the object itself
  sendOctet (PH_MEN);
  sendText (sn = o->getSName ());
  bn = o->getBName ();
  if (strcmp (bn, sn) == 0) bn = "";
  sendText (bn);
  if ((nn = o->getNName ()) == NULL) nn = "";
  sendText (nn);
  queueTemplates (sn, bn, nn);

  while (clist != NULL) {
    if (!zz_flg_nosysdisp || o != Kernel || clist->Class != AIC_process ||
      ((Process*)clist)->Owner != System || clist == ZZ_Main) {
      // Ignore system processes, if '-s' hasn't been selected
      sendText (sn = clist->getSName ());
      // The object's base name
      if (strcmp (sn, bn = clist->getBName ()) == 0) bn = "";
      sendText (bn);
      // The object's nickname
      if ((nn = clist->getNName ()) == NULL) nn = "";
      sendText (nn);
      queueTemplates (sn, bn, nn);
    }
    clist = clist->next;
  }
  cdebugl ("sendObjectMenu");
};

static  char       *objtofind;          // To make recursion less expensive
static  ZZ_Object  *objfound, *fchild;

ZZ_Object *zz_findAdvance (ZZ_Object *o) {
/* --------------------------------- */
/* Recursive searcher for findObject */
/* --------------------------------- */
  ZZ_Object  *clist;
  if (strcmp (o->getSName (), objtofind) == 0) return (o);
  switch (o->Class) {
    case    OBJ_station:
      clist = ((Station*)o)->ChList;  break;
#if	ZZ_NOC
    case    AIC_traffic:
      clist = ((Traffic*)o)->ChList;  break;
#endif
    case    AIC_process:
      clist = ((Process*)o)->ChList;  break;
    case    AIC_observer:
      clist = ((Observer*)o)->ChList; break;
    default:
      clist = NULL;
  }
  if (clist == NULL) return (NULL);       // Same thing
  for ( ; clist != NULL; clist = clist->next)
    if ((objfound = zz_findAdvance (clist)) != NULL)
      return (objfound);
  return (NULL);                          // Not found there
}

ZZ_Object *zz_findWAdvance (ZZ_Object *o) {
/* -------------------------------- */
/* Recursive searcher for findOwner */
/* -------------------------------- */
  ZZ_Object  *clist;
  switch (o->Class) {
    case    OBJ_station:
      clist = ((Station*)o)->ChList;
      break;
#if	ZZ_NOC
    case    AIC_traffic:
      clist = ((Traffic*)o)->ChList;
      break;
#endif
    case    AIC_process:
      clist = ((Process*)o)->ChList;
      break;
    case    AIC_observer:
      clist = ((Observer*)o)->ChList;
      break;
    default:
      return (NULL);                      // Nothing to do
  }
  if (clist == NULL) return (NULL);       // Same thing
  for ( ; clist != NULL; clist = clist->next) {
    if (clist == fchild) return (o);
    if ((objfound = zz_findWAdvance (clist)) != NULL)
      return (objfound);
  }
  return (NULL);                          // Not found there
}

static  ZZ_Object  *findOwner (ZZ_Object *o) {
/* ----------------------------- */
/* Finds the owner of object 'o' */
/* ----------------------------- */
  fchild = o;
  return (zz_findWAdvance (System));
};

static  ZZ_Object  *findObject (char *sn) {
/* ---------------------------------------------------------- */
/* Searches  the  system  object tree for the object with the */
/* given standard name                                        */
/* ---------------------------------------------------------- */
  objtofind = sn;
  return (zz_findAdvance (System));
};

void  zz_processWindowPhrase () {
/* ------------------------------------------------ */
/* Reads one window phrase arriving from the server */
/* ------------------------------------------------ */

  char    sname [256];    // The object's standard name
  int     i, orgmode,
          mode,           // Window mode
          stat;           // Station relative

  BIG     steptime;
  Long    stepevent;
  int     delmode;        // Defer mode

  int     scount,         // Item count limit -- low
          ecount;         // and high

  ZZ_WINDOW  *w;          // Window list pointers
  ZZ_SIT     *ws;
  ZZ_WSIT    *wsd;

  Station         *stepstation;   // Step parameters
  Process         *stepprocess;
  AI              *stepai;
  Observer        *stepobs;

  // Get the standard name of the object

  if (getOctet () != PH_TXT) {
    // Formal error
    monitorError (DERR_ILS, "Standard name missing from WND phrase");
    return;
  }
  for (i = 0; i < 256; i++)
    if ((sname [i] = getOctet ()) == '\0') break;

  if (i >= 256) {
    monitorError (DERR_ILS,
        "Standard name in WND phrase is longer than 256 characters");
    return;
  }
  if (peekOctet () != PH_INM) {
    // Error -- mode indicator expected
    monitorError (DERR_ILS, "Mode indicator missing in a WND phrase");
    return;
  }
  if ((orgmode = (int) getInt ()) < 0) {
    // Illegal mode. The case of a too high mode cannot be detected
    // here. It will be detected dynamically when the window is
    // displayed.
    mode = MAX_int; // Force it to be positive, but still illegal
  } else
    mode = orgmode;

  // Check if the window is to be station-relative
  if (peekOctet () == PH_INM) {
    // Station-relative window
    if ((stat = (int) getInt ()) < 0 || stat > NStations - 1) {
      // Illegal station
      monitorError (DERR_ISN, "Illegal station number");
      return;
    }
  } else
    stat = NONE;    // Not station-relative

  cdebugl (form ("Window phrase: %s, %1d, %1d", sname, mode, stat));

  stepevent = MAX_Long;
  steptime = BIG_inf;
  if (peekOctet () == PH_STP) {
    getOctet ();
    // Process STP subphrase
    cdebugl ("Step subphrase");
    if ((i = peekOctet ()) == PH_BNM) {
      // Time-delayed
      delmode = 1;
      steptime = getBIG ();
      if (peekOctet () == PH_REL) {
        // Relative
        steptime += Time;
        getOctet ();
      }
    } else if (i == PH_INM) {
      // Event-delayed
      delmode = -1;
      stepevent = getInt ();
      if (peekOctet () == PH_REL) {
        // Relative
        stepevent += zz_npre;
        getOctet ();
      }
    } else if (i == PH_ESM) {       // Exit step mode
      delmode = 2;
      getOctet ();
    } else
      delmode = 0;            // Undelayed step
  } else
    delmode = -2;                   // No step mode
  cdebugl (form ("Delmode %1d", delmode));

  // Check if item limit specified
  scount = 0;
  ecount = MAX_int;
  if (peekOctet () == PH_TRN) {
    getOctet ();
    if (peekOctet () != PH_INM) {
      // Formal error
      monitorError (DERR_ILS, "TRN phrase corrupted");
      return;
    }
    if ((scount = (int) getInt ()) < 0) scount = 0;
    if (peekOctet () != PH_INM || (ecount = (int) getInt ()) < scount) {
      // Formal error
      monitorError (DERR_ILS, "TRN phrase corrupted");
      return;
    }
    cdebugl (form ("Item bounds: %1d, %1d", scount, ecount));
  }

  // The entire phrase has been read in. Now process it
  if (delmode == 2) {
    // Exit step mode
    for (ws = zz_steplist; ws != NULL; ws = ws->next) {
      // Find the window on the step list
      if (ws->win->mode != mode ||
        ws->win->stid != stat) continue;
      if (strcmp (ws->win->obj->getSName (), sname) != 0)
        continue;
      // We've got it
      queue_out (ws);
      delete (ws);
      break;
    }
    if (ws != NULL) return;

    // Not found there: check if deferred
    for (wsd = zz_steplist_delayed; wsd != NULL; wsd = wsd->next) {
      // Find the window on the step list
      if (wsd->win->mode != mode ||
        wsd->win->stid != stat) continue;
      if (strcmp (wsd->win->obj->getSName (), sname) != 0)
        continue;
      // We've got it
      queue_out (wsd);
      delete (wsd);
      // Perhaps we should update here the minimum intervals
      // until the earliest deferred window will be stepped.
      // We don't do it. The event scheduler in main will have
      // to check whether such a window exists and if not,
      // update the intervals appropriately.
      break;
    }
    if (wsd == NULL) monitorError (DERR_CUN, "No window to unstep");
    return;                 // This part done
  }

  // Add the window, if not present
  for (w = zz_windows; w != NULL; w = w->next) {
    // Check if the window occurs on the active list
    if (w->mode != mode || w->stid != stat) continue;
    if (strcmp (w->obj->getSName (), sname) == 0) break;
  }

  if (w == NULL) {                // Must be added
    cdebugl ("Window not found -- added");
    ZZ_Object  *o;
    // Find the object in question
    if ((o = findObject (sname)) == NULL) {
      // No such object
      monitorError (DERR_ONF, "Object not found");
      return;
    }
    w = new ZZ_WINDOW (o, mode, stat, scount, ecount);
    queue_head (w, zz_windows, ZZ_WINDOW);
  } else {
    // A possible count modification
    if (ecount != MAX_int) {
	  w->scount = scount;
	  w->ecount = ecount;
	}
  }

  // Now determine the step status
  if (delmode == -2) return;      // No stepping

  // Step the window
  stepstation     = NULL;
  stepprocess     = NULL;
  stepai          = NULL;
  stepobs         = NULL;
  switch (w->obj->Class) {
    ZZ_Object *o;
    case  OBJ_station:
      // Step this station and nothing else
      stepstation = (Station*)(w->obj);
      break;
    case  AIC_process:
      // Step this process
      if (w->obj == Kernel) break;    // Global step
      stepprocess = (Process*)(w->obj);
      break;
#if	ZZ_NOS
    case  OBJ_rvariable:
      o = findOwner (w->obj);
      Assert (o != NULL,
          "processWindowPhrase: internal error -- owner not found");
      if (o->Class == OBJ_station)
        stepstation = (Station*)o;
      else if (o->Class == AIC_process)
        stepprocess = (Process*)o;
#if	ZZ_NOC
      else if (o->Class == AIC_traffic)
        stepai = (AI*)o;
#endif
      else
      excptn ("processWindowPhrase: internal error -- illegal RVariable owner");
      break;
#endif /* NOS */
    case  OBJ_eobject:
      o = findOwner (w->obj);
      Assert (o != NULL,
          "processWindowPhrase: internal error -- owner not found");
      if (o->Class == OBJ_station)
        stepstation = (Station*)o;
      else if (o->Class == AIC_process)
        stepprocess = (Process*)o;
      else
        excptn ("processWindowPhrase: internal error -- illegal EObject owner");
      break;
    case  AIC_observer:
      stepobs = (Observer*) (w->obj);
      break;
    default:
      // Only AIs are left
      stepai = (AI*)(w->obj);
      if (
#if	ZZ_NOL
           w->obj->Class != AIC_port &&
#endif
                                         w->obj->Class != AIC_mailbox
        && stat != NONE)
          stepstation = idToStation (stat);
  }
  if (delmode == 0) {             // Immediate stepping
    ws = new ZZ_SIT;
    ws->win = w;            // For this window
    ws->station = stepstation;
    ws->process = stepprocess;
    ws->ai      = stepai;
    ws->obs     = stepobs;
    queue_head (ws, zz_steplist, ZZ_SIT);
    return;
  }
  // Deferred stepping
  wsd = new ZZ_WSIT;
  wsd->win = w;                   // For this window
  wsd->station = stepstation;
  wsd->process = stepprocess;
  wsd->ai      = stepai;
  wsd->obs     = stepobs;
  wsd->cevent  = stepevent;
  wsd->ctime   = steptime;
  if (zz_steplist_delayed == NULL) {
    zz_commit_event = MAX_Long;
    zz_commit_time  = TIME_inf;
  }
  queue_head (wsd, zz_steplist_delayed, ZZ_WSIT);
  // Update minimum intervals
  if (stepevent < zz_commit_event) zz_commit_event = stepevent;
  if (steptime < zz_commit_time) zz_commit_time = steptime;
  return;
};

static  void    processEStepMode () {
/* ---------------------------- */
/* Exits globally the step mode */
/* ---------------------------- */
  ZZ_SIT     *ws, *wws;
  ZZ_WSIT    *wsd, *wwsd;

  for (ws = zz_steplist; ws != NULL; ws = wws) {
    wws = ws->next;
    queue_out (ws);
    delete (ws);
  }
  for (wsd = zz_steplist_delayed; wsd != NULL; wsd = wwsd) {
    wwsd = wsd->next;
    queue_out (wsd);
    delete (wsd);
  }
};

static  void    processEWindowPhrase () {
/* ----------------------------- */
/* Removes a window from display */
/* ----------------------------- */
  char    sname [256];
  int     i, mode, orgmode, stat;
  ZZ_WINDOW  *w;
  ZZ_SIT     *ws;
  ZZ_WSIT    *wsd;

  if (getOctet () != PH_TXT) {
    // Formal error
    monitorError (DERR_ILS, "Standard name missing from EWN phrase");
    return;
  }
  // Get the standard name of the window's object
  for (i = 0; i < 256; i++)
    if ((sname [i] = (char) (getOctet ())) == '\0') break;

  if (i >= 256) {
    // Standard name too long -- assume "illegal spurt" error
    monitorError (DERR_ILS,
      "Standard name in EWN phrase is longer than 256 characters");
    return;
  }
  if (peekOctet () != PH_INM) {
    // Error -- mode indicator expected
    monitorError (DERR_ILS, "Mode indicator missing in a EWN phrase");
    return;
  }
  if ((orgmode = (int) getInt ()) < 0) 
    // Illegal mode, make sure it is positive, though
    mode = MAX_int;
  else
    mode = orgmode;

  // Check if the window is station-relative
  if (peekOctet () == PH_INM) {
    // Station-relative window
    if ((stat = (int) getInt ()) < 0 || stat > NStations - 1) {
      monitorError (DERR_ISN, "Illegal station number");
      return;
    }
  } else
    stat = NONE;    // Not station-relative

  // Find the window on the active list
  for (w = zz_windows; w != NULL; w = w->next) {
    if (w->mode != mode || w->stid != stat) continue;
    if (strcmp (w->obj->getSName (), sname) == 0) break;
  }
  if (w == NULL) {
    // Not found
    monitorError (DERR_ONF, "Window not found");
    return;
  }
  zz_the_window = w;
  DisplayOpening = NO;
  DisplayClosing = YES;
  sitemcount = eitemcount = 0;
  withinwindow = YES;
  if (w->frame != NULL) w->obj->displayOut (w->mode, w->stid);
  Assert (withinregion == NO,
    "displayOut: %s, %1d, startRegion not matched by endRegion",
	w->obj->getSName (), w->mode);
  withinwindow = NO;
  queue_out (w);          // Remove from the active list

  // Check if occurs on steplists
  for (ws = zz_steplist; ws != NULL; ws = ws->next)
    if (ws->win == w) {
      queue_out (ws);
      delete (ws);
      break;
    }
  for (wsd = zz_steplist_delayed; wsd != NULL; wsd = wsd->next)
    if (wsd->win == w) {
      queue_out (wsd);
      delete (wsd);
      break;
    }
  delete w;
};

static void startBlock () {
  if (MOB->empty ())
    MOB->reset (PH_BLK);
};

static void sendPending () {
/* --------------------------------------------- */
/* Sends the pending outgoing stuff: errors, etc */
/* --------------------------------------------- */
  ORequest *orq;
  ZZ_Object *ob;
  if (!ORQ) return;
  cdebugl ("sendPending");
  startBlock ();
  while (ORQ) {
    switch (ORQ->type) {
      case ORQ_MENU:
        ob = findObject (ORQ->contents);
        if (ob == NULL)
          monitorError (DERR_ONF, "Menu object not found", YES);
        else
          zz_sendObjectMenu (ob);
        break;
      case ORQ_UPMENU:
        ob = findObject (ORQ->contents);
        if (ob != NULL) ob = findOwner (ob);
        if (ob == NULL) {
	  monitorError (DERR_ONF, "Object owner not found", YES);
	  ob = System;
	}
        zz_sendObjectMenu (ob);
        break;
      case ORQ_REMOVE:
        // Forced window removal
        sendOctet (PH_REM);
        sendText (ORQ->contents);
        sendInt (ORQ->par1);
        sendInt (ORQ->par2);
        break;
      case ORQ_TEMPLATE:
        ((Template*)(ORQ->par1))->send ();
        break;
      case ORQ_END:
        cdebugl ("SENDING END");
        sendOctet (PH_END);
        terminating = YES;
        break;
      default:
        // This is an error message
        sendOctet (PH_ERR);
        sendChar (ORQ->type);
        sendText (ORQ->contents);
    }
    ORQ = (orq = ORQ)->next;
    delete orq;
  }
};

void    refreshDisplay () {
/* ------------------------- */
/* Process one display cycle */
/* ------------------------- */
  int st, WindowsSent;

  if (Credits == 0 && !zz_send_step_phrase && !ORQ) {
    zz_events_to_display = RefreshDelay;
    if (++RefreshDelay > DisplayInterval) RefreshDelay = DisplayInterval;
    cdebugl ("No credits");
    return;
  }
  if ((st = MOB->write (monsock)) == NONE) {
    // The output buffer is not empty
    cdebugl ("Buffer full");
    if (zz_send_step_phrase) {
      // We are being stepped, so we must wait for the buffer to become
      // empty without executing a single event in the meantime.
      st = MOB->write (monsock, YES);
    } else {
      // Skip this turn, but don't wait as long for the next one
      zz_events_to_display = RefreshDelay;
      if (++RefreshDelay > DisplayInterval) RefreshDelay = DisplayInterval;
      return;
    }
  }
  if (st == ERROR) {
    cdebugl ("Connection lost");
    // Connection lost: we cancel the whole business
    dropMonitorConnection ();
    return;
  }
  RefreshDelay = 1;
  // Start from fulfilling the outstanding requests
  sendPending ();
  if (initializing) {
    cdebugl ("Sending initializing");
    startBlock ();
    // The first time around -- reply with some parameters
    initializing = NO;   // Won't do it next time
    // BIG precision. This phrase indicates to DSD that a session has begun.
    sendOctet (PH_BPR);
    sendInt (BIG_precision * (BIL-1));
    // The number of stations
    sendInt (NStations);
    // Display interval
    sendInt (DisplayInterval);
    // The initial menu
    zz_sendObjectMenu (System);
  }
  // Regular update
  // cdebugl ("Sending regular");
  withinwindow = YES;
  WindowsSent = NO;
  for (zz_the_window = zz_windows; zz_the_window != NULL;
		                         zz_the_window = zz_the_window->next) {
    startBlock ();
    // Set item number limit
    sitemcount = zz_the_window->scount;
	eitemcount = zz_the_window->ecount;
    DisplayClosing = NO;
    DisplayOpening = (zz_the_window->flag & WF_OPENING);
    // Start the phrase
    sendOctet (PH_WND);
    sendText (zz_the_window->obj->getSName ());
    sendInt (zz_the_window->mode);
    sendInt (zz_the_window->stid);
    sendInt (zz_the_window->scount);   // So that we know where it starts
    zz_dispfound = NO;
    zz_the_window->obj->displayOut (zz_the_window->mode, zz_the_window->stid);
    Assert (withinregion == NO,
      "displayOut: %s, %1d, startRegion not matched by endRegion",
 	zz_the_window->obj->getSName (), zz_the_window->mode);
    zz_the_window->flag = 0;        // Reset the status
    if (!zz_dispfound)
      monitorError (DERR_NMO, "Nonexistent mode", YES);
    else
      WindowsSent = YES;
  }
  withinwindow = NO;
  if (WindowsSent && Credits) Credits--;
  if (zz_send_step_phrase) {
    startBlock ();
    // Notify the server that some stepped windows have been included
    sendOctet (PH_STP);
  }
  while (DisplayActive) {
    if (!MOB->empty ()) {
      MOB->close ();
    }
    if (!zz_send_step_phrase) break;
    st = MOB->write (monsock, YES);
    if (st == ERROR) {
      cdebugl ("Step phrase write error");
      dropMonitorConnection ();
      return;
    }
    if (terminating) break;
    if (MIB->read (monsock, YES) == ERROR) {
      cdebugl ("Step read error");
      dropMonitorConnection ();
      return;
    }
    monitorRequest ();
    sendPending ();
  }
  zz_events_to_display = DisplayInterval;
  if (terminating) serviceEND ();
}

void    zz_DREM (ZZ_Object *ob) {
// -------------------------------
// Removal of a displayable object
// -------------------------------
  ZZ_WINDOW  *w, *wq;
  ZZ_SIT     *ws, *wsq;
  ZZ_WSIT    *wws, *wwsq;

  if (!DisplayActive) return;

  for (ws = zz_steplist; ws != NULL; ws = wsq) {
    wsq = ws->next;
    if (ws->win->obj == ob) {
      queue_out (ws);
      delete (ws);
    }
  }
  for (wws = zz_steplist_delayed; wws != NULL; wws = wwsq) {
    wwsq = wws->next;
    if (wws->win->obj == ob) {
      queue_out (wws);
      delete (wws);
    }
  }
  for (w = zz_windows; w != NULL; w = wq) {
    wq = w->next;
    if (w->obj == ob) {
      new ORequest (ORQ_REMOVE, ob->getSName (), w->mode, w->stid);     
      queue_out (w);
      delete (w);
    }
  }
};

void    zz_dterminate () {
/* ------------------------------------ */
/* Informs the server about termination */
/* ------------------------------------ */
  cdebugl ("dterminate");
  if (!DisplayActive) {
    zz_closeMonitor ();
    return;
  }
  if (setjmp (tmoutenv)) {
    // After write timeout
MonitorDead:
    cleartimer ();
    zz_closeMonitor ();
    return;
  }
  settimer (CTIMEOUT);
  if (MOB->write (monsock, YES) == ERROR) goto MonitorDead;
  refreshDisplay ();
  if (MOB->write (monsock, YES) == ERROR) goto MonitorDead;
  startBlock ();
  sendOctet (PH_EXI);
  MOB->close ();
  if (MOB->write (monsock, YES) == ERROR) goto MonitorDead;
  goto MonitorDead;
};

void displayNote (const char *t) {
/* ------------------------- */
/* Send a note to the server */
/* --------------------------*/
  if (DisplayActive) {
    monitorError (DERR_NOT, t);
    zz_events_to_display = 0;
  }
};

void    display (LONG p) {
/* -------------------------------------------------- */
/* User-visible function to display an integer number */
/* -------------------------------------------------- */
  if (!withinwindow)
    excptn ("display (int): called from outside of expose");
  itemcheck;
  sendInt (p);
}

#if     BIG_precision != 1

void    display (BIG p) {
/* --------------------------------------------- */
/* User-visible function to display a BIG number */
/* --------------------------------------------- */
  if (!withinwindow)
    excptn ("display (BIG): called from outside of expose");
  itemcheck;
  sendBIG (p);
}

#endif

#if  ZZ_TAG
void    display (ZZ_TBIG &p) {
/* --------------------------------------------- */
/* Display a tagged time value (used internally) */
/* --------------------------------------------- */
  TIME    tt;
  p . get (tt);
  display (tt);
}
#endif

void    display (double p) {
/* -------------------------------------------------------- */
/* User-visible function to display a floating point number */
/* -------------------------------------------------------- */
#if ZZ_NFP
  zz_nfp ("display");
#else
  if (!withinwindow)
    excptn ("display (float): called from outside of expose");
  itemcheck;
  sendFloat (p);
#endif
}

void    display (const char *p) {
/* --------------------------------------------------- */
/* User-visible function to display a character string */
/* --------------------------------------------------- */
  if (!withinwindow)
    excptn ("display (text): called from outside of expose");
  itemcheck;
  sendText (p);
}

void    display (char c) {
/* ----------------------------- */
/* To display a single character */
/* ----------------------------- */
  char    tt [2];
  if (!withinwindow)
    excptn ("display (char): called from outside of expose");
  tt[0] = c; tt[1] = '\0';
  sendText (tt);
};

// Segment stuff. We have reduced the amout of scaling work required by
// DSD; in particular, there is no floating point processing to be done by
// DSD. This forces us to do some work here.

#if ZZ_NFP
// No floating point - regions illegal
#else
static double   xsbxo, ysbyo, xotxsbxo, yotysbyo;

static  void    setScale (double xo, double xs, double yo, double ys) {
// We don't send the scale any more. Instead, we do all the scaling ourselves
// transforming the coordinates into 0..MAX_short, 0..MAX_short.
  xsbxo = (double) MAX_short / (xs - xo);
  xotxsbxo = xo * xsbxo;
  ysbyo = (double) MAX_short / (ys - yo);
  yotysbyo = yo * ysbyo;
  // We have calculated coefficients for transforming point coordinates into
  // the range 0..MAX_short. Namely, a coordinate, say x, between xo and xs
  // should be transformed as follows:
  //
  //    xx = Max_short * (x - xo) / (xs - xo) * MAX_short;
  //
};

static void setUnscaled () {
// Here we assume a default scale from from 0.0 to 1.0. This may not make a lot
// of sense  (and I don't think anybody has ever used this feature ;-), but the
// code is there, so ...
  setScale (0.0, 1.0, 0.0, 1.0);
};

static inline void sendPoint (double x, double y) {
  int tx, ty;
  if ((tx = (int) (x * xsbxo - xotxsbxo)) > MAX_short) tx = MAX_short;
  if ((ty = (int) (y * ysbyo - yotysbyo)) > MAX_short) ty = MAX_short;
  sendInt (tx);
  sendInt (ty);
  // Now we only send integers in segments. This will simplify a bit the
  // processing by DSD
};

static  void    sendSegment (Long att, int n, double *x, double *y) {
/* --------------------------------- */
/* Send to the server a line segment */
/* --------------------------------- */
  sendOctet (PH_SLI);
  if (att == NONE) {
    sendOctet (0);  // Defaults
    sendOctet (1);
  } else {
    sendOctet ((int)((att >> 7) & 0177));
    sendOctet ((int)(att & 0177));
  }
  while (n-- > 0) sendPoint (*x++, *y++);
};
#endif

void    display (double xo, double xs, double yo, double ys, Long att,
      int n, double *x, double *y) {
/* ----------------------------------------------- */
/* Display a region consisting of a single segment */
/* ----------------------------------------------- */
#if ZZ_NFP
  zz_nfp ("display (REG)");
#else
  if (!withinwindow)
    excptn ("display (REG): called from outside of expose");
  itemcheck;
  sendOctet (PH_REG);
  setScale (xo, xs, yo, ys);
  sendSegment (att, n, x, y);
  sendOctet (PH_REN);
#endif
};

void    display (Long att, int n, double *x, double *y) {
/* -------------------------------------------------------- */
/* Display a region consisting of a single unscaled segment */
/* -------------------------------------------------------- */
#if ZZ_NFP
  zz_nfp ("display (REG)");
#else
  if (!withinwindow)
    excptn ("display (REG): called from outside of expose");
  itemcheck;
  sendOctet (PH_REG);
  setUnscaled ();
  sendSegment (att, n, x, y);
  sendOctet (PH_REN);
#endif
};

void    display (int n, double *x, double *y) {
/* ---------------------------------------------------------- */
/* Display  a  region consisting of a single unscaled segment */
/* with default attributes                                    */
/* ---------------------------------------------------------- */
#if ZZ_NFP
  zz_nfp ("display (REG)");
#else
  if (!withinwindow)
    excptn ("display (REG): called from outside of expose");
  itemcheck;
  sendOctet (PH_REG);
  setUnscaled ();
  sendSegment (NONE, n, x, y);
  sendOctet (PH_REN);
#endif
};

void    startRegion (double xo, double xs, double yo, double ys) {
/* ----------------------------------- */
/* Start a scaled multi-segment region */
/* ----------------------------------- */
#if ZZ_NFP
  zz_nfp ("startRegion");
#else
  if (!withinwindow)
    excptn ("startRegion (...) called from outside of expose");
  if (withinregion)
    excptn ("startRegion within a region");
  withinregion = YES;
  itemcheck;
  sendOctet (PH_REG);
  setScale (xo, xs, yo, ys);
#endif
};

void    startRegion () {
/* -------------------------------------- */
/* Start an unscaled multi-segment region */
/* -------------------------------------- */
#if ZZ_NFP
  zz_nfp ("startRegion");
#else
  if (!withinwindow)
    excptn ("startRegion called from outside of expose");
  if (withinregion)
    excptn ("startRegion within a region");
  withinregion = YES;
  itemcheck;
  sendOctet (PH_REG);
  setUnscaled ();
#endif
};

void    endRegion () {
/* -------------------------- */
/* End a multi-segment region */
/* -------------------------- */
#if ZZ_NFP
  zz_nfp ("endRegion");
#else
  if (!withinwindow)
    excptn ("endRegion called from outside of expose");
  if (!withinregion)
    excptn ("endRegion not within a region");
  withinregion = NO;
  itemtest;
  sendOctet (PH_REN);
#endif
};

void    displaySegment (Long att, int n, double *x, double *y) {
/* ------------------------------------------- */
/* Display a segment of a multi-segment region */
/* ------------------------------------------- */
#if ZZ_NFP
  zz_nfp ("displaySegment");
#else
  if (!withinwindow)
    excptn ("displaySegment: called from outside of expose");
  itemtest;
  sendSegment (att, n, x, y);
#endif
};

void    displaySegment (int n, double *x, double *y) {
/* ---------------------------------------------------------- */
/* Display   a  multi-segment  region  segment  with  default */
/* atributes                                                  */
/* ---------------------------------------------------------- */
#if ZZ_NFP
  zz_nfp ("displaySegment");
#else
  if (!withinwindow)
    excptn ("displaySegment: called from outside of expose");
  itemtest;
  sendSegment (NONE, n, x, y);
#endif
};

void    startSegment (Long att) {
/* ----------------------------------------- */
/* Start a segment of a multi-segment region */
/* ----------------------------------------- */
#if ZZ_NFP
  zz_nfp ("startSegment");
#else
  if (!withinwindow)
    excptn ("displaySegment: called from outside of expose");
  itemtest;

  sendOctet (PH_SLI);
  if (att == NONE) {
    sendOctet (0);  // Defaults
    sendOctet (1);
  } else {
    sendOctet ((int)((att >> 7) & 0177));
    sendOctet ((int)(att & 0177));
  }
#endif
};

void    displayPoint (double x, double y) {
/* ---------------------------------------------------------- */
/* Display   a  multi-segment  region  segment  with  default */
/* atributes                                                  */
/* ---------------------------------------------------------- */
#if ZZ_NFP
  zz_nfp ("displayPoint");
#else
  if (!withinwindow)
    excptn ("displaySegment: called from outside of expose");
  itemtest;
  sendPoint (x, y);
#endif
};

void    endSegment () {
/* ------------------- */
/* Terminate a segment */
/* ------------------- */
  // Truly void
#if ZZ_NFP
  zz_nfp ("endSegment");
#else
  if (!withinwindow)
    excptn ("endSegment: called from outside of expose");
#endif
}

// ---------------------------------
// Local stuff for reading templates
// ---------------------------------

static void tabt (const char *msg) {
/* ---------------------- */
/* Process template error */
/* ---------------------- */
  char t [SANE];
  strcpy (t, "Templates: ");
  strcat (t, msg);
  excptn (t);
};

static void queueTemplates (const char *sn, const char *bn, const char *nn) {
// ----------------------------------------------------------
// Queues templates matching the three names for transmission
// ----------------------------------------------------------
  Template *ct;
  ORequest *orq;
  for (ct = Templates; ct; ct = ct->next) {
    // Check if the template is not queued already
    for (orq = ORQ; orq != NULL; orq = orq->next)
      if (orq->type == ORQ_TEMPLATE && (Template*)(orq->par1) == ct) break;
    if (orq == NULL) {
      // Check if any of the three names matches the template name
      if (tmatch (ct, sn) || tmatch (ct, bn) || tmatch (ct, nn))
        new ORequest (ORQ_TEMPLATE, NULL, (IPointer)ct);
    }
  }
};

#else

// Stubs for the display functions

ZZ_WINDOW::ZZ_WINDOW (ZZ_Object *o, int md, int stat, int scnt, int ecnt) {
  o += md + stat + scnt + ecnt;
};

void zz_check_asyn_io () { };
int zz_msock_stat (int &wr) { wr = NO; return NONE; };
void zz_closeMonitor () { };
int zz_connectToMonitor () { return NONE; };
void zz_init_windows () { };
int requestDisplay (const char *msg) { msg++; return ERROR; };
void zz_sendObjectMenu (ZZ_Object *o) { o++; };
ZZ_Object *zz_findAdvance (ZZ_Object *o) { o++; };
ZZ_Object *zz_findWAdvance (ZZ_Object *o) { o++; };
void  zz_processWindowPhrase () { };
void refreshDisplay () { };
void zz_DREM (ZZ_Object *ob) { ob++; };
void zz_dterminate () { };
void displayNote (const char *t) { t++; };
void    display (LONG p) { p; };

#if     BIG_precision != 1
void    display (BIG p) { p; };
#endif

#if  ZZ_TAG
void    display (ZZ_TBIG &p) { p; };
#endif

void    display (double p) { p = 0.0; };
void    display (const char *p) { p++; };
void    display (char c) { c = '\0'; };
void    display (double xo, double xs, double yo, double ys, Long att,
                                                 int n, double *x, double *y) {
  xo = xs = yo = ys = 0.0; x = y = (double*) NULL;
  att += n;
};

void    display (Long att, int n, double *x, double *y) {
  att = n; x++; y++;
};
void    display (int n, double *x, double *y) {
  x += n; y += n;
};
void    startRegion (double xo, double xs, double yo, double ys) {
  xo = xs = yo = ys;
};
void    startRegion () { };
void    endRegion () { };

void    displaySegment (Long att, int n, double *x, double *y) {
  att +=n; x++; y++;
};

void    displaySegment (int n, double *x, double *y) {
  x += n; y += n;
};

void    startSegment (Long att) { att++; };
void    displayPoint (double x, double y) { x = y; };
void    endSegment () { };

#endif   // ZZ_MONHOST
