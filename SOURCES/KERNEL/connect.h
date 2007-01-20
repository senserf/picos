#include <sys/ioctl.h>
#include <termios.h>
#include "../LIB/socklib.h"

#define SYSv	1

#define TTYB    struct  termios
#define TGET    TCGETA
#define TSET    TCSETAF

static void setTTYRaw (int desc, int speed, int parity, int stopb) {

	TTYB    NewTTYStat;

	if (tcgetattr (desc, &NewTTYStat) < 0)
		excptn ("Mailbox->connect: cannot get tty status (tcgetattr)");

	cdebug ("SysV RAW");

	NewTTYStat.c_iflag |= IGNBRK | IGNPAR;
	NewTTYStat.c_iflag &=
                            ~ISTRIP & ~INLCR & ~IGNCR & ~ICRNL & ~IXON & ~IXOFF;

	NewTTYStat.c_lflag &=
                            ~ECHO & ~ECHOE & ~ECHOK & ~ECHONL & ~ICANON & ~ISIG;
	NewTTYStat.c_oflag &= ~OPOST;

	NewTTYStat.c_cflag &= ~PARENB;
        if (parity) {
	  NewTTYStat.c_cflag &= ~CSIZE;
	  NewTTYStat.c_cflag |= CS7;
	  NewTTYStat.c_cflag |= PARENB;
	  NewTTYStat.c_cflag &= ~PARODD;
          if (parity % 2) NewTTYStat.c_cflag |= PARODD;
        }
        if (stopb) {
	  NewTTYStat.c_cflag &= ~CSTOPB;
          if (stopb > 1) NewTTYStat.c_cflag |= CSTOPB;
        }
	NewTTYStat.c_cc [VEOF] = 1;     // Immediately read any single char

        switch (speed) {
          case 50:
            cfsetospeed (&NewTTYStat, B50);
            cfsetispeed (&NewTTYStat, B50);
          case 0:
            break;
          case 75:
            cfsetospeed (&NewTTYStat, B75);
            cfsetispeed (&NewTTYStat, B75);
            break;
          case 110:
            cfsetospeed (&NewTTYStat, B110);
            cfsetispeed (&NewTTYStat, B110);
            break;
          case 134:
            cfsetospeed (&NewTTYStat, B134);
            cfsetispeed (&NewTTYStat, B134);
            break;
          case 150:
            cfsetospeed (&NewTTYStat, B150);
            cfsetispeed (&NewTTYStat, B150);
            break;
          case 200:
            cfsetospeed (&NewTTYStat, B200);
            cfsetispeed (&NewTTYStat, B200);
            break;
          case 300:
            cfsetospeed (&NewTTYStat, B300);
            cfsetispeed (&NewTTYStat, B300);
            break;
          case 600:
            cfsetospeed (&NewTTYStat, B600);
            cfsetispeed (&NewTTYStat, B600);
            break;
          case 1200:
            cfsetospeed (&NewTTYStat, B1200);
            cfsetispeed (&NewTTYStat, B1200);
            break;
          case 1800:
            cfsetospeed (&NewTTYStat, B1800);
            cfsetispeed (&NewTTYStat, B1800);
            break;
          case 2400:
            cfsetospeed (&NewTTYStat, B2400);
            cfsetispeed (&NewTTYStat, B2400);
            break;
          case 4800:
            cfsetospeed (&NewTTYStat, B4800);
            cfsetispeed (&NewTTYStat, B4800);
            break;
          case 9600:
            cfsetospeed (&NewTTYStat, B9600);
            cfsetispeed (&NewTTYStat, B9600);
            break;
          case 19200:
            cfsetospeed (&NewTTYStat, B19200);
            cfsetispeed (&NewTTYStat, B19200);
            break;
          case 38400:
            cfsetospeed (&NewTTYStat, B38400);
            cfsetispeed (&NewTTYStat, B38400);
            break;
#ifdef	B57600
          case 57600:
            cfsetospeed (&NewTTYStat, B57600);
            cfsetispeed (&NewTTYStat, B57600);
            break;
#endif
#ifdef	B115200
          case 115200:
            cfsetospeed (&NewTTYStat, B115200);
            cfsetispeed (&NewTTYStat, B115200);
            break;
#endif
#ifdef	B230400
          case 230400:
            cfsetospeed (&NewTTYStat, B230400);
            cfsetispeed (&NewTTYStat, B230400);
            break;
#endif
          default:
  	    excptn ("Mailbox->connect: illegal port speed %d", speed);
        }
	if (tcsetattr (desc, TCSANOW, &NewTTYStat) < 0)
		excptn ("Mailbox->connect: cannot set raw tty status (ioctl)");
};

