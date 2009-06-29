#ifndef __agent_c__
#define __agent_c__

// External agent interface

#include "board.h"
#include "agent.h"
//#include "stdattr.h"

//#undef	rnd

#define	imode(f)	((f) & XTRN_IMODE_MASK)
#define	omode(f)	((f) & XTRN_OMODE_MASK)

word	ZZ_Agent_Port	= AGENT_SOCKET;

static MUpdates *MUP = NULL,	// To signal mobility updates
		*PUP = NULL;	// To signal panel updates

void zz_panel_signal (Long sid) {

	if (PUP)
		PUP->queue (sid);
}

inline static void skipblk (char *&bp) {
	while (isspace (*bp))
		bp++;
}

static void mup_update (Long n) {

	MUP->queue (n);
}

static int dechex (char *&bp) {
//
// Made global as being useful outside this file
//
	int res;

	skipblk (bp);

	res = 0;

	if (*bp == '0' && *(bp+1) == 'x') {
		bp += 2;
		if (!isxdigit (*bp))
			return ERROR;
		do {
			res = (res << 4) | unhex (*bp);
			bp++;
		} while (isxdigit (*bp));
	} else {
		// Assume this is decimal
		if (!isdigit (*bp))
			return ERROR;
		do {
			res = (res * 10) + (*bp) - '0';
			bp++;
		} while (isdigit (*bp));
	}

	return res;
}

int Dev::wl (int st, char *&buf, int &left) {
	// This is for writing stuff potentially longer than the
	// mailbox buffer
	int nc;
	Long bs;

	if ((bs = getLimit ()) == 0)
		// Disconnection
		return ERROR;

	while (left > 0) {
		nc = (left > bs) ? (int) bs : left;
		if (this->write (buf, nc) != ACCEPTED) {
			if (!isActive ())
				return ERROR;
			this->wait (OUTPUT, st);
			sleep;
		}
		left -= nc;
		buf += nc;
	}
	return OK;
}

int Dev::wi (int st, const char *buf, int nc) {
	// This one is for writing short stuff (certainly not longer
	// than the mailbox buffer)
	Long bs;
	if ((bs = getLimit ()) == 0)
		// Disconnection
		return ERROR;
	assert (nc <= bs, "Dev->w at %s: attempt to write %1d bytes, which is "
		"more than %1d", TheStation->getSName (), nc, bs);
	if (this->write (buf, nc) != ACCEPTED) {
		if (!isActive ())
			return ERROR;
		this->wait (OUTPUT, st);
		sleep;
	}
	return OK;
}

int Dev::ri (int st, char *buf, int nc) {
	if (this->read (buf, nc) != ACCEPTED) {
		if (!isActive ())
			return ERROR;
		this->wait (NEWITEM, st);
		sleep;
	}
	return OK;
}

int Dev::rs (int st, char *&buf, int &left) {

	int nk;

	while (1) {
		if ((nk = readToSentinel (buf, left)) == ERROR)
			return ERROR;

		if (nk == 0) {
			this->wait (SENTINEL, st);
			sleep;
		}

		left -= nk;
		buf += nk;

		// Check if the sentinel has been read
		if (TheEvent)
			// An alias for Info01, means that the
			// sentinel was found
			return OK;

		if (left == 0)
			return REJECTED;
	} 
}

process	AgentConnector {
/*
 * Started by AgentInterface to handle one incoming Agent connection. Its
 * role is to read the request block and spawn the proper process to handle
 * the service.
 */
	Dev 	*Agent;

	states { Init, GiveUp };

	void setup (Dev*);

	perform;
};

process Disconnector {

	Dev	*Agent;
	lword	code;

	states { Send, Wait, Kill };

	void setup (Dev *a, lword c) { Agent = a; code = htonl (c); };

	perform;
};

process	UartHandler {
/*
 * Handles a UART connection
 */
	Dev 		*Agent;
	UART		*UA;
	char		*Buf;
	int		Left;

	states { AckUart, UartFlush };

	void setup (PicOSNode*, Dev*);

	perform;
};

process ClockHandler {
/*
 * Clock display
 */
	Dev	*Agent;
	TIME	secdelay;

	inline lword secconv () {
		return (lword) (ituToEtu (Time) + 0.5);
	}

	states	{ AckClk, Send };

	void setup (Dev*);

	perform;
};

process PinsHandler {
/*
 * Handles a PINS module connection and acts as the driver
 */
	PicOSNode *TPN;
	Dev	  *Agent;
	PINS	  *PN;
	int	  Length;
	char	  *Buf;

	states { AckPins, Loop, Send };

	void setup (PINS*, Dev*);

	void goaway (lword);

	perform;
};

process PinsInput {
/*
 * A sidekick tp PinsHandler: handles the input end of the connection
 * (or device)
 */
	PINS      *PN;
	char      *BP;
	int       Left;
	char      RBuf [PRQS_INPUT_BUFLEN];

	states { Loop, ReadRq, Delay };

	void setup (PINS*);

	perform;
};

process PulseMonitor {

	PINS *PN;
	Long WCNT, OFFTime;
	Process **TH;
	byte ONV, PIN;

	states { Probe, WECyc, WNewCyc };

	void setup (
			PINS*,		// The module
			Process**,	// Thread pointer
			byte,		// The pin: 0-cnt, 1-not
			byte,		// Polarity 0-low, 1-high
			Long,		// Ontime (debouncer)
			Long		// Offtime (debouncer)
	);

	perform;
};

process SensorsHandler {
/*
 * Handles a SNSRS module connection and acts as the driver
 */
	PicOSNode *TPN;
	Dev	  *Agent;
	SNSRS	  *SN;
	int	  Length;
	char	  *Buf;

	states { AckSens, Loop, Send };

	void setup (SNSRS*, Dev*);

	void goaway (lword);

	perform;
};

process SensorsInput {
/*
 * A sidekick tp SensorsHandler: handles the input end of the connection
 * (or device)
 */
	SNSRS	  *SN;
	char      *BP;
	int       Left;
	char      RBuf [SRQS_INPUT_BUFLEN];

	states { Loop, ReadRq, Delay };

	void setup (SNSRS*);

	perform;
};

process LedsHandler {

	PicOSNode *TPN;
	Dev *Agent;
	LEDSM *LE;
	int Length;
	char *Buf;

	states { AckLeds, Loop, Send, SendNop };

	void setup (PicOSNode*, LEDSM*, Dev*);

	perform;
};

process LcdgHandler {

	PicOSNode *TPN;
	Dev *Agent;
	LCDG *LC;
	word *Buf, WNOP;
	int Length;

	states { AckLcdg, Loop, Send, SendNop };

	void setup (PicOSNode*, Dev*);

	perform;

};

UART::UART (data_ua_t *UAD) {
/*
 * Initialize the UART model. mode is sum of
 *
 *  XTRN_IMODE_NONE, XTRN_IMODE_DEVICE, XTRN_IMODE_SOCKET or XTRN_IMODE_STRING
 *  XTRN_OMODE_NONE, XTRN_OMODE_DEVICE, XTRN_OMODE_SOCKET
 *  XTRN_IMODE_TIMED or XTRN_IMODE_UNTIMED
 *  XTRN_IMODE_HEX or XTRN_IMODE_ASCII
 *  XTRN_OMODE_HEX or XTRN_OMODE_ASCII
 *  XTRN_OMODE_HOLD of XTRN_OMODE_NOHOLD
 *  input string length is orred into mode
 *
 *  IMODE_SOCKET requires OMODE_SOCKET (this is value zero, i.e., default)
 *  IMODE_DEVICE + OMODE_DEVICE (the two devices can, but need not have to,
 *  be the same)
 *
 *  For _SOCKET, idev == NULL
 *
 *  _HEX data: 12af565e4ffg .... (whitespace ignored on input)
 *  _ASCII means "raw"
 *  _TIMED (input only) [+]Time { ... } (Time is in ITUs)
 */
	char *sp, *fn;
	int st;

	I = O = NULL;
	Flags = UAD->UMode;
	PI = NULL;
	PO = NULL;
	// Account for the UART register on the micro, which always provides
	// a single byte of buffer space, one byte extra for the circular
	// organization of the buffer
	B_ilen = UAD->UIBSize + 2;
	B_olen = UAD->UOBSize + 2;
	TI_aux = NULL;
	TimedChunkTime = TIME_0;

	IBuf = new byte [B_ilen];
	OBuf = new byte [B_olen];

	// Account for start / stop bits, assuming there is no parity and one
	// stop bit, which is the case in all our setups

 	DefRate = Rate = UAD->URate;
	// Will be actually set by rst

	if (imode (Flags) == XTRN_IMODE_SOCKET ||
					omode (Flags) == XTRN_OMODE_SOCKET) {
		// The socket case: both ends must use the same socket
		Assert (imode (Flags) == XTRN_IMODE_SOCKET &&
		    omode (Flags) == XTRN_OMODE_SOCKET, 
			"UART at %s: confilcting modes %x",
			    TheStation->getSName (),
			        Flags);
		// Don't create the device now; it will be created upon
		// connection. That's it for now.
	} else if (imode (Flags) == XTRN_IMODE_DEVICE) {
		// Need a mailbox for the input end
		Assert (UAD->UIDev != NULL,
			"UART at %s: input device cannot be NULL",
				TheStation->getSName ());
		I = create Dev;
		// Check if the output end uses the same device
		if (omode (Flags) == XTRN_OMODE_DEVICE &&
		    strcmp (UAD->UIDev, UAD->UODev) == 0) {
			// Yep, single mailbox will do
			st = I->connect (DEVICE+READ+WRITE, UAD->UIDev, 0,
					XTRN_MBX_BUFLEN);
			O = I;
		} else {
			st = I->connect (DEVICE+READ, UAD->UIDev, 0,
				XTRN_MBX_BUFLEN);
		}
		if (st == ERROR)
			excptn ("UART at %s: cannot open device %s",
				TheStation->getSName (), UAD->UIDev);
	} else if (imode (Flags) == XTRN_IMODE_STRING) {
		// String
		Assert (UAD->UIDev != NULL,
			"UART at %s: the input string is empty",
				TheStation->getSName ());
#if 0
		// SLen is part of Flags
		SLen = (Flags & XTRN_IMODE_STRLEN);
		if (SLen == 0)
			// Use strlen
			SLen = strlen (UAD->UIDev);
		if (SLen == 0)
			excptn ("UART at %s: input string length is 0",
				TheStation->getSName ());
#endif
		// No need to copy the string; it has been allocated as
		// shared by readNodeParams
		S = UAD->UIDev;
	}

	// We are done with the input end
	if (O == NULL && omode (Flags) == XTRN_OMODE_DEVICE) {
		// This can only mean a separate DEVICE 
		Assert (UAD->UODev != NULL, "UART at %s: no output device name",
			TheStation->getSName ());
		O = create Dev;
		if (O->connect (DEVICE+WRITE, UAD->UODev, 0, XTRN_MBX_BUFLEN))
			excptn ("UART at %s: cannot open device %s",
			    	TheStation->getSName (), UAD->UODev);
	}

	rst ();
}

void UART::setRate (word rate) {
//
// Sets the current rate
//
	if (!zz_validate_uart_rate (rate))
		syserror (EREQPAR, "phys_uart rate");

	Rate = rate;

 	// Note: Rate has been divided by 100, so this amounts to multiplying
 	// seconds/truerate by 10.0
 	ByteTime = (TIME) ((Second / Rate) / 10.0);
}

void UART::rst () {

	// When a node is reset, SMURPH kills all its processes, including
	// UART drivers. 

	// Note that the input is not reset, should it be? I don't think so.
	// Event string input should not be used for post reset initialization
	// (preinit) is for that. So what is gone is gone.

	IB_in = IB_out = 0;
	// Note that if we are reading from string, and the string is gone,
	// the process will immediately terminate itself.
	PI = create UART_in (this);

	OB_in = OB_out = 0;
	PO = create UART_out (this);

	setRate (DefRate);
}

void UART_in::setup (UART *u) {

	U = u;
	TimeLastRead = TIME_0;
}

void UART_out::setup (UART *u) {

	U = u;
	TimeLastWritten = TIME_0;
}

static void ti_err () {
	excptn ("UART->getTimed at %s: input format error",
		TheStation->getSName ());
};

char UART::getOneByte (int st) {
/*
 * Fetches one byte (from the mailbox or string)
 */
	char c;

	if (imode (Flags) == XTRN_IMODE_STRING) {
		if ((Flags & XTRN_IMODE_STRLEN) == 0) {
			// Kill the input process
			terminate (PI);
			PI = NULL;
			S = NULL;
			sleep;
		}
		Flags--;
		return  *S++;
	}
	// Read from the mailbox
	if (I == NULL) {
		// This means that we are reading from a socket that
		// isn't there yet, or from a non-existent interface. In
		// both cases, wait for the event (in the second case, it
		// will never arrive).
		Monitor->wait (&I, st);
		sleep;
	}
	if (I->ri (st, &c, 1) == ERROR) {
		// Disconnection or EOF
		if ((Flags & XTRN_IMODE_SOCKET)) {
			// Delete the mailbox only if this is a socket
			Assert (I == O, "UART at %s: illegal mailbox",
				TheStation->getSName ());
			delete I;
			I = O = NULL;
		} else {
			// Terminate the process if device, there is nothing
			// more it can do
			terminate (PI);
			PI = NULL;
			sleep;
		}
		// This will never happen for a device
		Monitor->wait (&I, st);
		sleep;
	}

	return c;
}