void	Mailbox::destroy_bound () {
/*
 * This one disconnects the mailbox and destroys it altogether triggering all
 * possible events - to let everybody know that it has been gone.
 */
  	Mailbox *m, *q;

	if (JT == NULL) {
		// There is a socket/device to close
		::close (sfd);
		if (sfd != csd && csd != NONE)
			::close (csd);
	}
	if (ibuf != NULL)
		delete (ibuf);
	if (obuf != NULL)
		delete (obuf);

	ibuf = obuf = NULL;
    	count = iin = iout = oin = oout = 0;

	// Remove the mailbox from the socket list
	for (q = NULL, m = zz_socket_list; m != this && m != NULL; m = m->nexts)
      		q = m;

	Assert (m != NULL, "delete Mailbox: internal error, socket not found");
	if (q == NULL)
		zz_socket_list = nexts;
	else
		q->nexts = nexts;

	trigger_all_events ();
};

void	Mailbox::setUnblocking (int which) {
  if ((skt & ZZ_MTYPEMASK) == DEVICE) {
    fcntl (csd, F_SETFL, (fcntl(csd, F_GETFL) | O_NONBLOCK));
  } else
    setSocketUnblocking (which ? csd : sfd);
};

void	Mailbox::setBlocking (int which) {
  if ((skt & ZZ_MTYPEMASK) == DEVICE) {
    fcntl (csd, F_SETFL, (fcntl(csd, F_GETFL) | ~(O_NONBLOCK)));
  } else
    setSocketBlocking (which ? csd : sfd);
};

void	Mailbox::doClose (int which) {
  if (JT != NULL) return;  // Do nothing for a journaled mailbox
  if ((skt & ZZ_MTYPEMASK) == DEVICE) {
    close (csd);
  } else
    closeSocket (which ? csd : sfd);
  count = iin = iout = oin = oout = 0;
};

int    Mailbox::connect (int st, const char *hostname, int port, int bsize,
                                            int speed, int parity, int stopb) {
/* --------------------------------------------- */
/* Associate the mailbox with a socket or device */
/* --------------------------------------------- */

  Mailbox *m, *q;
  int k;

  findJournals ();

  Assert (sfd == NONE, "Mailbox->connect: %s, already connected", getSName ());
  Assert (limit >= 0, "Mailbox->connect: %s, cannot connect a barrier mailbox",
	getSName ());

  if (bsize == 0) bsize = limit;

  // Check if the mailbox is empty and nobody is waiting on it at the time
  Assert (count == 0, "Mailbox->connect: %s, the mailbox must be empty",
	getSName ());
  Assert (WList == NULL,
    "Mailbox->connect: %s, the mailbox is busy with a process waiting",
	getSName ());

  if (st == NONE) {
    // We are receiving a connection
    m = (Mailbox*) hostname;
    // If they don't find you handsome, they'll at least find you handy
    Assert (m->isConnected () && ((m->skt) & MASTER),
       "Mailbox->connect: %s, argument mailbox is not a connected MASTER "
	   "mailbox", getSName ());
    if ((csd = sfd = (int) (m->obuf)) == NONE) return REJECTED;
    m->obuf = (char*) NONE;
    skt = m->skt & ~(MASTER | SERVER);
    if (JT != NULL) {
      // The mailbox is to be read from journal
      Assert (csd == JOURNAL, "Mailbox->connect: %s, journaled mailbox "
	"connected to a non-journaled one", getSName ());
      if (JT->skipConnectBlock () != OK) return REJECTED;
    } else {
      Assert (csd != JOURNAL,
         "Mailbox->connect: %s, non-journaled mailbox connected to a journaled "
	     "one", getSName ());
      setUnblocking ();
    }
    // Skip all further checks
    goto SetBuffer;
  }

  skt = st;  // Save type flags

  if ((skt & ZZ_MTYPEMASK) == DEVICE) {
    Assert ((skt & SERVER) == 0,
      "Mailbox->connect: %s, DEVICE and SERVER are illegal together",
	getSName ());
  }

  if (JT != NULL) {
    if (JT->skipConnectBlock () != OK) return REJECTED;
    // This mailbox will be read from a journal and its output will be
    // ignored -- we do no further checks
    csd = sfd = JOURNAL;
    // This is higher than any descriptor
  } else if ((skt & ZZ_MTYPEMASK) == DEVICE) {
    // We have a device mailbox
    if ((skt & WRITE) && (skt & READ))
      k = O_RDWR | O_CREAT;
    else if (skt & READ)
      k = O_RDONLY;
    else if (skt & WRITE)
      k = O_WRONLY | O_CREAT;
    else
      excptn ("Mailbox->connect: device mailbox must be READ and/or WRITE");
    sfd = open (hostname, k, 0666);
    if ((skt & READ) == 0) ftruncate (sfd, 0);

    if (sfd < 0) return ERROR;

    if (skt & RAW) {
      // This is going to be a raw serial device
      setTTYRaw (sfd, speed, parity, stopb);
    }
    csd = sfd;
    setUnblocking ();

  } else if ((skt & SERVER) == 0) {

    Assert ((skt & (MASTER | WAIT)) == 0,
      "Mailbox->connect: %s, MASTER or WAIT can only occur with SERVER", 
	getSName ());

    if ((skt & ZZ_MTYPEMASK) == INTERNET)
      sfd = openClientSocket (hostname, port);
    else
      sfd = openClientSocket (hostname);
    if (sfd < 0) return ERROR;
    csd = sfd;  // These two are the same
    setUnblocking ();

  } else {

    // We are the server
    csd = NONE;  // Nobody connected yet
    if ((skt & ZZ_MTYPEMASK) == INTERNET)
      sfd = openServerSocket (port);
    else
      sfd = openServerSocket (hostname);
    if (sfd < 0) return ERROR;

    if (skt & WAIT) {
      // Wait for a party to connect to us
      Assert ((skt & MASTER) == 0,
        "Mailbox->connect: %s, MASTER illegal with WAIT", getSName ());
      setBlocking ();
      csd = getSocketConnection (sfd);
      if (csd < 0) return REJECTED;
    }
    setUnblocking ();
  }

SetBuffer:

  iin = iout = oin = oout = 0;
  count = 0;
  snt = '\0';
  // Note: count applies to the input end of the socket
  oldlimit = limit;
  limit = bsize;

  if (skt & MASTER) {
    // We don't need the buffers -- the mailbox will only be used for
    // accepting connections
    ibuf = NULL;
    obuf = (char*)NONE;
    // ibuf == NULL means that it is a MASTER mailbox, but obuf will be
    // used to store the decriptor of a new incoming connection, so don't
    // rely on its being NULL.
  } else {
    Assert (bsize > 0, "Mailbox->connect: %s, buffer size can't be zero",
	getSName ());
    ibuf = new char [limit +1];  // These are circular buffers
    obuf = new char [limit +1];
  }

  for (q = NULL, m = zz_socket_list; m != NULL; m = m->nexts) q = m;
  // Add the mailbox to the socket list. Mailboxes fed from journals are
  // also kept there. But for them, the processing will be different.
  if (q == NULL)
    zz_socket_list = this;
  else
    q->nexts = this;
  nexts = NULL;

  if (JO != NULL) JO->connectBlock ();
  return ACCEPTED;
};

int Mailbox::resize (int bsize) {
/*
 * Resizes the mailbox buffer in the direction UP.
 * The buffer need not be empty. If the new size is <= old size, nothing
 * happens. The method returns the old size.
 */
	char *nibuf, *nobuf;
	int ni, no, where;

	if (sfd == NONE)
		excptn ("Mailbox->resize: the mailbox is not connected");

	if (ibuf == NULL)
		excptn ("Mailbox->resize: illegal on master socket");

	if (bsize <= limit)
		// Do nothing
		return limit;

	nibuf = new char [bsize + 1];
	nobuf = new char [bsize + 1];

	where = 0;
	for (no = iout; no != iin; ) {
		nibuf [where++] = ibuf [no];
		if (++no > limit)
			no = 0;
	}
	iout = 0;
	iin = where;

	// Now the other buffer

	where = 0;
	for (no = oout; no != oin; ) {
		nobuf [where++] = obuf [no];
		if (++no > limit)
			no = 0;
	}
	oout = 0;
	oin = where;

	delete ibuf;
	delete obuf;

	ibuf = nibuf;
	obuf = nobuf;

	ni = limit;
	limit = bsize;

	return ni;
}

int	Mailbox::disconnect (int who) {

/* ---------------------------------------- */
/* Disassociate the mailbox from the socket */
/* ---------------------------------------- */

  Mailbox *m, *q;
  ZZ_REQUEST *rq;
  ZZ_EVENT *e;
#if ZZ_TAG
  int qq;
#endif
  Boolean jrnl;

  if (sfd == NONE) return OK;          // Already disconnected

  // Client disconnection for a mailbox read from a journal
  jrnl = (JT != NULL && who != SERVER && (skt & SERVER));

  if (who == CLEAR) {
    // Trigger NEWITEM (UPDATE) and OUTPUT
    for (rq = WList; rq != NULL; rq = rq -> next) {
      if (rq->event_id == NEWITEM || rq->event_id == OUTPUT ||
                                rq->event_id == SENTINEL || rq->event_id > 0) {
        rq -> Info01 = (void*)0;
#if     ZZ_TAG
        rq -> when . set (Time);
        if ((qq = (e = rq->event) -> waketime .
              cmp (rq->when)) > 0 || (qq == 0 && FLIP))
#else
        rq -> when = Time;
        if ((e = rq->event) -> waketime > Time || FLIP)
#endif
           e -> new_top_request (rq);
      }
    }
    if (jrnl)
	// This means a server mailbox, client disconnection, and journal
	return OK;
    count = iin = iout = oin = oout = 0;
  } else {
    if (WList != NULL) return ERROR;   // Other disconnect illegal
  }

  // Ignore if master & client disconnection
  if (ibuf == NULL && who != SERVER) return ERROR;

  if (count > 0) return ERROR;         // Pending incoming stuff

  if (oin != oout) return ERROR;       // Pending outgoing stuff

  if (jrnl) return OK;

  doClose (YES);
  csd = NONE;

  if (who == SERVER || (skt & SERVER) == 0) {
    doClose ();
    sfd = NONE;
    if (ibuf != NULL) {
      // MASTER sockets don't have buffers. Note the obuff can't be tested
      // against NULL because it contains the decriptor of the incoming
      // connection (for a MASTER socket, that is)
      delete (ibuf);
      delete (obuf);
      ibuf = NULL;
      obuf = NULL;
    }
    limit = oldlimit;
    // Remove the mailbox from the socket list
    for (q = NULL, m = zz_socket_list; m != this && m != NULL; m = m->nexts)
      q = m;
    Assert (m != NULL, "Mailbox->disconnect: internal error, socket not found");
    if (q == NULL)
      zz_socket_list = nexts;
    else
      q->nexts = nexts;
  }
  JT = JO = NULL;   // We will restore them upon connect
  return OK;
};