/*
 * States of the automaton that reads bytes from a timed UART mailbox
 */
#define	TCS_WOB	0	// Waiting for chunk
#define	TCS_WTI	1	// Waiting for time
#define	TCS_RAS	2	// Reading ASCII
#define	TCS_EAS	3	// Escape ASCII
#define	TCS_UHX 4	// Hex upper nibble
#define	TCS_LHX	5	// Hex lower nibble

#define	TI_ASS_LEN  32	// Maximum length of time string

void UART::getTimed (int state, char *res) {
/*
 * We have to maintain state information as we may be called several times for
 * partial processing
 */
	double tm;
	int v;
	char *sp, c;
	Boolean rel;
	TIME st;

Redo:
	switch (TCS) {

	    case TCS_WOB:
		// Waiting for the opening brace
		while (1) {
			c = getOneByte (state);
			if (isspace (c))
				continue;
			if (isdigit (c) || c == '+' || c == '-' || c == '.' ||
			    c == 'e' || c == 'E') {
				if (TI_aux == NULL) {
					// Assembling the time string
					TI_aux = new char [TI_ASS_LEN];
					TI_ptr = 0;
				}
				if (TI_ptr >= TI_ASS_LEN - 1)
					ti_err ();
				TI_aux [TI_ptr++] = c;
				continue;
			}
			if (c == '{') {
				// Beginning of the string
				if (TI_aux == NULL)
					ti_err ();
				TI_aux [TI_ptr] = '\0';
				// Means relative to the previous one
				rel = (TI_aux [0] == '+');
				tm = strtod (TI_aux, &sp);
				if (sp != TI_aux + TI_ptr || tm < 0.0)
					ti_err ();
				delete [] TI_aux;
				TI_aux = NULL;
				// Determine the starting time in ITU
				st = etuToItu (tm);
				if (rel)
					TimedChunkTime += st;
				else
					TimedChunkTime = st;
				// Waiting for the right time
				TCS = TCS_WTI;
				goto Redo;
			}
			ti_err ();
		}

	    case TCS_WTI:
		// Waiting for the right time
		if (TimedChunkTime > Time) {
			Timer->wait (TimedChunkTime - Time, state);
			sleep;
		} else
			// I am not sure if this is right. If the input arrives
			// (e.g., over a socket) and is late, its playback time
			// is set to now, such that the subsequent offset will
			// be synchronized to the last sample.
			TimedChunkTime == Time;

		if (Flags & XTRN_IMODE_HEX) {
			// Reading HEX
			TCS = TCS_UHX;
			goto Redo;
		}
		// Reading ASCII
		TCS = TCS_RAS;

	    case TCS_RAS:

		c = getOneByte (state);
		if (c == '\\') {
			// Escape
			TCS = TCS_EAS;
			goto Redo;
		}
		if (c == '}') {
			// End of chunk
			TCS = TCS_WOB;
			goto Redo;
		}
		*res = c;
		return;

	    case TCS_EAS:

		// Escaped ASCII
		c = getOneByte (state);
		if (c == 'r')
			c = '\r';
		else if (c == 'n')
			c = '\n';
		else if (c == 't')
			c = '\t';
	
		TCS = TCS_RAS;
		*res = c;
		return;

	    case TCS_UHX:

		while (1) {
			c = getOneByte (state);
			if (c == '}') {
				// done
				TCS = TCS_WOB;
				goto Redo;
			}
			if (!isxdigit (c))
				// Ignore
				continue;

			TI_ptr = (unhex (c) << 4);
			TCS = TCS_LHX;
			goto Redo;
		}

	    case TCS_LHX:

		c = getOneByte (state);
		if (!isxdigit (c))
			ti_err ();

		TCS = TCS_UHX;
		*res = TI_ptr | unhex (c);
		return;
	}
}

void UART::sendStuff (int st, char *buf, int nc) {

	if (O == NULL) {
ReDo:
		// Here comes the tricky bit. This means that we are
		// writing to a socket and there is no connection yet.
		if ((Flags & XTRN_OMODE_HOLD) == 0)
			// Just drop the stuff; this also applies for a
			// non-socked (absent) interface
			return;

		// Should collect the bytes to present them once we get
		// connected

		Assert (I == NULL, "UART at %s: I != NULL while O == NULL",
			TheStation->getSName ());

		// Under the circumstances, we can recycle TI_aux for the
		// temporary buffer. We shall flush it forcibly when we get
		// the mailbox.

		if (TI_aux == NULL) {
			TI_aux = (char*) malloc (1024);
			// The first word of TI_aux stores the limit
			*((int*)TI_aux) = 1024;
			TI_ptr = sizeof (int);
		}

		while (TI_ptr + nc > *((int*)TI_aux)) {
			// We have to grow the buffer
			TI_aux = (char*) realloc (TI_aux, 
				*((int*)TI_aux) += 1024);
		}

		memcpy (TI_aux + TI_ptr, buf, nc);
		TI_ptr += nc;
		return;
	}

	if (O->wi (st, buf, nc) == ERROR) {
		// Disconnected, kill the mailbox
		if ((Flags & XTRN_OMODE_SOCKET)) {
			// Delete the mailbox only if this is a socket
			Assert (I == O, "UART at %s: illegal mailbox",
				TheStation->getSName ());
			delete I;
			I = O = NULL;
			goto ReDo;
		}
	}
}

UART_in::perform {

    TIME D;
    char c;

    state Get:

	if ((U->Flags & XTRN_IMODE_TIMED)) {
		// Timed
		if ((D = TimeLastRead + U->ByteTime) > Time) {
			Timer->wait (D - Time, Get);
			sleep;
		}
		// Timed sequences get in even if nobody wants them, i.e.,
		// no room in the buffer. They are lost in such cases.
		U->getTimed (Get, &c);
		if (!U->ibuf_full ())
			U->ibuf_put ((byte)c);
	} else {
		// Untimed: the characters are ready to be accepted, but they
		// wait until needed
		if (U->ibuf_full ()) {
			Monitor->wait (&(U->IB_out), Get);
			sleep;
		}
		if ((D = TimeLastRead + U->ByteTime) > Time) {
			Timer->wait (D - Time, Get);
			sleep;
		}
		// Get next byte
Retry:
		c = U->getOneByte (Get);
		if ((U->Flags & XTRN_IMODE_HEX)) {
			// This is the first byte of a hex tuple
			if (!isxdigit (c))
				// Ignore it as if nothing was there
				goto Retry;
			U->TI_ptr = (unhex (c) << 4);
			proceed GetH1;
		}
		// Just store the byte
		U->ibuf_put ((byte)c);
	}

Complete:

	TimeLastRead = Time;
	Monitor->signal (&(U->IB_in));
	proceed Get;

    state GetH1:

	c = U->getOneByte (GetH1);
	if (!isxdigit (c))
		ti_err ();
	U->ibuf_put (U->TI_ptr | unhex (c));
	goto Complete;
}

UART_out::perform {

    byte b;
    char tc [3];
    TIME D;

    state Put:

	if (U->obuf_empty ()) {
		Monitor->wait (&(U->OB_in), Put);
		sleep;
	}

	if ((D = TimeLastWritten + U->ByteTime) > Time) {
		Timer->wait (D - Time, Put);
		sleep;
	}

	b = U->obuf_peek ();

	if ((U->Flags & XTRN_OMODE_HEX)) {
		// Three bytes
		tc [0] = tohex (b >> 4);
		tc [1] = tohex (b & 0xf);
		tc [2] = ' ';
		U->sendStuff (Put, tc, 3);
	} else {
		U->sendStuff (Put, (char*)(&b), 1);
	}
	
	// Remove the byte
	U->obuf_get ();
	Monitor->signal (&(U->OB_out));

	TimeLastWritten = Time;

	proceed Put;
}

UART::~UART () {

	excptn ("UART at %s: cannot delete UARTs", TheStation->getSName ());

#if 0
	if (I == O)
		O = NULL;

	if (I != NULL) {
		if (I->isConnected ())
			I->disconnect (CLEAR);

		delete I;
	}

	if (O != NULL) {
		if (O->isConnected ())
			O->disconnect (CLEAR);

		delete O;
	}

	if (PI != NULL)
		PI->terminate ();

	if (PO != NULL)
		PO->terminate ();

	delete [] IBuf;
	delete [] OBuf;

	if (TI_aux != NULL)
		delete [] TI_aux;
#endif

}

int UART::ioop (int state, int ope, char *buf, int len) {
/*
 * Note: there is no 'dev' parameter from the PicOS version. Nodes will want
 * to wrap this method anyway.
 */
	int b, bc = len;

	if (len <= 0)
		// Not sure about this
		return 0;

	switch (ope) {

		case READ:

			while (bc > 0) {
				b = ibuf_get ();
				if (b < 0)
					break;
				*buf++ = (char) b;
				bc--;
			}

			if (bc == len) {
				// Nothing read
				if (state == NONE)
					return 0;
				Monitor->wait (&IB_in, state);
				sleep;
			}

			// We have extracted something
			Monitor->signal (&IB_out);
			return len - bc;

		case WRITE:

			while (bc > 0) {
				if (obuf_put (*buf) < 0)
					// No more room in the buffer
					break;
				buf++;
				bc--;
			}

			if (bc == len) {
				// Nothing written
				if (state == NONE)
					return 0;
				Monitor->wait (&OB_out, state);
				sleep;
			}

			// We have stored something
			Monitor->signal (&OB_in);
			return len - bc;

		default:

			excptn ("UART->ioop: illegal operation");
	}
}

void UartHandler::setup (PicOSNode *tpn, Dev *a) {

	lword c;

	Agent = a;
	UA = tpn->uart->U;

	// Make sure we don't belong to the node. That would get us killed
	// upon reset.

	c = ECONN_OK;

	if (UA == NULL) {
		// No UART for this station
		c = ECONN_NOUART;
	} else if (imode (UA->Flags) != XTRN_IMODE_SOCKET) {
		c = ECONN_ITYPE;
	} else if (UA->I != NULL || UA->O != NULL) {
		// Duplicate connection, refuse it
		assert (UA->I != NULL && UA->O != NULL,
			"UART at %s: inconsistent status",
				tpn->getSName ());
		c = ECONN_ALREADY;
	}

	if (c != ECONN_OK) {
		create Disconnector (Agent, c);
		terminate ();
	}
}

UartHandler::perform {

	int	nc;
	lword 	c;

	state AckUart:

 		c = htonl (ECONN_OK | (((lword)(UA->Rate)) << 8));
 		if (Agent->wi (AckUart, (char*)(&c), 4) == ERROR) {

			// We are disconnected
Term:
			delete Agent;
			terminate;
		}

		// Check if there is accumulated output to be flushed
		if (UA->TI_aux != NULL) {
			Buf = UA->TI_aux + sizeof (int);
			Left = UA->TI_ptr - sizeof (int);
			proceed UartFlush;
		}
UartFlushed:
		UA->I = UA->O = Agent;
		Monitor->signal (&(UA->I));
		// We are done; the UART driver takes over
		terminate;

	state UartFlush:

		if (Agent->wl (UartFlush, Buf, Left) == ERROR)
			goto Term;

		// Clean up

		free (UA->TI_aux);
		UA->TI_aux = NULL;
		// Clear the "save" bit, such that subsequent
		// disconnections will not result in accumulating
		// UART outputs in memory, which could overtax it.
		UA->Flags &= ~XTRN_OMODE_HOLD;

		goto UartFlushed;
}

void ClockHandler::setup (Dev *a) {

	Agent = a;
	secdelay = etuToItu (1.0);
}

ClockHandler::perform {

	lword netf, c;

	state AckClk:

		c = htonl (ECONN_OK);
		if (Agent->wi (AckClk, (char*)(&c), 4) == ERROR) {
			// We are disconnected
Term:
			delete Agent;
			terminate;
		}

	transient Send:

		netf = secconv ();
		netf = htonl (netf);

		if (Agent->wi (Send, (char*)(&netf), 4) == ERROR)
			goto Term;

		Timer->wait (secdelay, Send);
}

word PINS::adc (short v, int ref) {
/*
 * At present, this assumes that the voltage changes between -3.3V and +3.3V.
 * The ADC/DAC only recognize positive voltage.
 */
	word range;

	if (v <= 0)
		return 0;

	switch (ref) {

		case 0:	  range = (word) ((1.5/3.3) * (int)0x7fff); break;
		case 1:	  range = (word) ((2.5/3.3) * (int)0x7fff); break;
		case 3:		// Vref (have to define it in the data set)
				// ignore for now, i.e., use Vcc
		default:  range = (word) 0x7fff;	// Vcc
	}

	if (v >= range)
		return 4095;

	return (word) ((v * 4095) / range);
}

short PINS::dac (word v, int ref) {

	int res;

	if (v > 4095)
		v = 4095;

	res = ((int)v * (int)0x7fff) / 4095;

	if (ref)
		// reference == 3*3.3V, but the result is truncated at 3.3V
		res *= 3;

	if (res > 0x7fff)
		return 0x7fff;

	return (short) res;
}