int	Mailbox::isConnected () {
/* ------------------------- */
/* Tell whether we are bound */
/* ------------------------- */
  return sfd != NONE;
};

int	Mailbox::isActive () {
/* ------------------------------------------------ */
/* Tell whether we are bound and connected anywhere */
/* ------------------------------------------------ */
  return sfd != NONE && csd != NONE;
};

int	Mailbox::rawWrite (const char *buf, int length) {

/* ------------------------------ */
/* Write characters to the socket */
/* ------------------------------ */

  int k;

  Assert (sfd != NONE, "Mailbox->rawWrite: %s, the mailbox is disconnected",
	getSName ());
  if (csd == NONE && (skt & SERVER)) {
    // This is a non-MASTER server socket to which no one has connected yet
    csd = getSocketConnection (sfd);
    if (csd < 0) return (csd = NONE);       // Better luck next time
    setUnblocking (YES);
  }
  if (csd == JOURNAL) {
    // Write is void -- the operation always succeeds
    k = length;
  } else {
    k = writeSocket (csd, buf, length);
    if (k < 0) {
      if (errno != EAGAIN && count == 0) {
        // Our party has disconnected
        disconnect (CLEAR);
      }
    }
  }
  if (JO != NULL && k > 0) JO->writeData (buf, k, 'O');
  return k;
}

int     Mailbox::rawRead (char *buf, int length) {

/* ------------------------------- */
/* Read characters from the socket */
/* ------------------------------- */

  int k, dsc;

  Assert (sfd != NONE, "Mailbox->rawRead: %s, the mailbox is disconnected",
	getSName ());
  if (csd == NONE && (skt & SERVER)) {
    // This is a non-MASTER server socket to which no one has connected
    csd = getSocketConnection (sfd);
    if (csd < 0) return (csd = NONE);       // Better luck next time
    setUnblocking (YES);
  }

  if (csd == JOURNAL) {
    k = JT->readData (buf, length);
    dsc = k < 0;
  } else {
    k = readSocket (csd, buf, length);
    dsc = (k < 0 && errno != EAGAIN) || k == 0;
  }

  if (dsc && (iin == iout)) {  // Error - forced disconnection
    // Ignore read errors if this is a device opened for writing only
    if ((skt & DEVICE) == 0 || (skt & READ) != 0) {
      // Our party has disconnected and the buffer is empty
      // trace ("Disconnect"); Ouf.flush();
      disconnect (CLEAR);
      csd = NONE;
    }
  }

  if (JO != NULL && k > 0) JO->writeData (buf, k, 'I');
  return k;
}

Long	Mailbox::outputPending () {

/* ------------------------------------------------------------ */
/* Returns the number of outstanding bytes in the output buffer */
/* ------------------------------------------------------------ */

	if (oin == oout)
		return 0;
	else if (oin > oout)
		return oin - oout;
	else
		return limit - oout + oin + 1;
}

void	Mailbox::flushOutput () {

/* ------------------------------------------- */
/* Empty the output buffer of a socket mailbox */
/* ------------------------------------------- */

  int term, nw;
  ZZ_REQUEST *rq;
  ZZ_EVENT *e;
#if ZZ_TAG
  int q;
#endif

  // This also works for MASTER mailboxes -- does nothing
  if (oin == oout) return;
  // There is some stuff in the output buffer
  term = (oin > oout) ? oin : limit+1;  // The boundary of the first portion
  nw = rawWrite (obuf + oout, term - oout);
  if (nw <= 0) // Nothing written -- leave things intact
    return;
  // Something has been written -- check how much
  oout += nw;
  if (oout > limit) {
    oout = 0;
    if (oin > 0) {
      // There is a second part
      nw = rawWrite (obuf, oin);
      if (nw > 0) oout += nw;
    }
  }
  // Some stuff has been removed from the buffer. Restart processes waiting
  // for OUTPUT
  for (rq = WList; rq != NULL; rq = rq->next) {
    if (rq->event_id == OUTPUT) {
#if ZZ_TAG
      rq -> when . set (Time);
      if ((q = (e = rq->event) -> waketime . cmp (rq->when)) > 0 ||
        (q == 0 && FLIP))
#else
      rq -> when = Time;
      if ((e = rq->event) -> waketime > Time || FLIP)
#endif
        e -> new_top_request (rq);
    }
  }
};

void	Mailbox::readInput () {

/* ----------------------------------------- */
/* Fill the input buffer of a socket mailbox */
/* ----------------------------------------- */

  int br, nw, SentinelFound;
  char cbuf [4];
  ZZ_REQUEST *rq;
  ZZ_EVENT   *e;

#if ZZ_TAG
        int qq;
#endif
  if (ibuf == NULL) {
    // This is a master socket -- check for incoming connection request
    if (obuf == (void*)NONE) {
      // If not pending already
      if (csd == JOURNAL) {
        // Reading from journal
        br = JT->readData (cbuf, 4);
        if (br <= 0) return;  // Not yet
        // Sanity check
        Assert (br == 4 && strncmp (cbuf, CONNECTMAGIC, 4) == 0,
          "Mailbox->readInput: %s, journal data wrong for MASTER mailbox",
		getSName ());
        // This should be larger than a file descriptor -- just in case
        obuf = (char*) JOURNAL;
        // Update new journal
        if (JO != NULL) JO->writeData (CONNECTMAGIC, 4, 'I');
      } else {
        // Try "accept"
        if ((nw = getSocketConnection (sfd)) < 0) return;
        obuf = (char*) nw;
        // Update new journal
        if (JO != NULL) JO->writeData (CONNECTMAGIC, 4, 'I');
      }
    }
    if (obuf != (void*)NONE) {
      // Restart processes waiting for this event
      for (rq = WList; rq != NULL; rq = rq -> next) {
        // This is also update
        if (rq->event_id == NEWITEM) {
          rq -> Info01 = (void*) obuf;
#if     ZZ_TAG
          rq -> when . set (Time);
          if ((qq = (e = rq->event) -> waketime .
                cmp (rq->when)) > 0 || (qq == 0 && FLIP))
#else
          rq -> when = Time;
          if ((e = rq->event) -> waketime > Time || FLIP)
#endif
             e -> new_top_request (rq);
        }
      }
    }
    return;
  }

  // Process the usual reading

  if (iin >= iout) {
    br = limit + 1 - iin;   // How much we can try to read
    if (iout == 0) br--;    // We can't let iin and iout meet
    if (br <= 0) return;    // Don't bother
    if ((br = rawRead (ibuf+iin, br)) <= 0) return;   // Nothing read
    if ((iin += br) > limit) iin = 0;
  } else {
    br = 0;
  }
  if (iin < iout-1) {
    // There may be a second part
    if ((nw = rawRead (ibuf+iin, iout - iin - 1)) > 0) {
      br += nw;
      iin += nw;
    }
  }
  if (br) {
    // We get here if anything has been read at all. We should now
    // determine the new element count.
    br = (iin >= iout) ? iin - iout : limit + 1 - iout + iin;
    if (br != count) {
      assert (br > count, "Mailbox->readInput: %s, inconsistent count update",
	getSName ());
      // We may have to trigger some events
      count = br;
      SentinelFound = -1;
      for (rq = WList; rq != NULL; rq = rq -> next) {
        switch (rq->event_id) {
          case NEWITEM:
          case NONEMPTY:
          case RECEIVE:
            rq -> Info01 = (void*) ibuf [iout];
            break;
          case SENTINEL:
            if (count == limit) {
              rq -> Info01 = (void*) 0;
              break;
            } else {
              if (SentinelFound < 0) SentinelFound = sentinelFound ();
              if (SentinelFound) {
                rq -> Info01 = (void*) SentinelFound;
                break;
              }
            }
            continue;  // This applies to the loop (or so I hope)
          default:
            if (rq->event_id < 0 || rq->event_id > count) continue;
            rq -> Info01 = (void*) ibuf [iout];
        }
#if     ZZ_TAG
        rq -> when . set (Time);
        if ((qq = (e = rq->event) -> waketime .
              cmp (rq->when)) > 0 || (qq == 0 && FLIP))
#else
        rq -> when = Time;
        if ((e = rq->event) -> waketime > Time || FLIP)
#endif
           e -> new_top_request (rq);
      }
    }
  }
};

int Mailbox::isWaiting () {

/* --------------------------------------------------- */
/* Checks whether there is an outstanding wait request */
/* --------------------------------------------------- */

  int fd;

  if (WList != NULL) {
    if ((fd = csd) == NONE) fd = sfd;  // Use master descriptor if disconnected
    return (fd == JOURNAL) ? -1 : fd;  // Fed from a journal
  } else
    return -1;
};