ButtonPin::ButtonPin (PINS *ps, byte pn, byte pol) {

	Repeater = NULL;
	Polarity = pol;
	Pin = pn;
	Pins = ps;
	State = 0;	// Off
}

void ButtonPin::reset () {

	if (Repeater != NULL) {
		Repeater->terminate ();
		// This is redundant
		Repeater = NULL;
	}

	State = 0;
	LastUpdate = TIME_0;
}

Boolean ButtonPin::pinon () {
//
// Tells if the current pin status is "on"
//
	int pv;

	pv = PINS::gbit (Pins->IValues, Pin);
	return (Polarity && pv) || (!Polarity && !pv);
}

void ButtonPin::update (word val) {

	byte on;

	// The current pin status
	on = Polarity ? (val != 0) : (val == 0);

	if (on) {
		// The requested state is on
		if (!Repeater)
			Repeater = create (System) ButtonRepeater (this);
	} else {
		// The pin is OFF
		if (Repeater)
			Repeater->signal (0);
	}
}

ButtonRepeater::perform {

	Long di;

	state BRP_START:

		if (done ())
			terminate;

		doaction ();

		// Check if should debounce
		if ((di = debounce (4)) != 0) {
			// Yes, wait for this much and check the status again
			Timer->delay (MILLISECOND * di, BRP_CHECK);
			sleep;
		}

	transient BRP_CHECK:

		if (done ())
			terminate;

		this->wait (SIGNAL, BRP_CHECK);

		if ((di = debounce (5)) != 0) {
			// Wait for repeat
			Timer->delay (MILLISECOND * di, BRP_REPEAT);
			sleep;
		}

	state BRP_REPEAT:

		if (done ())
			terminate;

		doaction ();

		this->wait (SIGNAL, BRP_REPEAT);

		if ((di = debounce (6)) == 0) 
			di = debounce (5);
		// Wait for repeat
		Timer->delay (MILLISECOND * di, BRP_REPEAT);
}
		
void PINS::reset_buttons () {

	int i;

	ButtonsAction = NULL;
	for (i = 0; i < NButs; i++)
		Buts [i] -> reset ();
}

PINS::PINS (data_pn_t *PID) {

	int i, k;
	byte *taken;

	I = O = NULL;
	Upd = NULL;
	InputThread = OutputThread = MonitorThread = NotifierThread = NULL;

	TimedRequestTime = TIME_0;

	TPN = ThePicOSNode;

	Flags = PID->PMode;

	// This has been checked already
	assert (PID->NP != 0, "PINS: the number of pins at %s is zero",
		TheStation->getSName ());
	PIN_MAX = PID->NP;

	// Pin array size in bytes
	PASIZE = ((PIN_MAX + 7) >> 3);

	// Same in nibbles
	NASIZE = ((PIN_MAX + 3) >> 2);

	taken = new byte [PASIZE];
	for (i = 0; i < PASIZE; i++)
		taken [i] = 0;

	IValues   = new byte [PASIZE];
	OValues   = new byte [PASIZE];
	Direction = new byte [PASIZE];

	PIN_MAX_ANALOG = PID->NA;

	// This has been checked, too (assert is removed at optimization)
	assert (PIN_MAX_ANALOG <= PIN_MAX,
		"PINS: number of ADC-capable pins (%1d) at %s is > total",
			PIN_MAX_ANALOG, TheStation->getSName ());

	// We assume that those arrays won't disappear on us; these are the
	// defaults, which we need for reset. They also describe the static
	// values of pins, if they are not supposed to change during the
	// experiment. Note that default directions for all praxis-accessible
	// PINS are IN.
	//
	// Note: it is essential that they are not mofified, as they can be
	// shared by multiple nodes, if decribed in <defaults>!!!
	DefIVa = PID->IV;
	DefAVo = PID->VO;
	Status = PID->ST;

	// Debouncers: we need them for CNT and NOT, as well as the buttons
	for (i = 0; i < 7; i++)
		// We set them up, unless all entries in DEB are zero
		if (PID->DEB [i] != 0)
			break;
	if (i < 7) {
		Debouncers = new Long [7];
		for (i = 0; i < 7; i++)
			Debouncers [i] = PID->DEB [i];
	} else {
		Debouncers = NULL;
	}
	
	// The buttons
	NButs = 0;
	if ((Buttons = PID->BN) != NULL) {
		// Yes, it is a bit messy; we use two structures for pins;
		// Buttons gives us a mapping from pin number to button number;
		// Debouncing (timing) parameters are global for now (and so is
		// the polarity), but this can be easily changed later
		for (i = 0; i < PIN_MAX; i++) {
			if (Buttons [i] != BNONE)
				NButs++;
		}
		Buts = new ButtonPin* [NButs];
		for (i = k = 0; i < PIN_MAX; i++) {
			if (Buttons [i] != BNONE) {
				Buts [k] = new ButtonPin (this, (byte)i,
					PID->BPol);
				k++;
			}
		}
	} else {
		Buts = NULL;
	}
			
	PIN_MONITOR [0] = PIN_MONITOR [1] = BNONE;

	if (PID->MPIN != BNONE || PID->NPIN != BNONE) {
		// At least one of the monitor pins is defined; check for
		// debouncers
	if (PID->MPIN != BNONE) {
			Assert (PID->MPIN < PIN_MAX && gbit (taken, PID->MPIN)
				== 0,
		    	"PINS: illegal counter pin number %1d at %s", PID->MPIN,
				TheStation->getSName ());
			sbit (taken, PID->MPIN);
			PIN_MONITOR [0] = PID->MPIN;
		}
		if (PID->NPIN != BNONE) {
			Assert (PID->NPIN < PIN_MAX && gbit (taken, PID->NPIN)
				== 0,
	    	       "PINS: illegal notifier pin number %1d at %s", PID->NPIN,
				TheStation->getSName ());
			sbit (taken, PID->NPIN);
			PIN_MONITOR [1] = PID->NPIN;
		}
	}

	if (PID->D0PIN != BNONE) {
		Assert (PID->D0PIN < PIN_MAX && gbit (taken, PID->D0PIN) == 0,
		    "PINS: illegal DAC0 pin number %1d at %s", PID->D0PIN,
			TheStation->getSName ());
		PIN_DAC [0] = PID->D0PIN;
		sbit (taken, PID->D0PIN);
	} else 
		PIN_DAC [0] = BNONE;

	if (PID->D1PIN != BNONE) {
		Assert (PID->D1PIN < PIN_MAX && gbit (taken, PID->D1PIN) == 0,
		    "PINS: illegal DAC1 pin number %1d at %s", PID->D1PIN,
			TheStation->getSName ());
		PIN_DAC [1] = PID->D1PIN;
		sbit (taken, PID->D1PIN);
	} else
		PIN_DAC [1] = BNONE;

	delete [] taken;

	if (PIN_MAX_ANALOG) {
		AASIZE = (PIN_MAX_ANALOG + 7) >> 3;
		Analog = new byte [AASIZE];
		// Set in rst
		VADC = new short [PIN_MAX_ANALOG];
	} else {
		AASIZE = 0;
		Analog = NULL;
		VADC = NULL;
	}

	if (imode (Flags) == XTRN_IMODE_SOCKET ||
					omode (Flags) == XTRN_OMODE_SOCKET) {
		Assert (imode (Flags) == XTRN_IMODE_SOCKET &&
		    omode (Flags) == XTRN_OMODE_SOCKET, 
			"PINS at %s: confilcting modes %x",
			    TheStation->getSName (),
			        Flags);
		// That's it, the devices will be created upon connection
	
	} else if (imode (Flags) == XTRN_IMODE_DEVICE) {
		// Need a mailbox for the input end
		Assert (PID->PIDev != NULL,
			"PINS at %s: input device cannot be NULL",
				TheStation->getSName ());
		I = create Dev;
		// Check if the output end uses the same device
		if (omode (Flags) == XTRN_OMODE_DEVICE && strcmp (PID->PIDev,
		    PID->PODev) == 0) {
			// Yep, single mailbox will do
			i = I->connect (DEVICE+READ+WRITE, PID->PIDev, 0,
				(XTRN_MBX_BUFLEN > PUPD_OUTPUT_BUFLEN) ?
					XTRN_MBX_BUFLEN : PUPD_OUTPUT_BUFLEN);
			O = I;
		} else {
			i = I->connect (DEVICE+READ, PID->PIDev, 0,
				XTRN_MBX_BUFLEN);
		}
		if (i == ERROR)
			excptn ("PINS at %s: cannot open device %s",
				TheStation->getSName (), PID->PIDev);
	} else if (imode (Flags) == XTRN_IMODE_STRING) {
		Assert (PID->PIDev != NULL,
			"PINS at %s: the input string is empty",
				TheStation->getSName ());
		S = PID->PIDev;
	}

	// We are done with the input end
	if (O == NULL && omode (Flags) == XTRN_OMODE_DEVICE) {
		// A separate DEVICE 
		Assert (PID->PODev != NULL, "PINS at %s: no output device name",
			TheStation->getSName ());
		O = create Dev;
		if (O->connect (DEVICE+WRITE, PID->PODev, 0,
		    PUPD_OUTPUT_BUFLEN))
			excptn ("PINS at %s: cannot open device %s",
				TheStation->getSName (), PID->PODev);
	}

	// Allocate buffer for updates
	UBuf = new char [PUPD_OUTPUT_BUFLEN];

	// Note that I shares location with S
	if (I != NULL) {
		// The input end
		create (System) PinsInput (this);
	}

	if (O != NULL) {
		// Note that this one will be indestructible
		Upd = create PUpdates (MAX_Long);
		// Signal initial (full) update; not needed, rst does it
		// qupd_all ();
		create (System) PinsHandler (this, NULL);
	}

	rst ();
}

void PINS::rst () {

	int i;

	// Note that reset doesn't affect the agent interface. I am not
	// sure what we should do other than sending a "full update" to
	// the agent.

	for (i = 0; i < PASIZE; i++) {
		// Default directions are IN
		Direction [i] = 0;
		IValues [i] = (DefIVa != NULL) ? DefIVa [i] : 0;
		OValues [i] = 0;
	}

	PIN_DAC_USED [0] = PIN_DAC_USED [1] = NO;

	if (PIN_MAX_ANALOG) {
		for (i = 0; i < AASIZE; i++)
			// Analog input all-off
			Analog [i] = 0;
		// Perhaps we should reconcile voltage with pin values, but
		// perhaps not. Perhaps the default voltages should
		// be interpreted as an extra degree of freedom - to be there
		// when the praxis asks for an ADC value. Note that the
		// user can always make them consistent.
		for (i = 0; i < PIN_MAX_ANALOG; i++)
			// Initial voltage
			VADC [i] = (DefAVo != NULL) ? DefAVo [i] : 0;
	}

	adc_inuse = pmon_cnt_on = pmon_cmp_on = pmon_cmp_pending = pmon_not_on =
		pmon_not_pending = NO;

	if (Upd != NULL) {
		// Signal a change: send all
		Upd->erase ();
		qupd_all ();
	}

	if (MonitorThread != NULL) {
		MonitorThread->terminate ();
		MonitorThread = NULL;
	}

	if (NotifierThread != NULL) {
		NotifierThread->terminate ();
		NotifierThread = NULL;
	}

	reset_buttons ();
}

PINS::~PINS () {

	excptn ("PINS at %s: cannot delete PINS", TheStation->getSName ());
}

void PulseMonitor::setup (PINS *pins, Process **p, byte pin, byte onv,
						Long ontime, Long offtime) {
	PN = pins;
	TH = p;
	ONV = onv;
	WCNT = ontime;
	OFFTime = offtime;
	PIN = PN->PIN_MONITOR [pin];
	assert (*TH == NULL, "PulseMonitor at %s: previous monitor thread is "
		"alive", PN->TPN->getSName ());
	*TH = this;
}

PulseMonitor::perform {

	byte b;

	state Probe:

		b = PINS::gbit (PN->IValues, PIN);

		if (b != ONV) {
			*TH = NULL;
			terminate;
		}

		if (WCNT == 0) {
			// Trigger
			PN->pmon_update (PIN);
			WCNT = OFFTime * 8;
			proceed WECyc;
		}

		WCNT--;

		Timer->delay (MILLISECOND, Probe);

	state WECyc:

		// Wait for OFF

		b = PINS::gbit (PN->IValues, PIN);

		if (b != ONV) {
			WCNT = OFFTime;
			proceed WNewCyc;
		}

		if (WCNT == 0) {
			// Give up
			*TH = NULL;
			terminate;
		}

		WCNT--;

		Timer->delay (MILLISECOND, WECyc);

	state WNewCyc:

		b = PINS::gbit (PN->IValues, PIN);

		if (b == ONV)
			// Failed
			proceed WECyc;

		if (WCNT == 0) {
			// Done, disappear
			*TH = NULL;
			terminate;
		}

		WCNT--;

		Timer->delay (MILLISECOND, WNewCyc);
}

void PINS::buttons_action (void (*act)(word)) {
//
// Declare a pin action function
//
	reset_buttons ();
	ButtonsAction = act;
}
			