int Mailbox::sentinelFound () {
/* ----------------------------------------------------------------------- */
/* Check if the sentinel is present, return the number of bytes before and */
/* including the sentinel.                                                 */
/* ----------------------------------------------------------------------- */

  register int ip, ic;

  if (csd != NONE) {
    // We don't fail for a disconnected mailbox -- the read will fail (softly)
    for (ip = iout, ic = count; ic; ic--, ip++) {
      if (ip > limit) ip = 0;
      if (ibuf [ip] == snt) return count - ic + 1;
    }
  }
  return 0;
};

int Mailbox::read (char *b, int nc) {

/* --------- */
/* Block get */
/* --------- */

  int xc;

  if (sfd == NONE) return ERROR;
  Assert (nc > 0 && nc <= limit,
    "Mailbox->read: %s, number of bytes (%1d) must be > 0 and <= buffer size"
	" (limit)", getSName (), nc);
  if (count < nc) return REJECTED;
  if (limit+1 - iout >= nc) {
    bcopy ((BCPCASTS)(ibuf + iout), (BCPCASTD)b, nc);
    if ((iout += nc) > limit) iout = 0;
  } else {
    bcopy ((BCPCASTS)(ibuf + iout), (BCPCASTD)b, xc = limit-iout+1);
    bcopy ((BCPCASTS)ibuf, (BCPCASTD)(b + xc), nc - xc);
    iout = iout + nc - limit - 1;
  }
  count -= nc;
  return ACCEPTED;
};

int Mailbox::readToSentinel (char *b, int nc) {

/* ----------------------------------------- */
/* Block get to sentinel (sentinel included) */
/* ----------------------------------------- */

  int ip, ic, fc, fp;
  char xc;

  if (sfd == NONE) return ERROR;
  Assert (nc > 0, "Mailbox->readToSentinel: %s, number of bytes must be > 0",
	getSName ());
  Info01 = (void*)((int)NO);
  for (ip = iout, ic = 0, fc = 0; fc < count; fc++) {
    if (ip > limit) ip = 0;
    xc = ibuf [ip++];
    if (ic < nc) {
      b [ic++] = xc;
      fp = ip;
    }
    if (xc == snt) {
      Info01 = (void*)((int)YES);
      goto SF;
    }
  }
  // The sentinel hasn't been found
  if (count < limit) {
    // The mailbox is not full
    return 0;  // Failure
  }
SF:
  // Remove the read portion from the buffer
  iout = (fp > limit) ? 0 : fp;
  count -= ic;
  return ic;
};

int Mailbox::readAvailable (char *b, int nc) {

/* ------------------- */
/* Block get available */
/* ------------------- */

  int ic;

  if (sfd == NONE) return ERROR;
  Assert (nc > 0, "Mailbox->readAvailable: %s, number of bytes must be > 0",
	getSName ());
  for (ic = 0; count && ic < nc; count--, ic++) {
    *(b++) = ibuf [iout++];
    if (iout > limit) iout = 0;
  }
  return ic;
};
      
int Mailbox::write (const char *b, int nc) {

/* --------- */
/* Block put */
/* --------- */

  int free;

  if (sfd == NONE) return ERROR;

  Assert (nc > 0 && nc <= limit,
    "Mailbox->write: %s, number of bytes (%1d) must be > 0 and <= limit",
	getSName (), nc);
  // Determine the amount of free space in the buffer

  free = (oin < oout) ? oout - oin - 1 : limit - oin + oout;

  if (free < nc) return REJECTED;

  if (limit+1 - oin >= nc) {
    bcopy ((BCPCASTS)b, (BCPCASTD)(obuf + oin), nc);
    if ((oin += nc) > limit) oin = 0;
  } else {
    bcopy ((BCPCASTS)b, (BCPCASTD)(obuf + oin), free = limit-oin+1);
    bcopy ((BCPCASTS)(b + free), (BCPCASTD)obuf, nc - free);
    oin = oin + nc - limit - 1;
  }
  return ACCEPTED;
};

// --------------------- //
// Journalling functions //
// --------------------- //

static void jfname (const char *F, char *FN, char TP) {
  // Builds a filename out of the mailbox name
  int i, nc;
  char c;
  nc = strlen (F);
  strncpy (FN, F, nc);
  for (i = 0; i < nc; i++) {
    // Make all characters look decent
    c = FN [i];
    if ((c > 'z' || c < 'a') && (c > 'Z' || c < 'A') && (c > '9' || c < '0'))
      FN [i] = '_';
  }
  // Add the suffix
  FN [nc++] = '.';
  FN [nc++] = 'j';
  FN [nc++] = 'n';
  FN [nc++] = (TP == 'J') ? 'j' : 'x';
  FN [nc  ] = '\0';
};