void PINS::pin_new_value (word pin, word val) {

	int i;
	byte prev;

	if (pin == PIN_MONITOR [0]) {
		if (pmon_cnt_on) {
			// This means that the pulse monitor is being used
			prev = gbit (IValues, pin);
			// The previous value
			if (prev == val)
				// Nothing to do
				return;
			if (val)
				sbit (IValues, pin);
			else
				cbit (IValues, pin);

			if (val == MonPolarity) {
				// Counter trigger
				if (Debouncers == NULL || (Debouncers [0] == 0 
				    && Debouncers [1] == 0)) {
					// No debouncer
					pmon_update (pin);
				} else {
					if (MonitorThread == NULL) {
						// Must run at System to make
						// oblivious to node's resets
						create (System)
							PulseMonitor (this,
								&MonitorThread,
								0,
								MonPolarity,
								Debouncers [0],
								Debouncers [1]);
					}
				}
			}
			return;
		}
	} else if (pin == PIN_MONITOR [1]) {
		if (pmon_not_on) {
			// This means that the notifier is being used
			prev = gbit (IValues, pin);
			// The previous value
			if (prev == val)
				// Nothing to do
				return;
			if (val)
				sbit (IValues, pin);
			else
				cbit (IValues, pin);

			if (val == NotPolarity) {
				// Notifier trigger
				if (Debouncers == NULL || (Debouncers [2] == 0
				    && Debouncers [3] == 0)) {
					// No debouncer
					pmon_update (pin);
				} else {
					if (NotifierThread == NULL) {
						create (System)
							PulseMonitor (this,
								&NotifierThread,
								1,
								NotPolarity,
								Debouncers [2],
								Debouncers [3]);
					}
				}
			}
			return;
		}
	}

	// A regular pin, including those assigned to Pulse Monitor and
	// Notifier, if not being used as such

	if (Buttons != NULL && Buttons [pin] != BNONE) {
		for (i = 0; i < NButs; i++) {
			if (Buts [i] -> Pin == pin) {
				Buts [i] -> update (val);
				break;
			}
		}
	}

	if (val) {
		sbit (IValues, pin);
		if (pin < PIN_MAX_ANALOG)
			// Set the analog input voltage as well
			VADC [pin] = 0x7fff;
	} else {
		cbit (IValues, pin);
		if (pin < PIN_MAX_ANALOG)
			// Set the analog input voltage as well
			VADC [pin] = 0x0000;
	}
}

void PINS::pin_new_voltage (word pin, word val) {

	if (!pin_adc (pin))
		return;

	VADC [pin] = val;

	// Set input value as well
	if (gbit (IValues, pin)) {
		// Was up, check if going down
		if (val < SCHMITT_DOWNH && 
			// Has a chance to go down at all
			(val < SCHMITT_DOWNL ||
			    // Sure to go down
			    rnd (SEED_toss) < ((double)SCHMITT_DOWNH - val)/
				((double)(SCHMITT_DOWNH - SCHMITT_DOWNL))))
				    cbit (IValues, pin);
	} else {
		// Was down, check if going up
		if (val > SCHMITT_UPL && 
			// Has a chance to go up at all
			(val > SCHMITT_UPH ||
			    // Sure to go up
			    rnd (SEED_toss) > ((double)SCHMITT_UPH - val)/
				((double)(SCHMITT_UPH - SCHMITT_UPL))))
				    sbit (IValues, pin);
	}
}

void PINS::pmon_update (byte pin) {
/*
 * This is called to update the pulse counter or notifier, depending on pin
 */
	if (pin == PIN_MONITOR [0]) {
		// Update the counter
		if (((++pmon_cnt) & 0xff000000))
			// Wrap around at 16M
			pmon_cnt = 0;
		if (pmon_cmp_on && pmon_cnt == pmon_cmp) {
			pmon_cmp_pending = YES;
			TPN->TB.signal (PMON_CNTEVENT);
		}
	} else {
		assert (pin == PIN_MONITOR [1],
			"pmon_update at %s: illegal monitor pin %1d",
				TPN->getSName (), pin);
		pmon_not_pending = YES;
		TPN->TB.signal (PMON_NOTEVENT);
	}
}

byte PINS::pin_gstat (word p) {

	if (p >= PIN_MAX)
		return PINSTAT_ABSENT;

	if (Status != NULL && gbit (Status, p) == 0)
		return PINSTAT_ABSENT;

	if (p == PIN_MONITOR [0] && pmon_cnt_on)
		return PINSTAT_PULSE;

	if (p == PIN_MONITOR [1] && pmon_not_on)
		return PINSTAT_NOTIFIER;

	if (p == PIN_DAC [0] && PIN_DAC_USED [0])
		return PINSTAT_DAC0;

	if (p == PIN_DAC [1] && PIN_DAC_USED [1])
		return PINSTAT_DAC1;

	if (p < PIN_MAX_ANALOG && gbit (Analog, p))
		return PINSTAT_ADC;

	return gbit (Direction, p);

	return YES;
}

void PINS::qupd_pin (word pn) {
/*
 * Queue an update regarding the new status of a pin
 */
	pin_update_t upd;
	word val;
	byte st;

	if (OutputThread == NULL)
		// Forget it
		return;

	switch (st = pin_gstat (pn)) {

	    case PINSTAT_INPUT		: val = gbit (IValues, pn); break;
	    case PINSTAT_OUTPUT		: val = gbit (OValues, pn); break;
	    case PINSTAT_ADC		: val = VADC [pn]; break;
	    case PINSTAT_DAC0		: val = VDAC [0]; break;
	    case PINSTAT_DAC1		: val = VDAC [1]; break;
	    case PINSTAT_PULSE		: val = 0; break;
	    case PINSTAT_NOTIFIER	: val = 0; break;
	    case PINSTAT_ABSENT		: val = 0; break;

	    default:
			// Impossible
			excptn ("qupd_pin: illegal pin status %1d", st);
	}

	upd.pin = pn;
	upd.stat = st;
	upd.value = val;
	Upd->queue (upd);
}

void PINS::qupd_all () {
/*
 * Initial update for all pins
 */
	byte pin;

	// Request parameters
	Upd->put (-1);
	for (pin = 0; pin < PIN_MAX; pin++) 
		qupd_pin (pin);
}

Boolean PINS::pin_adc_available (word p) {

	return (p < PIN_MAX_ANALOG && pin_available (p));
}

word PINS::pin_read (word pin) {

	if (!pin_available (pin))
		return 8;

	if (pin_dac (pin))
		return 6;

	if (pin_adc (pin))
		return 4;

	if (pin_output (pin))
		return pin_ovalue (pin) | 2;

	return pin_ivalue (pin);
}

int PINS::pin_write (word pin, word val) {

	byte od, ov;

	if (!pin_available (pin))
		return ERROR;

	if ((val & 4)) {
		if (pin_dac_present () && (val & 2) == 0) {
			if (!pin_dac_available (pin))
				return ERROR;
			if (!pin_dac (pin)) {
				pin_set_dac (pin);
				// Send DAC voltage update
				qupd_pin (pin);
			}
		} else {
			if (!pin_adc_available (pin))
				return ERROR;
			if (!pin_adc (pin)) {
				pin_set_adc (pin);
				qupd_pin (pin);
			}
		}
		return 0;
	}

	od = gbit (Direction, pin);

	if ((val & 2)) {
		if (od) {
			pin_set_input (pin);
			qupd_pin (pin);
		}
		return 0;
	}

	ov = gbit (OValues, pin);

	if (val)
		pin_set (pin);
	else
		pin_clear (pin);

	pin_set_output (pin);

	if (ov != val || od == 0)
		// Update needed
		qupd_pin (pin);

	return 0;
}

int PINS::pin_read_adc (word state, word pin, word ref, word smpt) {

	int res;

	if (!pin_adc_available (pin))
		return ERROR;

	if (gbit (Analog, pin) == 0) {
		// Must change status
		pin_set_adc (pin);
		qupd_pin (pin);
	}

	// At the moment, this model is stupid and, I am afraid, it will have
	// to remain so. We simply wait for the prescribed smpt interval and
	// take the voltage. What else would you expect me to do?

	if (adc_inuse) {
		// Second call to collect the data
		adc_inuse = NO;
Immed:
		return (int) adc (VADC [pin], ref);
	}

	if (state == WNONE)
		// This is a simplification
		goto Immed;

	adc_inuse = YES;
	Timer->delay (MILLISECOND * smpt, state);
	sleep;
}

int PINS::pin_write_dac (word pin, word val, word ref) {

	int ix;
	word ov;
	Boolean m;

	if (!pin_dac_available (pin))
		return ERROR;

	m = pin_dac (pin);

	pin_set_dac (pin);

	ix = (pin == PIN_DAC [0]) ? 0 : 1;

	ov = VDAC [ix];
	VDAC [ix] = dac (val, ref);

	if (VDAC [ix] != ov || !m)
		// Update needed
		qupd_pin (pin);
}

void PINS::pmon_start_cnt (long count, Boolean edge) {

	Assert (PIN_MONITOR [0] != BNONE,
		"pmon_start_cnt at %s: this node has no pulse monitor",
			TheStation->getSName ());

	if (MonitorThread != NULL) {
		MonitorThread->terminate ();
		MonitorThread = NULL;
	}

	MonPolarity = edge ? 1 : 0;

	pmon_cmp_pending = NO;

	if (!pmon_cnt_on) {
		pmon_cnt_on = YES;
		qupd_pin (PIN_MONITOR [0]);
	}

	if (count >= 0)
		// Set the counter
		pmon_cnt = count & 0x00ffffff;

	if (pmon_cmp_on) {
		pmon_cmp = pmon_cnt;
		pmon_cmp_pending = NO;
	}
}

void PINS::pmon_dec_cnt (void) {

	pmon_cnt = (pmon_cnt - pmon_cmp) & 0x00ffffff;
}

void PINS::pmon_sub_cnt (long decr) {

	pmon_cnt = (pmon_cnt - decr) & 0x00ffffff;
}

void PINS::pmon_add_cmp (long incr) {

	pmon_cmp = (pmon_cmp + incr) & 0x00ffffff;
}

void PINS::pmon_stop_cnt () {

	if (!pmon_cnt_on)
		return;

	if (MonitorThread != NULL) {
		MonitorThread->terminate ();
		MonitorThread = NULL;
	}

	if (pmon_cnt_on)
		pmon_cnt_on = NO;
		qupd_pin (PIN_MONITOR [0]);
}

void PINS::pmon_set_cmp (long count) {

	pmon_cmp_pending = NO;

	if (count < 0) {
		// This switched off the comparator
		pmon_cmp_on = NO;
		return;
	}

	pmon_cmp = count & 0x00ffffff;

	pmon_cmp_on = YES;

	if (pmon_cmp == pmon_cnt) {
		pmon_cmp_pending = YES;
		TPN->TB.signal (PMON_CNTEVENT);
	}
}

lword PINS::pmon_get_cnt () {

	return pmon_cnt;

}

lword PINS::pmon_get_cmp () {

	return pmon_cmp;

}

void PINS::pmon_start_not (Boolean edge) {

	Assert (PIN_MONITOR [1] != BNONE,
		"pmon_start_not at %s: this node has no notifier",
			TheStation->getSName ());

	if (NotifierThread != NULL) {
		NotifierThread->terminate ();
		NotifierThread = NULL;
	}

	NotPolarity = edge ? 1 : 0;

	pmon_not_pending = NO;

	if (!pmon_not_on) {
		pmon_not_on = YES;
		qupd_pin (PIN_MONITOR [1]);
	}
}

void PINS::pmon_stop_not () {

	if (!pmon_not_on)
		return;

	if (NotifierThread != NULL) {
		NotifierThread->terminate ();
		NotifierThread = NULL;
	}

	pmon_not_pending = pmon_not_on = NO;
	qupd_pin (PIN_MONITOR [1]);
}

Boolean PINS::pmon_pending_not () {

	Boolean res;

	res = pmon_not_pending;
	pmon_not_pending = NO;
	return res;
}

Boolean PINS::pmon_pending_cmp () {

	Boolean res;

	res = pmon_cmp_pending;
	pmon_cmp_pending = NO;
	return res;
}

word PINS::pmon_get_state () {

	word state = 0;

	if (MonPolarity)
		state |= PMON_STATE_CNT_RISING;

	if (NotPolarity)
		state |= PMON_STATE_NOT_RISING;

	if (pmon_cmp_on)
		state |= PMON_STATE_CMP_ON;

	if (pmon_not_on)
		state |= PMON_STATE_NOT_ON;

	if (pmon_not_pending)
		state |= PMON_STATE_NOT_PENDING;

	if (pmon_cmp_pending)
		state |= PMON_STATE_CMP_PENDING;

	if (pmon_cnt_on)
		state |= PMON_STATE_CNT_ON;

	return state;
}

int PINS::pinup_status (pin_update_t upd) {
/*
 * Transforms a pin update request into an outgoing message
 */
	char *tb;
	tb = UBuf;

	if (*((long*)(&upd)) == -1) {
		// This is a params update: two numbers only
		sprintf (UBuf, "N %1u %1u\n", PIN_MAX, PIN_MAX_ANALOG);
	} else {
		sprintf (UBuf, "U %08.3f: %1u %1u %1u\n", ituToEtu (Time),
			upd.pin, upd.stat, upd.value);
	}

	return strlen (UBuf);
}

int PINS::pinup_update (char *rb) {
/*
 * Update the pins according to the input request. When we are called, rb
 * points to the first non-blank character of the request.
 */
	int pn, pv;
	char c;

	c = *rb++;

	pn = dechex (rb);
	pv = dechex (rb);

	if (pn < 0 || pv < 0)
		return ERROR;

	if (pn > PIN_MAX)
		// Am not sure about this one. Perhaps just ignore?
		return ERROR;

	switch (c) {

	    case 'P' :
		// Set pin value
		if (pv > 1)
			return ERROR;

		pin_new_value ((word)pn, (word)pv);
		return OK;

	    case 'D' :

		// Voltage
		if (pv > 0x0000ffff)
			return ERROR;
		pin_new_voltage ((word)pn, (word)pv);

		// More requests to come
	    default:

		// Ignore: this may be an empty line or a comment
		return OK;
	}
}
	
void PinsHandler::setup (PINS *pn, Dev *a) {

	lword c;

	PN = pn;

	if ((Agent = a) == NULL) {
		assert (PN != NULL, "PinsHandler: no PINS to handle");
		// We have been called to handle device IO
		assert (PN->O != NULL,
			"PinsHandler at %s: the mailbox is missing",
				PN->TPN->getSName ());
		assert (PN->OutputThread == NULL,
			"PinsHandler at %s: agent thread already running",
				PN->TPN->getSName ());
		PN->OutputThread = this;

	} else {
		// This is a socket connection, so we know for a fact that
		// both ends are active; thus, we are responsible for creating
		// the other thread
		c = ECONN_OK;
		if (PN == NULL)
			c = ECONN_NOPINS;
		else if (imode (PN->Flags) != XTRN_IMODE_SOCKET)
			c = ECONN_ITYPE;
		else if (PN->OutputThread != NULL)
			c = ECONN_ALREADY;

		// Make sure the buffer size will allow us to accommodate
		// requests and updates
		Agent->resize (XTRN_MBX_BUFLEN > PUPD_OUTPUT_BUFLEN ?
			XTRN_MBX_BUFLEN : PUPD_OUTPUT_BUFLEN);

		if (c != ECONN_OK) {
			create Disconnector (Agent, c);
			terminate ();
		} else {
			Assert (PN->Upd == NULL,
				"PinsHandler at %s: Upd not NULL",
					PN->TPN->getSName ());
			PN->Upd = create PUpdates (MAX_Long);
			PN->I = PN->O = Agent;
			PN->OutputThread = this;
			PN->InputThread = create PinsInput (PN);
			PN->qupd_all ();
		}
	}
}

void PinsHandler::goaway (lword c) {
/*
 * Can also be called by the input thread to kill the output end, but only
 * for a socket connection. It will also terminate the input thread itself,
 * so be careful.
 */

	if (Agent == NULL)
		excptn ("PinsHandler at %s: error writing to device",
			PN->TPN->getSName ());

	// This is a socket, so clean up and continue
	if (Agent->isActive ())
		create Disconnector (Agent, c);
	else
		delete Agent;

	PN->I = PN->O = NULL;
	if (PN->InputThread != NULL)
		PN->InputThread->terminate ();
	delete PN->Upd;
	PN->Upd = NULL;
	PN->OutputThread = PN->InputThread = NULL;
	this->terminate ();
}

PinsHandler::perform {
/*
 * Unlike the UartHandler, we are also the driver, i.e., we stay alive for as
 * long as the agent is connected and handle pin output. The input is covered
 * by a child process.
 */
	lword c;

	state AckPins:

		if (Agent != NULL) {
			// Socket connection: acknowledge
			c = htonl (ECONN_OK);
			if (Agent->wi (AckPins, (char*)(&c), 4) == ERROR) {
				// We are disconnected
				goaway (ECONN_DISCONN);
				// This means 'terminate'
				sleep;
			}
		}

	transient Loop:

		// This is the main loop
		if (PN->Upd->empty ()) {
			// No updates
			PN->Upd->wait (NONEMPTY, Loop);
			sleep;
		}

		Length = PN->pinup_status (PN->Upd->retrieve ());
		Buf = PN->UBuf;

	transient Send:

		if (PN->O->wl (Send, Buf, Length) == ERROR) {
			// Got disconnected
			goaway (ECONN_DISCONN);
			sleep;
		}

		proceed Loop;
}

void PinsInput::setup (PINS *p) {

	PN = p;
	assert (PN->InputThread == NULL, "PinsInput at %d: duplicate thread",
		PN->TPN->getSName ());
	PN->InputThread = this;

	if (imode (PN->Flags) != XTRN_IMODE_STRING)
		// Otherwise, the mailbox is not a mailbox at all
		PN->I->setSentinel ('\n');
}

PinsInput::perform {
/*
 * Request formats:
 *
 *      T [+]delay (double)  [24]
 *      P pin val [0,1]	     [7]
 *      D pin voltage        [10]
 */
	TIME st;
	double del;
	int rc;
	char *re;
	Boolean off, err;
	char cc;

	state Loop:

		BP = &(RBuf [0]);
		Left = PRQS_INPUT_BUFLEN;

		if (imode (PN->Flags) == XTRN_IMODE_STRING) {
			// This is simple
			while ((PN->Flags & XTRN_IMODE_STRLEN) != 0) {
				PN->Flags--;
				cc = *(PN->S)++;
				if (cc == '\n' || cc == '\0') {
					*BP = '\0';
					goto HandleInput;
				}
				if (Left > 1) {
					*BP++ = cc;
					Left--;
				}
			}
			if (Left == PRQS_INPUT_BUFLEN) {
				// End of string
				PN->InputThread = NULL;
				terminate;
			}
			*BP = '\0';
			goto HandleInput;
		}

	transient ReadRq:

		if ((rc = PN->I->rs (ReadRq, BP, Left)) == ERROR) {
			// Here we know this is not string
			if ((PN->Flags & XTRN_IMODE_SOCKET) == 0) {
				// A device,  EOF, close this part of the shop
				PN->InputThread = NULL;
				terminate;
			}
			// This is a socket disconnection
			Assert (PN->OutputThread != NULL,
			    	"PinsInput at %s: sibling thread is gone",
					PN->TPN->getSName ());
			((PinsHandler*)(PN->OutputThread))->
				goaway (ECONN_DISCONN);
			// That has got us killed
			sleep;
		}

		if (rc == REJECTED) {
			// Line too long
			if ((PN->Flags & XTRN_IMODE_SOCKET) == 0) {
				// A device
				excptn ("PinsHandler at %s: request "
				    "line too long, %1d is the max",
					PN->TPN->getSName (),
					    PRQS_INPUT_BUFLEN);
			}

			((PinsHandler*)(PN->OutputThread))->
				goaway (ECONN_LONG);
			sleep;
		}

		// Fine: this is the length excluding the newline
		RBuf [PRQS_INPUT_BUFLEN - 1 - Left] = '\0';
HandleInput:
		BP = &(RBuf [0]);

		skipblk (BP);
		if (*BP == 'T') {
			// This is a delay request, which we handle locally
			BP++;
			skipblk (BP);
			if (*BP == '+') {
				// Offset
				off = YES;
				BP++;
			} else
				off = NO;

			del = strtod (BP, &re);
			if (BP == re || del < 0.0) {
Error:
				// This is an error
				if ((PN->Flags & XTRN_IMODE_SOCKET) == 0) {
				    excptn ("PinsHandler at %s: illegal request"
					      " '%s'", PN->TPN->getSName (),
						RBuf);
				} else {
				    // This may be temporary (FIXME)
				    trace ("PinsHandler at %s: illegal request"
					      " '%s'", PN->TPN->getSName (),
						RBuf);
				    ((PinsHandler*)(PN->OutputThread))->
					goaway (ECONN_INVALID);
				    sleep;
				}
			}
			st = etuToItu (del);
			if (off)
				PN->TimedRequestTime += st;
			else
				PN->TimedRequestTime = st;

			proceed Delay;
		}

		if (PN->pinup_update (BP) == ERROR)
			goto Error;
			
		proceed Loop;

	state Delay:

		if (PN->TimedRequestTime > Time) {
			// May put in here some mailbox monitoring
			// code ???
			Timer->wait (PN->TimedRequestTime - Time, Delay);
			sleep;
		} else
			PN->TimedRequestTime = Time;

		proceed Loop;
}

void SNSRS::rst () {
/*
 * Sensor/actuator reset
 */
	int i;

	// Change all actuator values to defaults (do not touch the sensors);
	// Note: the "reset" values are stored there, so we can still do
	// something with them, if we want

	for (i = 0; i < NActuators; i++)
		if (Actuators [i] . Length)
			Actuators [i] . Value = Actuators [i] . RValue;

	if (Upd != NULL) {
		// Send all actuator values
		Upd->erase ();
		qupd_all ();
	}
}

SNSRS::SNSRS (data_sa_t *SID) {

	int i;

	I = O = NULL;
	Upd = NULL;
	InputThread = OutputThread = NULL;

	TimedRequestTime = TIME_0;

	TPN = ThePicOSNode;

	Flags = SID->SMode;

	if ((NSensors = SID->NS) != 0) {
		Sensors = new SensActDesc [NSensors];
		bcopy (SID->Sensors, Sensors, sizeof (SensActDesc) * NSensors);
	} else
		Sensors = NULL;

	if ((NActuators = SID->NA) != 0) {
		Actuators = new SensActDesc [NActuators];
		bcopy (SID->Actuators, Actuators, sizeof (SensActDesc) *
			NActuators);
	} else
		Actuators = NULL;
		
	// This should have been checked by now
	assert (NSensors != 0 || NActuators != 0,
		"SENSORS: the number of sensors and actuators at %s is zero",
			TheStation->getSName ());

	if (imode (Flags) == XTRN_IMODE_SOCKET ||
					omode (Flags) == XTRN_OMODE_SOCKET) {
		Assert (imode (Flags) == XTRN_IMODE_SOCKET &&
		    omode (Flags) == XTRN_OMODE_SOCKET, 
			"SENSORS at %s: confilcting modes %x",
			    TheStation->getSName (),
			        Flags);
		// That's it, the devices will be created upon connection
	
	} else if (imode (Flags) == XTRN_IMODE_DEVICE) {
		// Need a mailbox for the input end
		Assert (SID->SIDev != NULL,
			"SENSORS at %s: input device cannot be NULL",
				TheStation->getSName ());
		I = create Dev;
		// Check if the output end uses the same device
		if (omode (Flags) == XTRN_OMODE_DEVICE && strcmp (SID->SIDev,
		    SID->SODev) == 0) {
			// Yep, single mailbox will do
			i = I->connect (DEVICE+READ+WRITE, SID->SIDev, 0,
				(XTRN_MBX_BUFLEN > AUPD_OUTPUT_BUFLEN) ?
					XTRN_MBX_BUFLEN : AUPD_OUTPUT_BUFLEN);
			O = I;
		} else {
			i = I->connect (DEVICE+READ, SID->SIDev, 0,
				XTRN_MBX_BUFLEN);
		}
		if (i == ERROR)
			excptn ("SENSORS at %s: cannot open device %s",
				TheStation->getSName (), SID->SIDev);
	} else if (imode (Flags) == XTRN_IMODE_STRING) {
		Assert (SID->SIDev != NULL,
			"SENSORS at %s: the input string is empty",
				TheStation->getSName ());
		S = SID->SIDev;
	}

	// We are done with the input end
	if (O == NULL && omode (Flags) == XTRN_OMODE_DEVICE) {
		// A separate DEVICE 
		Assert (SID->SODev != NULL,
			"SENSORS at %s: no output device name",
			TheStation->getSName ());
		O = create Dev;
		if (O->connect (DEVICE+WRITE, SID->SODev, 0,
		    AUPD_OUTPUT_BUFLEN))
			excptn ("SENSORS at %s: cannot open device %s",
				TheStation->getSName (), SID->SODev);
	}

	// Allocate buffer for updates
	UBuf = new char [AUPD_OUTPUT_BUFLEN];

	// Note that I shares location with S
	if (I != NULL) {
		// The input end
		create (System) SensorsInput (this);
	}

	if (O != NULL) {
		// Note that this one will be indestructible
		Upd = create SUpdates (MAX_Long);
		// Signal initial (full) update; no, let rst do that
		// qupd_all ();
		create (System) SensorsHandler (this, NULL);
	}
	rst ();
}

SNSRS::~SNSRS () {

	excptn ("SENSORS at %s: cannot delete SENSORS",
		TheStation->getSName ());
}

void SNSRS::read (int state, word sn, address vp) {

	TIME del;
	SensActDesc *s;

	if (sn >= NSensors || (s = Sensors + sn) -> Length == 0)
		syserror (EREQPAR, "read_sensor");

	if (s->MaxTime == 0.0) {
		// return immediately
		s->get (vp);
		return;
	}

	if (undef (s->ReadyTime)) {
		// Starting up
		s->ReadyTime = Time + (del = s->action_time ());
		Timer->wait (del, state);
		sleep;
	}

	if (Time >= s->ReadyTime) {
		s->ReadyTime = TIME_inf;
		s->get (vp);
		return;
	}
	Timer->wait (s->ReadyTime - Time, state);
	sleep;
}