static Long inline extrLong (const char *t) {
  // Extract integer number in network format
  return  ((int)(unsigned char)(*(t+3))) |
         (((int)(unsigned char)(*(t+2))) <<  8) |
         (((int)(unsigned char)(*(t+1))) << 16) |
         (((int)(unsigned char)(*(t  ))) << 24);
};

static Long inline insrLong (char *t, Long n) {
  // Extract integer number in network format
  *(t+3) = (char) ((n      ) & 0xff);
  *(t+2) = (char) ((n >>  8) & 0xff);
  *(t+1) = (char) ((n >> 16) & 0xff);
  *(t  ) = (char) ((n >> 24) & 0xff);
};

ZZ_Journal::ZZ_Journal (const char *F, char Type) {
  // Create a new journal structure

  next = ZZ_Journals;
  ZZ_Journals = this;
  LeftInBlock = 0;
  FD = NONE;

  JName = new char [strlen (F) + 1];
  strcpy (JName, F);
  // Mailbox not connected yet
  MB = NULL;
  JType = Type;
};

int ZZ_Journal::openFile () {
  // Opens the journal
  char *FN, *MSG;
  char tmp [JHEADERLENGTH];
  FN = new char [strlen (JName) + 5];
  // Build the file name
  jfname (JName, FN, JType);
  if (JType == 'J') {
    // New journal
    if ((FD = open (FN, O_WRONLY | O_CREAT | O_EXCL, 0777)) < 0) {
      // The file cannot exist
      excptn ("Journal->openFile: can't open '%s' (the file mustn't exist!)",
	FN);
    }
  } else {
    // Old journal -- now the file must exist
    if ((FD = open (FN, O_RDONLY, 0777)) < 0) {
      excptn ("Journal->openFile: can't open '%s' (the file must exist!)", FN);
    }
  }
  // Deallocate file name
  delete FN;
  if (JType == 'J') {
    // Write journal header
    strcpy (tmp, JOURNALMAGIC);
    WhenSec = getEffectiveTimeOfDay ();
    insrLong (tmp + JHTIMEOFFSET, WhenSec);
    if (writeBlock (tmp, JHEADERLENGTH) == ERROR)
      excptn ("Journal->openFile: write error in '%s'", JName);
  } else {
    // Verify journal header
    if (readBlock (tmp, JHEADERLENGTH) != OK ||
                           strncmp (tmp, JOURNALMAGIC, strlen (JOURNALMAGIC))) {
      excptn ("Journal->openFile: header error in '%s'", JName);
    }
    WhenSec = extrLong (tmp+JHTIMEOFFSET);
  }
};

int ZZ_Journal::readBlock (char *S, int Len) {
  // Reads Len bytes from journal
  int k;
  while (Len) {
    if ((k = ::read (FD, S, Len)) == Len) return OK;
    if (k < 0) return ERROR;
    if (k == 0) return NONE;
    S += k;
    Len -= k;
  }
  return OK;
};

void ZZ_Journal::connectBlock () {
  // Write a new connection block
  char block [JBHEADERLENGTH];
  Long csec, cusec;
  int nc;
  strcpy (block, BLOCKMAGIC);
  nc = strlen (BLOCKMAGIC);
  csec = getEffectiveTimeOfDay (&cusec);
  insrLong (block + nc, csec);
  insrLong (block + nc + 4, cusec);
  nc += 8;
  if (writeBlock (block, nc) == ERROR)
      excptn ("Journal->connectBlock: write error in '%s'", JName);
};

int ZZ_Journal::skipConnectBlock () {
  char block [JBUFSIZE];
  int Len, Stat;
  // Note: if LeftInBlock == 0, it means that the last block has been
  // exhausted. LeftInBlock == NONE means that we are at EOF or at the
  // beginning of the next "connect" block.
  if (LeftInBlock < 0) {
    // We are at a connect block or EOF
    if (LeftInBlock == NONE) {
      // We are at the beginning of a new connect block
      LeftInBlock = 0;
      return OK;
    } else
      // EOF -- fail
      return ERROR;
  }
  // Ideally, if things are in sync, we are now at the connect block header,
  // but just in case, let's try to get rid of any garbage. Note that this
  // code doesn't have to be very efficient, as it will be executed seldom
  // (if ever).
  while (1) {
    if (LeftInBlock)
      Len = (LeftInBlock > JBUFSIZE) ? JBUFSIZE : LeftInBlock;
    else
      Len = JBHEADERLENGTH;
    if ((Stat = readBlock (block, Len)) == ERROR)
      excptn ("Journal->skipConnectBlock: read error in '%s'", JName);
    else if (Stat == NONE) {
      // We are at EOF
      return (LeftInBlock = ERROR);
    } else {
      // A successful read
      if (LeftInBlock)
        LeftInBlock -= Len;
      else {
        // We have read a header
        if (strncmp (BLOCKMAGIC, block, strlen (BLOCKMAGIC)) == 0)
          // OK - note that LeftInBlock == 0 at the moment
          return OK;
        Assert (block [0] == 'I' || block [0] == 'O',
               "Journal->skipConnectBlock: format error in '%s'", JName);
        LeftInBlock = extrLong (block + JBLENGTHOFFSET);
      }
    }
  }
};