void SNSRS::write (int state, word sn, address vp) {

	TIME del;
	SensActDesc *s;

	if (sn >= NActuators || (s = Actuators + sn) -> Length == 0)
		syserror (EREQPAR, "write_actuator");

	if (s->MaxTime == 0.0)
		// return immediately
		goto SetIt;

	if (undef (s->ReadyTime)) {
		// Starting up
		s->ReadyTime = Time + (del = s->action_time ());
		Timer->wait (del, state);
		sleep;
	}

	if (Time >= s->ReadyTime) {
		s->ReadyTime = TIME_inf;
SetIt:
		if (s->expand (vp)) 
			qupd_act (SEN_TYPE_ACTUATOR, sn);
		return;
	}

	Timer->wait (s->ReadyTime - Time, state);
	sleep;
}

int SNSRS::act_status (byte what, byte sid, Boolean lm) {
/*
 * Transforms an actuator update message into an outgoing message
 */
	double tm;
	char *tb;
	SensActDesc *s;
	char ct;

	tb = UBuf;

	if (what == SEN_TYPE_PARAMS) {
		// Send the number of actuators + the number of sensors
		sprintf (UBuf, "N %1u %1u\n", NActuators, NSensors);
	} else {
		if (what == SEN_TYPE_SENSOR) {
			ct = 'S';
			s = &(Sensors [0]);
		} else {
			ct = 'A';
			s = &(Actuators [0]);
		}
		tm = ituToEtu (Time);
		s += sid;
		if (lm)
			sprintf (UBuf, "%c %08.3f: %1u %08x %08x\n",
				ct, tm, sid, s->Value, s->Max);
		else
			sprintf (UBuf, "%c %08.3f: %1u %08x\n", ct, tm, sid,
				s->Value);
	}
	return strlen (UBuf);
}

int SNSRS::sensor_update (char *rb) {
/*
 * Update the sensor according to the input request. When we are called, rb
 * points to the first non-blank character of the request.
 */
	SensActDesc *s;
	lword sn, sv;
	char c;

	c = *rb++;

	if (c != 'S')
		// Only sensor setting is accepted
		return OK;

	sn = (lword) dechex (rb);
	sv = (lword) dechex (rb);

	if (sn > NSensors)
		return ERROR;

	s = Sensors + sn;

	if (s->Length == 0)
		// This sensor does not exist
		return ERROR;

	if (s->set (sv) && omode (Flags) == XTRN_OMODE_SOCKET) {
		// The value has changed and this is a socket
		qupd_act (SEN_TYPE_SENSOR, sn);
	}

	return OK;
}

void SNSRS::qupd_act (byte tp, byte sn, Boolean lm) {
/*
 * Queue a value update for output
 */
	if (OutputThread == NULL)
		return;

	Upd->queue (tp, sn, lm);
}

void SNSRS::qupd_all () {
/*
 * Queue all updates
 */
	int i;

	// The '1' is to make sure that the request does not look like zero
	// (in case we want to check it against NULL)
	Upd->queue (SEN_TYPE_PARAMS, 1);

	for (i = 0; i < NActuators; i++) {
		if (Actuators [i] . Length)
			qupd_act (SEN_TYPE_ACTUATOR, (byte) i, YES);
	}

	if (omode (Flags) != XTRN_OMODE_SOCKET)
		// Do not send sensor values unless this is a socket
		return;

	for (i = 0; i < NSensors; i++) {
		if (Sensors [i] . Length)
			qupd_act (SEN_TYPE_SENSOR, (byte) i, YES);
	}
}
	
void SensorsHandler::setup (SNSRS *sn, Dev *a) {

	lword c;

	SN = sn;


	if ((Agent = a) == NULL) {
		assert (SN != NULL, "SensorsHandler: no Sensors to handle");
		// We have been called to handle device IO
		assert (SN->O != NULL,
			"SensorsHandler at %s: the mailbox is missing",
				SN->TPN->getSName ());
		assert (SN->OutputThread == NULL,
			"SensorsHandler at %s: agent thread already running",
				SN->TPN->getSName ());
		SN->OutputThread = this;

	} else {
		// This is a socket connection, so we know for a fact that
		// both ends are active; thus, we are responsible for creating
		// the other thread
		c = ECONN_OK;
		if (SN == NULL)
			c = ECONN_NOSENSORS;
		else if (imode (SN->Flags) != XTRN_IMODE_SOCKET)
			c = ECONN_ITYPE;
		else if (SN->OutputThread != NULL)
			c = ECONN_ALREADY;

		// Make sure the buffer size will allow us to accommodate
		// requests and updates
		Agent->resize (XTRN_MBX_BUFLEN > AUPD_OUTPUT_BUFLEN ?
			XTRN_MBX_BUFLEN : AUPD_OUTPUT_BUFLEN);

		if (c != ECONN_OK) {
			create Disconnector (Agent, c);
			terminate ();
		} else {
			Assert (SN->Upd == NULL,
				"SensorsHandler at %s: Upd not NULL",
					SN->TPN->getSName ());
			SN->Upd = create SUpdates (MAX_Long);
			SN->I = SN->O = Agent;
			SN->OutputThread = this;
			SN->InputThread = create SensorsInput (SN);
			SN->qupd_all ();
		}
	}
}

void SensorsHandler::goaway (lword c) {
/*
 * Can also be called by the input thread to kill the output end, but only
 * for a socket connection. It will also terminate the input thread itself,
 * so be careful.
 */
	if (Agent == NULL)
		excptn ("SensorsHandler at %s: error writing to device",
			SN->TPN->getSName ());

	// This is a socket, so clean up and continue
	if (Agent->isActive ())
		create Disconnector (Agent, c);
	else
		delete Agent;

	SN->I = SN->O = NULL;
	if (SN->InputThread != NULL)
		SN->InputThread->terminate ();
	delete SN->Upd;
	SN->Upd = NULL;
	SN->OutputThread = SN->InputThread = NULL;
	this->terminate ();
}

SensorsHandler::perform {
/*
 * This is a close cousin of PinsDriver. We will have to combine them into
 * one set.
 */
	byte tp, sid;
	lword c;
	Boolean lm;

	state AckSens:

		if (Agent != NULL) {
			// Socket connection: acknowledge
			c = htonl (ECONN_OK);
			if (Agent->wi (AckSens, (char*)(&c), 4) == ERROR) {
				// We are disconnected
				goaway (ECONN_DISCONN);
				// This means 'terminate'
				sleep;
			}
		}

	transient Loop:

		// This is the main loop
		if (SN->Upd->empty ()) {
			// No updates
			SN->Upd->wait (NONEMPTY, Loop);
			sleep;
		}


 		lm = SN->Upd->retrieve (tp, sid);
 		Length = SN->act_status (tp, sid, lm);
		Buf = SN->UBuf;

	transient Send:

		if (SN->O->wl (Send, Buf, Length) == ERROR) {
			// Got disconnected
			goaway (ECONN_DISCONN);
			sleep;
		}

		proceed Loop;
}

void SensorsInput::setup (SNSRS *s) {

	SN = s;
	assert (SN->InputThread == NULL, "SensorsInput at %d: duplicate thread",
		SN->TPN->getSName ());
	SN->InputThread = this;

	if (imode (SN->Flags) != XTRN_IMODE_STRING)
		// Otherwise, the mailbox is not a mailbox at all
		SN->I->setSentinel ('\n');
}

SensorsInput::perform {
/*
 * Request formats
 *
 *      T [+]delay (double)  [24]
 *      S num val
 *      FIXME: this MUST be merged with PinsInput
 */
	TIME st;
	double del;
	int rc;
	char *re;
	Boolean off, err;
	char cc;

	state Loop:

		BP = &(RBuf [0]);
		Left = SRQS_INPUT_BUFLEN;

		if (imode (SN->Flags) == XTRN_IMODE_STRING) {
			// This is simple
			while ((SN->Flags & XTRN_IMODE_STRLEN) != 0) {
				SN->Flags--;
				cc = *(SN->S)++;
				if (cc == '\n' || cc == '\0') {
					*BP = '\0';
					goto HandleInput;
				}
				if (Left > 1) {
					*BP++ = cc;
					Left--;
				}
			}
			if (Left == SRQS_INPUT_BUFLEN) {
				// End of string
				SN->InputThread = NULL;
				terminate;
			}
			*BP = '\0';
			goto HandleInput;
		}

	transient ReadRq:

		if ((rc = SN->I->rs (ReadRq, BP, Left)) == ERROR) {
			// We know this is not string
			if ((SN->Flags & XTRN_IMODE_SOCKET) == 0) {
				// A device,  EOF, close this part of the shop
				SN->InputThread = NULL;
				terminate;
			}
			// This is a socket disconnection
			Assert (SN->OutputThread != NULL,
			    	"SensorsInput at %s: sibling thread is gone",
					SN->TPN->getSName ());
			((SensorsHandler*)(SN->OutputThread))->
				goaway (ECONN_DISCONN);
			// We are dead now
			sleep;
		}

		if (rc == REJECTED) {
			// Line too long
			if ((SN->Flags & XTRN_IMODE_SOCKET) == 0) {
				// A device
				excptn ("SensorsHandler at %s: request "
				    "line too long, %1d is the max",
					SN->TPN->getSName (),
					    SRQS_INPUT_BUFLEN);
			}

			((SensorsHandler*)(SN->OutputThread))->
				goaway (ECONN_LONG);
			sleep;
		}

		// Fine: this is the length excluding the newline
		RBuf [SRQS_INPUT_BUFLEN - 1 - Left] = '\0';
HandleInput:
		BP = &(RBuf [0]);

		skipblk (BP);
		if (*BP == 'T') {
			// This is a delay request, which we handle locally
			BP++;
			skipblk (BP);
			if (*BP == '+') {
				// Offset
				off = YES;
				BP++;
			} else
				off = NO;

			del = strtod (BP, &re);
			if (BP == re || del < 0.0) {
Error:
				// This is an error
				if ((SN->Flags & XTRN_IMODE_SOCKET) == 0) {
				    excptn ("SensorsHandler at %s: illegal "
					"request '%s'", SN->TPN->getSName (),
						RBuf);
				} else {
				    // This may be temporary (FIXME)
				    trace ("SensorsHandler at %s: illegal "
					"request '%s'", SN->TPN->getSName (),
						RBuf);
				    ((SensorsHandler*)(SN->OutputThread))->
					goaway (ECONN_INVALID);
				    sleep;
				}
			}
			st = etuToItu (del);
			if (off)
				SN->TimedRequestTime += st;
			else
				SN->TimedRequestTime = st;

			proceed Delay;
		}

		if (SN->sensor_update (BP) == ERROR)
			goto Error;
			
		proceed Loop;

	state Delay:

		if (SN->TimedRequestTime > Time) {
			// May put in here some mailbox monitoring
			// code ???
			Timer->wait (SN->TimedRequestTime - Time, Delay);
			sleep;
		} else
			SN->TimedRequestTime = Time;

		proceed Loop;
}

LEDSM::LEDSM (data_le_t *le) {

	PicOSNode *ThisNode;

	NLeds = le->NLeds;

	Assert (NLeds > 0 && NLeds <= 32,
		"LEDSM at %s: the number of leds (%1d) must be > 0 and <= 64",
			TheStation->getSName (), NLeds);

	// Calculate the update message size:
	//
	// time [0/1] xxxxxxxxxxxxxxx\n
	// 9 s 1 s NLeds EOL	= NLeds + 13
	//

	OUpdSize = NLeds + 16;	// Round it up

	UBuf = new char [OUpdSize];

	if (le->LODev != NULL) {
		// Device output
		Device = YES;
		O = create Dev;
		if (O->connect (DEVICE+WRITE, le->LODev, 0, OUpdSize) == ERROR)
			excptn ("LEDSM at %s: cannot open device %s",
			    TheStation->getSName (), le->LODev);
	} else {
		// Socket
		Device = NO;
		O = NULL;
	}

	LStat = new byte [(NLeds+1)/2];

	if (O != NULL) {
		ThisNode = ThePicOSNode;
		OutputThread = create (System) LedsHandler (ThisNode, this,
			NULL);
	} else
		OutputThread = NULL;
	rst ();
}

void LEDSM::rst () {

	word i;

	for (i = 0; i < NLeds; i++)
		setstat (i, LED_OFF);

	Changed = YES;
	Fast = NO;

	if (OutputThread != NULL)
		OutputThread->signal (NULL);
}

LEDSM::~LEDSM () {

	excptn ("LEDSM at %s: cannot delete LED modules",
		TheStation->getSName ());
}

void LEDSM::leds_op (word led, word op) {

	if (led >= NLeds)
		// Ignore
		return;

	Assert (op < 3, "LEDSM at %s: illegal operation %1d",
		TheStation->getSName (), op);

	if (getstat (led) != op) {
		setstat (led, op);
		Changed = YES;
		if (OutputThread != NULL)
			OutputThread->signal (NULL);
	}
}

void LEDSM::fastblink (Boolean on) {

	if (on != Fast) {
		Fast = on;
		Changed = YES;
		if (OutputThread != NULL)
			OutputThread->signal (NULL);
	}
}

int LEDSM::ledup_status () {
/*
 * Prepare an update message
 */
	
	int i, len;

	sprintf (UBuf, "U %08.3f: %c ", ituToEtu (Time), Fast ? '1' : '0');

	for (len = strlen (UBuf), i = 0; i < NLeds; i++)
		UBuf [len++] = (char) (getstat (i) + '0');

	UBuf [len++] = '\n';

	return len;
}

void LedsHandler::setup (PicOSNode *tpn, LEDSM *le, Dev *a) {

	lword c;

	TPN = tpn;

	if ((Agent = a) == NULL) {
		// We are handling file output: sanity checks
		LE = le;
		assert (LE->O != NULL,
			"LedsHandler at %s: the mailbox is missing",
				TPN->getSName ());
		assert (LE->OutputThread == NULL,
			"LedsHandler at %s: agent thread already running",
				TPN->getSName ());
		LE->OutputThread = this;
	} else {
		// This is a socket connection, Node structure already present
		LE = tpn->ledsm;
		c = ECONN_OK;
		if (LE == NULL)
			c = ECONN_NOLEDS;
		else if (LE->Device)
			c = ECONN_ITYPE;
		else if (LE->OutputThread != NULL)
			c = ECONN_ALREADY;


		if (c != ECONN_OK) {
			create Disconnector (Agent, c);
			terminate ();
		} else {
			// Make sure the buffer size will allow us to
			// accommodate requests and updates
			Agent->resize (LE->OUpdSize);
			LE->O = Agent;
			LE->OutputThread = this;
			LE->Changed = YES;
		}
	}
}

LedsHandler::perform {
/*
 * We handle both file output and socket output. FIXME: I am not sure whether
 * we will be able to detect disconnection (with only one end of the socket)
 * without some feedback in the opposite direction. Will check it out.
 */
	lword c;

	state AckLeds:

		if (Agent != NULL) {
			// Socket connection: acknowledge
 			c = htonl (ECONN_OK);
 			if (Agent->wi (AckLeds, (char*)(&c), 4) == ERROR) {
				// We are disconnected
Disconnect:
				delete Agent;
				LE->O = NULL;
				LE->OutputThread = NULL;
				terminate;
			}
		}

	transient Loop:

		// This is the main loop
		if (!(LE->Changed)) {
			if (Agent != NULL)
				// Periodic NOPs, if socket
				Timer->delay (10.0, SendNop);
			this->wait (SIGNAL, Loop);
			sleep;
		}

		LE->Changed = NO;
		Length = LE->ledup_status ();
		Buf = LE->UBuf;

	transient Send:

		if (LE->O->wl (Send, Buf, Length) == ERROR) {
Error:
			if (Agent == NULL)
				excptn ("LedsHandler at %s: error writing to "
					"device", TPN->getSName ());
			// Disconnection
			goto Disconnect;
		}

		proceed Loop;

	state SendNop:

		if (LE->O->wi (SendNop, "\n", 1) == ERROR)
			// FIXME: check if this actually detects disconnections
			goto Error;

		proceed Loop;
}

// ============================================================================

void LcdgHandler::setup (PicOSNode *tpn, Dev *a) {

	lword c;

	TPN = tpn;

	// Note: we are never called with a == NULL, as a device (file)
	// connection is handled separately (and can in fact coexist with
	// a socket connection)

	Agent = a;
	LC = tpn->lcdg;
	c = ECONN_OK;
	if (LC == NULL)
		c = ECONN_NOLCDG;
	else if (LC->OutputThread != NULL)
		c = ECONN_ALREADY;
	if (c != ECONN_OK) {
		create Disconnector (Agent, c);
		terminate ();
	} else {
		Agent->resize (LCDG_OUTPUT_BUFLEN);
		LC->init_connection (this);
	}
	// The NOP command
	WNOP = htons (0x1000);
}

LcdgHandler::perform {
//
// We only handle socket output
//
	lword c;
	lcdg_update_t *cu;

	state AckLcdg:

		c = htonl (ECONN_OK);
 		if (Agent->wi (AckLcdg, (char*)(&c), 4) == ERROR) {
			// We are disconnected
Disconnect:
			delete Agent;
			LC->close_connection ();
			terminate;
		}

	transient Loop:

		// This is the main loop
		if (LC->UHead == NULL) {
			// Periodic NOPs
			Timer->delay (10.0, SendNop);
			this->wait (SIGNAL, Loop);
			sleep;
		}

		// This is in words
		Length = LC->UHead->Size;
		Buf = LC->UHead->Buf;

		// Convert to network format
#if BYTE_ORDER == LITTLE_ENDIAN
		{
			int i;
			for (i = 0; i < Length; i++)
				Buf [i] = htons (Buf [i]);
		}
#endif
		// Make it bytes
		Length <<= 1;

	transient Send:

		// Can we get away with wi instead of wl? Yep, we can.
		if (Agent->wi (Send, (const char*)Buf, Length) == ERROR)
			// Assume disconnection
			goto Disconnect;

		// Deallocate the head
		LC->UHead = (cu = LC->UHead)->Next;
		delete [] (byte*) cu;
		proceed Loop;

	state SendNop:

		if (Agent->wi (SendNop, (const char*)(&WNOP), 2) == ERROR)
			goto Disconnect;

		proceed Loop;
}
// ============================================================================

void MoveHandler::setup (Dev *a, FLAGS f) {

	int i;

	Flags = f;
	RBuf = NULL;	// Used as flag by the destructor

	if (imode (Flags) == XTRN_IMODE_SOCKET) {
		if (MUP != NULL) {
			// For now allow only one Internet connection at a time
			create Disconnector (a, ECONN_ALREADY);
			terminate ();
			return;
		}
		// Create the mailbox to receive updates
		MUP = create MUpdates (MAX_Long);
		rwpmmSetNotifier (mup_update);
	}

	Agent = a;

	if (imode (Flags) != XTRN_IMODE_STRING)
		Agent->setSentinel ('\n');
	TimedRequestTime = Time;
	// This will do for input requests, but we have to play it
	// safe with the output stuff, which includes node type names,
	// as there is no explicit limit on their length
	RBuf = new char [RBSize = MRQS_INPUT_BUFLEN];
}

MoveHandler::~MoveHandler () {

	if (RBuf != NULL) {
		// We have managed to create some stuff
		delete [] RBuf;
		if (imode (Flags) == XTRN_IMODE_SOCKET) {
			delete MUP;
			MUP = NULL;
			rwpmmSetNotifier (NULL);
		}
	}
}

MoveHandler::perform {

	TIME st;
	double xx, yy;
	nparse_t NP [9];
	int rc;
	Long NN, NS;
	char *re;
	PicOSNode *pn;
	Transceiver *TR;
	lword c;
	char cc;
	Boolean off;

	state AckMove:

		if (imode (Flags) == XTRN_IMODE_SOCKET) {
			// Need to acknowledge
 			c = htonl (ECONN_OK);
 			if (Agent->wi (AckMove, (char*)(&c), 4) == ERROR) {
				// Disconnected
				delete Agent;
				terminate;
			}
		}

	transient Loop:

		BP = RBuf;
		Left = MRQS_INPUT_BUFLEN;

		if (imode (Flags) == XTRN_IMODE_STRING) {
			// Reading from string
			while ((Flags & XTRN_IMODE_STRLEN) != 0) {
				Flags--;
				cc = *(String)++;
				if (cc == '\n' || cc == '\0') {
					*BP = '\0';
					goto HandleInput;
				}
				if (Left > 1) {
					*BP++ = cc;
					Left--;
				}
			}
			if (Left == MRQS_INPUT_BUFLEN) {
				// End of string
				terminate;
			}
			*BP = '\0';
			goto HandleInput;
		}

	transient ReadRq:

		if (imode (Flags) == XTRN_IMODE_SOCKET && 
						Left == MRQS_INPUT_BUFLEN) {
			// This is extremely clumsy; the process asks to
			// be completely reorganized. This condition tells
			// us that we haven't started reading yet and we
			// are an Internet mover, in which case MUP must
			// be present.
			if (!MUP->empty ()) {
				NN = MUP->get ();
				pn = (PicOSNode*) idToStation (NN);
				pn -> get_location (xx, yy);
				// This one is safe as we do not include the
				// standard name in an update
				sprintf (RBuf, "U %1ld %1f %1f\n", NN, xx, yy);
				BP = &(RBuf [0]);
				Left = strlen (RBuf);
				proceed Reply;
			}

			MUP->wait (NONEMPTY, ReadRq);
		}

		if ((rc = Agent->rs (ReadRq, BP, Left)) == ERROR) {
			if (imode (Flags) == XTRN_IMODE_SOCKET)
				delete Agent;
			// EOF. That's it. We cannot delete the mailbox
			// because it formally belongs to a station.
			terminate;
		}

		if (rc == REJECTED) {
			if (imode (Flags) != XTRN_IMODE_SOCKET)
				excptn ("MoveHandler: request line too long, "
					"%1d is the max", MRQS_INPUT_BUFLEN);
			create Disconnector (Agent, ECONN_LONG);
			terminate;
		}
			
		// this is the length excluding the newline
		RBuf [MRQS_INPUT_BUFLEN - 1 - Left] = '\0';
HandleInput:
		// Request format:
		// T delay
		// Q node (and nothing else) -> request for coordinates, illegal
		// if device;
		// M node x y -> teleport there immediately
		// R node x0 y0 x1 y1 mins maxs minp maxp time -> starts a
		// random mobility process for the node

		BP = RBuf;
		skipblk (BP);

		switch (*BP++) {

		    case 'T':

			// Delay request
			BP++;
			skipblk (BP);
			if (*BP == '+') {
				// Offset
				off = YES;
				BP++;
			} else
				off = NO;
			xx = strtod (BP, &re);
			if (BP == re || xx < 0.0) {
				// Illegal request
Illegal:
				if (imode (Flags) != XTRN_IMODE_SOCKET)
					excptn ("MoveHandler: illegal request "
						"%s", RBuf);
				create Disconnector (Agent, ECONN_INVALID);
				terminate;
			}
			st = etuToItu (xx);
			if (off)
				TimedRequestTime += st;
			else
				TimedRequestTime = st;

			proceed Delay;

		    case 'Q':

			// Position query
			if ((NN = dechex (BP)) == ERROR)
				goto Illegal;
			// trace ("QQQ: %1d", NN);

			if (imode (Flags) != XTRN_IMODE_SOCKET)
				excptn ("MoveHandler: coordinate query '%s; "
					"illegal from non-socket interface",
						RBuf);

			if (!isStationId (NN)) {
Illegal_nid:
				if (imode (Flags) != XTRN_IMODE_SOCKET)
					excptn ("MoveHandler: illegal node Id "
						"%1d", NN);
				// Socket, NACK + disconnect
				create Disconnector (Agent, ECONN_STATION);
				terminate;
			}

			pn = (PicOSNode*)idToStation (NN);
			pn -> get_location (xx, yy);

			while ((rc = snprintf (RBuf, RBSize,
			   "P %1ld %1ld %1f %1f %s\n", NN, NStations, xx, yy,
			      pn->getTName ())) >= RBSize) {
				// Must grow the buffer
				RBSize = (word)(rc + 1);
				delete [] RBuf;
				RBuf = new char [RBSize];
			}

			BP = &(RBuf [0]);
			Left = strlen (RBuf);
			proceed Reply;

		    case 'M':

			// Move
			if ((NN = dechex (BP)) == ERROR)
				goto Illegal;

			if (!isStationId (NN))
				goto Illegal_nid;

			// Prepare the parse structure
			NP [0] . type = NP [1] . type = TYPE_double;

			if ((rc = parseNumbers (BP, 2, NP)) != 2)
				goto Illegal;

			if (NP [0].DVal < 0.0 || NP [1].DVal < 0.0) {
Illegal_crd:
				if (imode (Flags) != XTRN_IMODE_SOCKET)
					excptn ("MoveHandler: illegal "
					    "coordinates (%f, %f), must not be "
						"negative", NP[0].DVal,
						    NP[1].DVal);
				// Socket
				create Disconnector (Agent, ECONN_INVALID);
				terminate;
			}
			// trace ("MMM: %1d %f %f", NN, NP [0].DVal, NP [1].DVal);

			pn = (PicOSNode*)idToStation (NN);
			if (pn->RFInt != NULL) {
				TR = pn->RFInt->RFInterface;
				// Cancel any present movement
				rwpmmStop (TR);
				TR -> setLocation (NP [0].DVal, NP [1].DVal);
			}
			// Send the update regardless, you will get 0,0 for a
			// transceiver-less node
			if (MUP != NULL)
				MUP->queue (NN);

			proceed Loop;

		    case 'R':

			// Start roaming
			if ((NN = dechex (BP)) == ERROR)
				goto Illegal;

			if (!isStationId (NN))
				goto Illegal_nid;

			for (rc = 0; rc < 9; rc++)
				NP [rc] . type = TYPE_double;

			if ((rc = parseNumbers (BP, 9, NP)) != 9)
				goto Illegal;

			if (NP [0].DVal < 0.0 || NP [1].DVal < 0.0)
				goto Illegal_crd;

			// More validation of arguments: the rectangle
			if (NP [2].DVal < NP [0].DVal || NP [3].DVal <
			    NP [1].DVal) {
				if (imode (Flags) != XTRN_IMODE_SOCKET)
				    excptn ("MoveHandler: illegal rectangle "
					"coordinates "
					"<%f,%f> <%f,%f>", NP[0].DVal,
					   NP[1].DVal, NP[2].DVal, NP[3].DVal);
				// Socket
				create Disconnector (Agent, ECONN_INVALID);
				terminate;
			}

			if (NP [4].DVal <= 0.0 || NP [5].DVal < NP [4].DVal) {
				if (imode (Flags) != XTRN_IMODE_SOCKET)
					excptn ("MoveHandler: illegal speed "
					"parameters "
					"(%f,%f)", NP[4].DVal, NP[5].DVal);
				// Socket
				create Disconnector (Agent, ECONN_INVALID);
				terminate;
			}

			if (NP [6].DVal < 0.0 || NP [7].DVal < NP [6].DVal) {
				if (imode (Flags) != XTRN_IMODE_SOCKET)
					excptn ("MoveHandler: illegal pause "
					"parameters "
					"(%f,%f)", NP[6].DVal, NP[7].DVal);
				// Socket
				create Disconnector (Agent, ECONN_INVALID);
				terminate;
			}

			// The time can be negative, which (including zero)
			// means that we roam forever

			// Do not roam stations devoid of RF interface
			pn = (PicOSNode*) idToStation (NN);
			if (pn->RFInt != NULL)
				rwpmmStart (NN, pn->RFInt->RFInterface,
					NP [0].DVal, NP [1].DVal,
					NP [2].DVal, NP [3].DVal,
					NP [4].DVal, NP [5].DVal,
					NP [6].DVal, NP [7].DVal,
					NP [8].DVal);

			// Just ignore otherwise
			proceed Loop;

		    case '\0':

			// Empty line
			proceed Loop;

		}

		if (imode (Flags) != XTRN_IMODE_SOCKET)
			excptn ("MoveHandler: illegal request '%s'", BP);

		proceed Loop;
	
	state Reply:

		if (Agent->wl (Reply, BP, Left) != ERROR)
			proceed Loop;

		// Disconnection (this can only be a socket)
		delete Agent;
		terminate;

	state Delay:

		if (TimedRequestTime > Time) {
			// FIXME: what if waiting erroneously long??
			Timer->wait (TimedRequestTime - Time, Delay);
			sleep;
		} else
			TimedRequestTime = Time;

		proceed Loop;
}