int ZZ_Journal::readData (char *S, int Len) {
  char block [JBUFSIZE];
  int k, len, slen, stt;
  Long msec, csecn, cusec;
  for (k = 0; Len; ) {
    if (LeftInBlock > 0) {
      csecn = getEffectiveTimeOfDay (&cusec);
      if (WhenSec > csecn || (WhenSec == csecn && WhenUSec > cusec)) {
        // Determine the delay until the next journal event
        msec = (WhenSec - csecn) * 1000 + (WhenUSec - cusec) / 1000;
        if (msec < zz_max_sleep_time) zz_max_sleep_time = msec;
        return k;
      }

      len = (LeftInBlock > Len) ? Len : LeftInBlock;
      if ((stt = readBlock (S, len)) == ERROR)
                excptn ("Journal->readData: read error in '%s'", JName);
      if (stt == NONE)
        // This cannot happen
            excptn ("Journal->readData: unexpected EOF in '%s'", JName);
      LeftInBlock -= len;
      Len -= len;
      S += len;
      k += len;
    } else if (LeftInBlock == 0) {
      // Try to read the next block
      while (1) {
        // May need to skip some of them
        if ((stt = readBlock (block, JBHEADERLENGTH)) == ERROR)
                 excptn ("Journal->readData: read error in '%s'", JName);
        if (stt == NONE) {
          // This time EOF is legal
          LeftInBlock = ERROR;
          return k;
        }
        if (block [0] == 'N') {
          // This is a connection block
          LeftInBlock = NONE;
          return k;
        }
        LeftInBlock = extrLong (block + JBLENGTHOFFSET);
        if (block [0] == JType) break;
        // Must skip this block
        while (LeftInBlock) {
          slen = (LeftInBlock > JBUFSIZE) ? JBUFSIZE : LeftInBlock;
          if ((stt = readBlock (block, slen)) == ERROR)
                 excptn ("Journal->readData: read error in '%s'", JName);
          if (stt == NONE)
             excptn ("Journal->readData: unexpected EOF in '%s'", JName);
          LeftInBlock -= slen;
        }
      }
      // Determine the time of this block
      WhenSec = extrLong (block + JBTIMEOFFSET);
      WhenUSec = extrLong (block + JBTIMEOFFSET + 4);
    } else {
      // LeftInBlock == NONE or ERROR
      return ERROR;
    }
  }
  return k;
};

void ZZ_Journal::writeData (const char *S, int Len, char tp) {
  char block [JBUFSIZE];
  Long sec, usec;
  block [0] = tp;
  sec = getEffectiveTimeOfDay (&usec);
  insrLong (block + JBTIMEOFFSET, sec);
  insrLong (block + JBTIMEOFFSET + 4, usec);
  insrLong (block + JBLENGTHOFFSET, Len);
  if (Len <= JBUFSIZE - JBHEADERLENGTH) {
    bcopy ((BCPCASTS)S, (BCPCASTD)(block+JBHEADERLENGTH), Len);
    if (writeBlock (block, JBHEADERLENGTH+Len) == ERROR)
               excptn ("Journal->writeData: write error in '%s'", JName);
  } else {
    // Do it in two steps
    if (writeBlock (block, JBHEADERLENGTH) == ERROR)
               excptn ("Journal->writeData: write error in '%s'", JName);
    if (writeBlock (S, Len) == ERROR)
               excptn ("Journal->writeData: write error in '%s'", JName);
  }
};
  
int ZZ_Journal::writeBlock (const char *S, int Len) {
  int k;
  while (Len) {
    if ((k = ::write (FD, S, Len)) == Len) return OK;
    if (k < 0) return ERROR;
    S += k;
    Len -= k;
  }
};
  
void Mailbox::findJournals () {
  ZZ_Journal *jn;
  JT = JO = NULL;
  for (jn = ZZ_Journals; jn != NULL; jn = jn -> next) {
    // The name should match either the mailbox's nickname or
    // standard name.
    if (strcmp (jn->JName, getNName ()) == 0 ||
        strcmp (jn->JName, getSName ()) == 0   ) {
      // We have a match
      Assert (jn->MB == NULL || jn->MB == this,
        "Mailbox->connect: %s, ambiguous journals %s", getSName (), jn->JName);
      jn->MB = this;
      // Determine journal type
      if (jn->JType == 'J') {
        Assert (JO == NULL, "Mailbox->connect: %s, ambiguous 'J' journals %s",
		getSName (), jn->JName);
        JO = jn;
      } else {
        Assert (JT == NULL, "Mailbox->connect: %s, ambiguous 'I/O' journals %s",
		getSName (), jn->JName);
        JT = jn;
      }
    }
  }
};