void PanelHandler::setup (Dev *a, FLAGS f) {

	Flags = f;
	RBuf = NULL;	// A flag for the destructor

	if (imode (Flags) == XTRN_IMODE_SOCKET) {
		if (PUP != NULL) {
			// Allow only one connection at a time
			create Disconnector (a, ECONN_ALREADY);
			terminate ();
			return;
		}
		// Create the mailbox for updates; note: the only reason we
		// need it is that nodes may halt by themselves
		PUP = create MUpdates (MAX_Long);
	}

	Agent = a;

	if (imode (Flags) != XTRN_IMODE_STRING)
		Agent->setSentinel ('\n');

	TimedRequestTime = Time;

	RBuf = new char [RBSize = ARQS_INPUT_BUFLEN];
}

PanelHandler::~PanelHandler () {

	if (RBuf) {

		delete [] RBuf;

		if (imode (Flags) == XTRN_IMODE_SOCKET) {
			delete PUP;
			PUP = NULL;
		}
	}
}

PanelHandler::perform {

	TIME st;
	double d;
	PicOSNode *pn;
	char *re;
	Long NN;
	int rc;
	lword c;
	char cc;
	Boolean off;

	state AckPanel:

		// Done only once, if ever, upon startup

		if (imode (Flags) == XTRN_IMODE_SOCKET) {
			// Need to acknowledge
 			c = htonl (ECONN_OK);
 			if (Agent->wi (AckPanel, (char*)(&c), 4) == ERROR) {
				// Disconnected
				delete Agent;
				terminate;
			}
		}

	transient Loop:

		BP = RBuf;
		Left = ARQS_INPUT_BUFLEN;

		if (imode (Flags) == XTRN_IMODE_STRING) {
			// Reading from string
			while ((Flags & XTRN_IMODE_STRLEN) != 0) {
				Flags--;
				cc = *(String)++;
				if (cc == '\n' || cc == '\0') {
					*BP = '\0';
					goto HandleInput;
				}
				if (Left > 1) {
					*BP++ = cc;
					Left--;
				}
			}
			if (Left == ARQS_INPUT_BUFLEN) {
				// End of string
				terminate;
			}
			*BP = '\0';
			goto HandleInput;
		}

	transient ReadRq:

		if (imode (Flags) == XTRN_IMODE_SOCKET && 
						Left == ARQS_INPUT_BUFLEN) {
			// We haven't started reading yet and we run from
			// a socket; thus PUP must be present
			if (!PUP->empty ()) {
				NN = PUP->get ();
				sprintf (RBuf, "%1ld %c\n", NN,
					((PicOSNode*)idToStation (NN))->Halted ?
						'F' : 'O');
				BP = &(RBuf [0]);
				Left = strlen (RBuf);
				proceed Reply;
			}
			PUP->wait (NONEMPTY, ReadRq);
		}

		if ((rc = Agent->rs (ReadRq, BP, Left)) == ERROR) {
			if (imode (Flags) == XTRN_IMODE_SOCKET)
				delete Agent;
			// Cannot delete station's mailbox
			terminate;
		}

		if (rc == REJECTED) {
			if (imode (Flags) != XTRN_IMODE_SOCKET)
				excptn ("PanelHandler: request line too long, "
					"%1d is the max", ARQS_INPUT_BUFLEN);
			create Disconnector (Agent, ECONN_LONG);
			terminate;
		}
			
		// this is the length excluding the newline
		RBuf [ARQS_INPUT_BUFLEN - 1 - Left] = '\0';
HandleInput:
		// Request format:
		// T delay
		// Q node -> request for name and status,
		// O node -> on
		// F node -> off
		// R node -> Reset

		BP = RBuf;
		skipblk (BP);

		switch (*BP++) {

		    case 'T':

			// Delay request
			BP++;
			skipblk (BP);
			if (*BP == '+') {
				// Offset
				off = YES;
				BP++;
			} else
				off = NO;
			d = strtod (BP, &re);
			if (BP == re || d < 0.0) {
				// Illegal request
Illegal:
				if (imode (Flags) != XTRN_IMODE_SOCKET)
					excptn ("PanelHandler: illegal request "
						"%s", RBuf);
				create Disconnector (Agent, ECONN_INVALID);
				terminate;
			}
			st = etuToItu (d);
			if (off)
				TimedRequestTime += st;
			else
				TimedRequestTime = st;

			proceed Delay;

		    case 'Q':

			// Status query
			if ((NN = dechex (BP)) == ERROR)
				goto Illegal;

			if (imode (Flags) != XTRN_IMODE_SOCKET)
				excptn ("PanelHandler: node query '%s; "
					"illegal from non-socket interface",
						RBuf);

			if (!isStationId (NN)) {
Illegal_nid:
				if (imode (Flags) != XTRN_IMODE_SOCKET)
					excptn ("PanelHandler: illegal node Id "
						"%1d", NN);
				// Socket, NACK + disconnect
				create Disconnector (Agent, ECONN_STATION);
				terminate;
			}

			pn = (PicOSNode*)idToStation (NN);

			while ((rc = snprintf (RBuf, RBSize,
			    "%1ld %c %1ld %s\n",
			    NN,
			    pn->Halted ?  'F' : 'O',
			    NStations,
			    pn->getTName ())) >= RBSize) {
				// Must grow the buffer
				RBSize = (word)(rc + 1);
				delete [] RBuf;
				RBuf = new char [RBSize];
			}

			BP = &(RBuf [0]);
			Left = strlen (RBuf);
			proceed Reply;

		    case 'O':

			// ON
			if ((NN = dechex (BP)) == ERROR)
				goto Illegal;

			if (!isStationId (NN))
				goto Illegal_nid;

			pn = (PicOSNode*)idToStation (NN);

			if (pn->Halted) {
				// Execute this in the right context
				TheStation = pn;
Init:
				pn->reset ();
				pn->init ();
				pn->Halted = NO;
				zz_panel_signal (NN);
				TheStation = System;
			}

			proceed Loop;

		    case 'F':

			// OFF
			if ((NN = dechex (BP)) == ERROR)
				goto Illegal;

			if (!isStationId (NN))
				goto Illegal_nid;

			pn = (PicOSNode*)idToStation (NN);

			if (!pn->Halted) {
				TheStation = pn;
				// This sets Halted
				pn->stopall ();
				zz_panel_signal (NN);
				TheStation = System;
			}

			proceed Loop;

		    case 'R':

			// Reset
			if ((NN = dechex (BP)) == ERROR)
				goto Illegal;

			if (!isStationId (NN))
				goto Illegal_nid;

			pn = (PicOSNode*)idToStation (NN);

			TheStation = pn;

			if (pn->Halted)
				goto Init;

			pn->stopall ();
			pn->reset ();
			pn->Halted = NO;
			pn->init ();
			TheStation = System;

			proceed Loop;
				
		    case '\0':

			// Empty line
			proceed Loop;

		}

		if (imode (Flags) != XTRN_IMODE_SOCKET)
			excptn ("PanelHandler: illegal request '%s'", BP);

		proceed Loop;
	
	state Reply:

		if (Agent->wl (Reply, BP, Left) != ERROR)
			proceed Loop;

		// Disconnection (this can only be a socket)
		delete Agent;
		terminate;

	state Delay:

		if (TimedRequestTime > Time) {
			// FIXME: what if waiting erroneously long??
			Timer->wait (TimedRequestTime - Time, Delay);
			sleep;
		} else
			TimedRequestTime = Time;

		proceed Loop;
}

void AgentConnector::setup (Dev *m) {

	Agent = create Dev;

	if (Agent->connect (m, XTRN_MBX_BUFLEN) != OK) {
		// Failed
		delete Agent;
		terminate ();
	}
}

AgentConnector::perform {

	rqhdr_t	header;
	int	st;
	PicOSNode *TPN;

	state Init:

		// Must receive the block within this interval
		Timer->delay (MILLISECOND * CONNECTION_TIMEOUT, GiveUp);

		if (Agent->ri (Init, (char*)(&header), sizeof (rqhdr_t)) ==
		    ERROR) {
			// This means we are disconnected
			delete Agent;
			terminate;
		}

		// Sanity check
		if (header.magic != AGENT_MAGIC) {
			create Disconnector (Agent, ECONN_MAGIC);
			terminate;
		}

		st = ntohl (header.stnum);
		TPN = isStationId (st) ? (PicOSNode*) idToStation (st) : NULL;

		switch (ntohs (header.rqcod)) {

			case AGENT_RQ_UART:

				if (TPN == NULL)
					create Disconnector (Agent,
						ECONN_STATION);
				else
					create UartHandler (TPN, Agent);
				terminate;

			case AGENT_RQ_PINS:

				if (TPN == NULL)
					create Disconnector (Agent,
						ECONN_STATION);
				else
					create PinsHandler (TPN->pins, Agent);
				terminate;

			case AGENT_RQ_SENSORS:

				if (TPN == NULL)
					create Disconnector (Agent,
						ECONN_STATION);
				else
					create SensorsHandler (TPN->snsrs,
						Agent);
				terminate;

			case AGENT_RQ_LEDS:

				if (TPN == NULL)
					create Disconnector (Agent,
						ECONN_STATION);
				else
					create LedsHandler (TPN, NULL, Agent);
				terminate;

			case AGENT_RQ_MOVE:

				create MoveHandler (Agent, XTRN_IMODE_SOCKET);
				terminate;

			case AGENT_RQ_CLOCK:

				create ClockHandler (Agent);
				terminate;

			case AGENT_RQ_PANEL:

				create PanelHandler (Agent, XTRN_IMODE_SOCKET);
				terminate;

			case AGENT_RQ_LCDG:

				// Note: the write-to-file option will be
				// handled separately
				create LcdgHandler (TPN, Agent);
				terminate;

			// More options will come later
			default:
				create Disconnector (Agent, ECONN_UNIMPL);
		}
		terminate;

	state GiveUp:

		create Disconnector (Agent, ECONN_TIMEOUT);
		terminate;
}

Disconnector::perform {

	state Send:

		// Try to disconnect softly, so as to convey a NACK to the
		// other party, if only possible. But never hang around
		// indefinitely.
		Timer->delay (MILLISECOND * SHORT_TIMEOUT, Kill);
		if (Agent->wi (Send, (char*)(&code), 4) == ERROR)
			goto Term;

	transient Wait:

		// In this state we wait for the output to be flushed; normally,
		// we need a single pass through the scheduler
		if (!Agent->isActive () || !Agent->outputPending ())
			goto Term;
		Agent->wait (OUTPUT, Wait);
		Timer->delay (MILLISECOND * SHORT_TIMEOUT, Kill);

	state Kill:
Term:
		delete Agent;
		terminate;
}

void AgentInterface::setup () {

	M = create Dev;
	if (M->connect (INTERNET + SERVER + MASTER, ZZ_Agent_Port) != OK)
		excptn ("AgentInterface: cannot set up master socket");
}

AgentInterface::perform {

	state WaitConn:

		if (M->isPending ()) {
			create AgentConnector (M);
			proceed WaitConn;
		}

		M->wait (UPDATE, WaitConn);
}

#endif
