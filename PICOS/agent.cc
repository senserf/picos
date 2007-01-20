#ifndef __agent_c__
#define __agent_c__

// External agent interface

#include "board.h"
#include "agent.h"
//#include "stdattr.h"

//#undef	rnd

process	Teleporter;

struct	movepool_s {

	struct movepool_s *next, *prev;

	Teleporter *Thread;
	Long NNumber;
};

typedef	struct movepool_s movepool_t;

static movepool_t *MIP = NULL;		// List of moves in progress

static movepool_t *find_mip (Long sid) {

	movepool_t *p;

	for (p = MIP; p != NULL; p = p->next)
		if (p->NNumber == sid)
			return p;
	return NULL;
}

static inline double dist (double x, double y, double xt, double yt) {

	return sqrt ((xt - x) * (xt - x) + (yt - y) * (yt - y));
}

static void push_bval (char *&tb, byte val) {
/*
 * Two hex digits + leading space
 */
	int d;

	*tb++ = ' ';
	d = (val >> 4) & 0xf;
	*tb++ = tohex (d);
	d = val & 0xf;
	*tb++ = tohex (d);
}

static void push_pval (char *&tb, byte *pns, int ns) {
/*
 * A hex string of binary pin values
 */
	int i, d;

	*tb++ = ' ';
	for (i = 0; i < ns; i++) {
		d = pns [i >> 1];
		if ((i & 1))
			d = (d >> 4);
		d &= 0xf;
		*tb++ = tohex (d);
	}
}

static void push_sval (char *&tb, short val) {

	int i, d;

	*tb++ = ' ';
	for (i = 12; i >= 0; i -= 4) {
		d = (val >> i) & 0xf;
		*tb++ = tohex (d);
	}
}

static void push_msec (char *&tb, TIME t) {

	double d;
	Long s, m;

	// This gives us seconds
	s = (Long)(d = ituToEtu (t));
	// This gives us milliseconds
	m = (Long)((d - s) * 1000.0);

	if (s >= 10000)
		s = s % 10000;

	sprintf (tb, "%04d.%03d: ", s, m);
	tb += 10;
}

inline static void skipblk (char *&bp) {
	while (isspace (*bp))
		bp++;
}

static int dechex (char *&bp) {

	int res;

	skipblk (bp);

	if (!isxdigit (*bp))
		return ERROR;

	res = 0;

	do {
		res = (res << 4) | unhex (*bp);
		bp++;
	} while (isxdigit (*bp));

	return res;
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
	byte	code;

	states { Send, Wait, Kill };

	void setup (Dev *a, char c) { Agent = a; code = c; };

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

	void goaway (byte);

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

	void setup (PINS*, Process**, byte, Long, Long);

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

process MoveHandler {

	TIME TimedRequestTime;
	Dev *Agent;
	int Left;
	char *BP;
	nparse_t NP [5];
	char *RBuf;
	word RBSize;

	Boolean	Device;

	states { AckMove, Loop, ReadRq, Reply, Delay };

	void setup (Dev*, Boolean);
	~MoveHandler () { delete RBuf; };

	perform;
};

process Teleporter {

	movepool_t *ME;
	PicOSNode *No;
	double CX, CY, TX, TY, TLeft;
	Long Count;

	states { Advance };

	// pool item, X, Y, steps, total time
	void setup (movepool_t*, double, double, Long, double);

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
	int st, imode, omode;
	Boolean R, W;

	R = W = NO;
	I = O = NULL;
	Flags = UAD->UMode;
	PI = NULL;
	PO = NULL;
	B_len = UAD->UBSize;
	IBuf = OBuf = NULL;
	String = NULL;
	TI_aux = NULL;
	TimedChunkTime = TIME_0;

	imode = (Flags & XTRN_IMODE_MASK);
	omode = (Flags & XTRN_OMODE_MASK);

	// Account for start / stop bits, assuming there is no parity and one
	// stop bit, which is the case in all our setups
	ByteTime = (TIME) ((Second / UAD->URate) * 10.0);

	if (imode == XTRN_IMODE_SOCKET || omode == XTRN_OMODE_SOCKET) {
		// The socket case: both ends must use the same socket
		Assert (imode == XTRN_IMODE_SOCKET &&
		    omode == XTRN_OMODE_SOCKET, 
			"UART at %s: confilcting modes %x",
			    TheStation->getSName (),
			        Flags);
		// Don't create the device now; it will be created upon
		// connection. That's it for now.
		R = W = YES;
	} else if (imode == XTRN_IMODE_DEVICE) {
		// Need a mailbox for the input end
		Assert (UAD->UIDev != NULL,
			"UART at %s: input device cannot be NULL",
				TheStation->getSName ());
		R = YES;
		I = create Dev;
		// Check if the output end uses the same device
		if (omode == XTRN_OMODE_DEVICE &&
		    strcmp (UAD->UIDev, UAD->UODev) == 0) {
			// Yep, single mailbox will do
			st = I->connect (DEVICE+READ+WRITE, UAD->UIDev, 0,
					XTRN_MBX_BUFLEN);
			O = I;
			W = YES;
		} else {
			st = I->connect (DEVICE+READ, UAD->UIDev, 0,
				XTRN_MBX_BUFLEN);
		}
		if (st == ERROR)
			excptn ("UART at %s: cannot open device %s",
				TheStation->getSName (), UAD->UIDev);
	} else if (imode == XTRN_IMODE_STRING) {
		// String
		Assert (UAD->UIDev != NULL,
			"UART at %s: the input string is empty",
				TheStation->getSName ());
		SLen = (Flags & XTRN_IMODE_STRLEN);
		if (SLen == 0)
			// Use strlen
			SLen = strlen (UAD->UIDev);
		if (SLen == 0)
			excptn ("UART at %s: input string length is 0",
				TheStation->getSName ());
		// No need to copy the string; it has been allocated as
		// shared by readNodeParams
		String = UAD->UIDev;
		R = YES;
	}

	// We are done with the input end
	if (O == NULL && omode == XTRN_OMODE_DEVICE) {
		// This can only mean a separate DEVICE 
		Assert (UAD->UODev != NULL, "UART at %s: no output device name",
			TheStation->getSName ());
		W = YES;
		O = create Dev;
		if (O->connect (DEVICE+WRITE, UAD->UODev, 0, XTRN_MBX_BUFLEN))
			excptn ("UART at %s: cannot open device %s",
			    	TheStation->getSName (), UAD->UODev);
	}

	if (R)
		IBuf = new byte [B_len];

	if (W)
		OBuf = new byte [B_len];

	rst ();
}

void UART::rst () {

	if (IBuf != NULL) {
		IB_in = IB_out = 0;
		PI = create UART_in (this);
	}

	if (OBuf != NULL) {
		OB_in = OB_out = 0;
		PO = create UART_out (this);
	}
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

	if ((Flags & XTRN_IMODE_MASK) == XTRN_IMODE_STRING) {
		if (SLen == 0) {
			// This event will never happen. We wait here forever
			// when the string has ended. Perhaps later I will do
			// something about the process.
			terminate (PI);
			PI = NULL;
			sleep;
		}
		SLen--;
		return *String++;
	}
	// Read from the mailbox
	if (I == NULL) {
		// This can only mean that we are reading from a socket that
		// isn't there yet. Hold on.
		Assert ((Flags & XTRN_IMODE_MASK) == XTRN_IMODE_SOCKET,
			"UART at %s: mailbox pointer is NULL",
				TheStation->getSName ());
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
				delete TI_aux;
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
			// Just drop the stuff
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

	if (IBuf != NULL)
		delete IBuf;

	if (OBuf != NULL)
		delete OBuf;

	// if (String != NULL)
		// delete String;
	// String may be shared

	if (TI_aux != NULL)
		delete TI_aux;
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

	byte c;

	Agent = a;
	UA = tpn->uart->U;

	// Make sure we don't belong to the node. That would get us killed
	// upon reset.

	c = ECONN_OK;

	if (UA == NULL) {
		// No UART for this station
		c = ECONN_NOUART;
	} else if ((UA->Flags & XTRN_IMODE_SOCKET) == 0) {
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
	byte 	c;

	state AckUart:

		c = ECONN_OK;
		if (Agent->wi (AckUart, (char*)(&c), 1) == ERROR) {
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

	lword netf;
	char c;

	state AckClk:

		c = ECONN_OK;
		if (Agent->wi (AckClk, (char*)(&c), 1) == ERROR) {
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
		default:  range = (word) 0x7fff;

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

PINS::PINS (data_pn_t *PID) {

	int i, imode, omode;
	byte *taken;
	PicOSNode *ThisNode;

	I = O = NULL;
	Upd = NULL;
	InputThread = OutputThread = MonitorThread = NotifierThread = NULL;

	TimedRequestTime = TIME_0;

	TPN = ThePicOSNode;

	Flags = PID->PMode;
	imode = (Flags & XTRN_IMODE_MASK);
	omode = (Flags & XTRN_OMODE_MASK);

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
	DefIVa = PID->IV;
	DefAVo = PID->VO;
	Status = PID->ST;

	if (PID->MPIN == BNONE || PID->NPIN == BNONE) {
		Assert (PID->MPIN == BNONE && PID->NPIN == BNONE, 
		"PINS: monitor and notifier pins at %s must be both absent or"
		" both present", TheStation->getSName ());
		PIN_MONITOR [0] = PIN_MONITOR [1] = BNONE;
	} else {
		Assert (PID->MPIN < PIN_MAX && gbit (taken, PID->MPIN) == 0,
		    "PINS: illegal monitor pin number %1d at %s", PID->MPIN,
			TheStation->getSName ());
		PIN_MONITOR [0] = PID->MPIN;
		sbit (taken, PID->MPIN);
		Assert (PID->NPIN < PIN_MAX && gbit (taken, PID->NPIN) == 0,
		    "PINS: illegal notifier pin number %1d at %s", PID->NPIN,
			TheStation->getSName ());
		PIN_MONITOR [1] = (byte) PID->NPIN;
		sbit (taken, PID->NPIN);
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

	delete taken;

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

	if (imode == XTRN_IMODE_SOCKET || omode == XTRN_OMODE_SOCKET) {
		Assert (imode == XTRN_IMODE_SOCKET &&
		    omode == XTRN_OMODE_SOCKET, 
			"PINS at %s: confilcting modes %x",
			    TheStation->getSName (),
			        Flags);
		// That's it, the devices will be created upon connection
	
	} else if (imode == XTRN_IMODE_DEVICE) {
		// Need a mailbox for the input end
		Assert (PID->PIDev != NULL,
			"PINS at %s: input device cannot be NULL",
				TheStation->getSName ());
		I = create Dev;
		// Check if the output end uses the same device
		if (omode == XTRN_OMODE_DEVICE && strcmp (PID->PIDev,
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
	}

	// We are done with the input end
	if (O == NULL && omode == XTRN_OMODE_DEVICE) {
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

	if (I != NULL) {
		// The input end
		create (System) PinsInput (this);
	}

	if (O != NULL) {
		// Note that this one will be indestructible
		Upd = create PUpdates (MAX_Long);
		// Signal initial (full) update
		qupd_all ();
		// create will steal ThePicOSNode from us
		ThisNode = ThePicOSNode;
		create (System) PinsHandler (this, NULL);
	}

	rst ();
}

void PINS::rst () {

	int i;

	// Note that reset doesn't affect the agent interface. I am not
	// sure what we should do other than sending a "full update" to
	// the agent.

	if (Upd != NULL) {
		// Signal a change: send all
		Upd->erase ();
		qupd_all ();
	}

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

	if (MonitorThread != NULL) {
		MonitorThread->terminate ();
		MonitorThread = NULL;
	}

	if (NotifierThread != NULL) {
		NotifierThread->terminate ();
		NotifierThread = NULL;
	}
}

PINS::~PINS () {

	excptn ("PINS at %s: cannot delete PINS", TheStation->getSName ());
}

void PulseMonitor::setup (PINS *pins, Process **p, byte onv, Long ontime,
								Long offtime) {
	PN = pins;
	TH = p;
	ONV = onv;
	WCNT = ontime;
	OFFTime = offtime;
	PIN = PN->PIN_MONITOR [0];
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
}
			
void PINS::pin_new_value (word pin, word val) {

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

			if (val == MonPolarity && MonitorThread == NULL)
				// Must run at System to make it independent
				// of the node's resets
				create (System) PulseMonitor (this,
						&MonitorThread,
						MonPolarity,
						PMON_DEBOUNCE_CNT_ON,
						PMON_DEBOUNCE_CNT_OFF);
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

			if (val == NotPolarity && NotifierThread == NULL)
				create (System) PulseMonitor (this,
						&NotifierThread,
						NotPolarity,
						PMON_DEBOUNCE_NOT_ON,
						PMON_DEBOUNCE_NOT_OFF);
			return;
		}
	}

	// A regular pin, including those assigned to Pulse Monitor and
	// Notifier, if not being used as such

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
			trigger (PMON_CNTEVENT);
		}
	} else {
		assert (pin == PIN_MONITOR [1],
			"pmon_update at %s: illegal monitor pin %1d",
				TPN->getSName (), pin);
		pmon_not_pending = YES;
		trigger (PMON_NOTEVENT);
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
		trigger (PMON_CNTEVENT);
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

	pmon_not_pending = pmon_not_on = YES;
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
 * Transforms a pin update request into an outgoing message. The message
 * size is 10 + 1 + 2 + 1 + 2 + 1 + 2 + 1 + 1 + 4 + 1 = 26 bytes
 */
	char *tb;
	tb = UBuf;

	push_msec (tb, Time);
	push_bval (tb, PIN_MAX);
	push_bval (tb, PIN_MAX_ANALOG);
	push_bval (tb, upd.pin);

	*tb++ = ' ';
	*tb++ = tohex (upd.stat);

	push_sval (tb, upd.value);
	*tb++ = '\n';
	*tb = '\0';

	return tb - UBuf;
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

	byte c;

	PN = pn;

	assert (PN != NULL, "PinsHandler: no PINS to handle");

	if ((Agent = a) == NULL) {

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
		else if ((PN->Flags & XTRN_IMODE_SOCKET) == 0)
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

void PinsHandler::goaway (byte c) {
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
	byte c;

	state AckPins:

		if (Agent != NULL) {
			// Socket connection: acknowledge
			c = ECONN_OK;
			if (Agent->wi (AckPins, (char*)(&c), 1) == ERROR) {
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
	PN->I->setSentinel ('\n');
}

PinsInput::perform {
/*
 * Request formats (all numbers in hex):
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

	state Loop:

		BP = &(RBuf [0]);
		Left = PRQS_INPUT_BUFLEN;

	transient ReadRq:

		if ((rc = PN->I->rs (ReadRq, BP, Left)) == ERROR) {
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
		LStat [i] = LED_OFF;

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
	char *BP;
	word i;
	word s;

	BP = UBuf;

	push_msec (BP, Time);
	*BP++ = Fast ? '1' : '0';
	*BP++ = ' ';

	for (i = 0; i < NLeds; i++) {
		s = getstat (i);
		*BP++ = (char)(s + '0');
	}
	*BP++ = '\n';

	return BP - UBuf;
}

void LedsHandler::setup (PicOSNode *tpn, LEDSM *le, Dev *a) {

	byte c;

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
	byte c;

	state AckLeds:

		if (Agent != NULL) {
			// Socket connection: acknowledge
			c = ECONN_OK;
			if (Agent->wi (AckLeds, (char*)(&c), 1) == ERROR) {
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

void MoveHandler::setup (Dev *a, Boolean device) {

	int i;

	Agent = a;
	Agent->setSentinel ('\n');
	Device = device;
	TimedRequestTime = Time;
	// Note: we allow multiple such processes, because they operate
	// globally and their service areas may be disjoint.
	// Initialize the number parse structure
	NP [0] . type = TYPE_hex;
	for (i = 1; i < 5; i++)
		NP [i] . type = TYPE_double;
	// This will do for input requests, but we have to play it
	// safe with the output stuff, which includes node type names, as
	// there is no explicit limit on their length
	RBuf = new char [RBSize = MRQS_INPUT_BUFLEN];
}

MoveHandler::perform {

	TIME st;
	double dd, ee, xx, yy;
	movepool_t *ME;
	int rc;
	Long NN, NS;
	char *re;
	PicOSNode *pn;
	byte c;
	Boolean off;

	state AckMove:

		if (!Device) {
			c = ECONN_OK;
			if (Agent->wi (AckMove, (char*)(&c), 1) == ERROR) {
				// Disconnected
				delete Agent;
				terminate;
			}
		}

	transient Loop:

		BP = RBuf;
		Left = MRQS_INPUT_BUFLEN;

	transient ReadRq:

		if ((rc = Agent->rs (ReadRq, BP, Left)) == ERROR) {
			if (!Device)
				delete Agent;
				// EOF. That's it. We cannot delete the mailbox
				// because it formally belongs to a station.
				// FIXME: can we do that somehow?
			terminate;
		}

		if (rc == REJECTED) {
			if (Device)
				excptn ("MoveHandler: request line too long, "
					"%1d is the max", MRQS_INPUT_BUFLEN);
			create Disconnector (Agent, ECONN_LONG);
			terminate;
		}
			
		// this is the length excluding the newline
		RBuf [MRQS_INPUT_BUFLEN - 1 - Left] = '\0';

		// Request format:
		// T delay
		// node (and nothing else) -> request for coordinates, illegal
		// if device;
		// node x y -> teleport there immediately
		// node x y speed grain (4 fp numbers) -> move there at speed
		// speed with granularity grain metres

		BP = RBuf;
		skipblk (BP);
		if (*BP == 'T') {
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
				if (Device)
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
		}

		if ((rc = parseNumbers (RBuf, 5, NP)) == 0)
			// Treat this as a null line, e.g., heartbeat or comment
			proceed Loop;

		// Otherwise, this must be a valid node number
		NN = NP [0].IVal;
		if (!isStationId (NN)) {
			if (Device)
				excptn ("MoveHandler: illegal node Id %1d", NN);
			// Socket, NACK + disconnect
			create Disconnector (Agent, ECONN_STATION);
			terminate;
		}

		if (rc == 1) {
			// Node number only: send back the coordinates
			if (Device)
				excptn ("MoveHandler: coordinate request '%s; "
					"illegal from non-socket interface",
						RBuf);

			pn = (PicOSNode*)idToStation (NN);
			pn -> _da (RFInterface)->getLocation (xx, yy);

			while ((rc = snprintf (RBuf, RBSize, "%x %x %f %f %s\n",
			   NN, NStations, xx, yy, pn->getTName ())) >= RBSize) {
				// Must grow the buffer
				RBSize = (word)(rc + 1);
				delete RBuf;
				RBuf = new char [RBSize];
			}

			BP = &(RBuf [0]);
			Left = strlen (RBuf);
			proceed Reply;
		}

		if (rc != 3 && rc != 5)
			goto Illegal;

		if ((ME = find_mip (NN)) != NULL) {
			// Terminate the previous mover
			ME->Thread->terminate ();
			pool_out (ME);
			delete ME;
		}

		if (NP [1].DVal < 0.0 || NP [2].DVal < 0.0) {
			// Illegal target coordinate
			if (Device)
				excptn ("MoveHandler: illegal coordinates "
					"(%f, %f), must not be negative",
						NP[1].DVal, NP[2].DVal);
			// Socket
			create Disconnector (Agent, ECONN_INVALID);
			terminate;
		}
		
		if (rc == 3) {
			// Immediate teleport
Immediate:
			((PicOSNode*)idToStation (NN))->
				_da (RFInterface)->setLocation (NP [1].DVal,
					NP [2].DVal);
			proceed Loop;
		}

		if (NP [3].DVal < 0.0001 || NP [4].DVal < 0.0001) {
			// Illegal move parameters
			if (Device)
				excptn ("MoveHandler: illegal speed or grain, "
					"%f, %f, must be >= 0.0001",
						NP [3].DVal, NP[4].DVal);
			create Disconnector (Agent, ECONN_INVALID);
			terminate;
		}

		ThePicOSNode->_da (RFInterface)->getLocation (xx, yy);

		// Distance
		dd = dist (xx, yy, NP [1].DVal, NP [2].DVal);
		// Number of steps
		ee = dd / NP [4].DVal;
		if (ee > 10000.0) {
			// Make this a limit
			NS = 10000;
		} else {
			NS = (Long)(ee + 0.5);
			if (NS = 0)
				goto Immediate;
		}
		// Total time interval
		dd = dd / NP [3].DVal;

		ME = new movepool_t;
		ME->NNumber = NN;
		ME->Thread = create Teleporter (ME, NP [1].DVal, NP [2].DVal,
			NS, dd);
		pool_in (ME, MIP, movepool_t);

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

void Teleporter::setup (movepool_t *me, double x, double y, Long cn,
					   			double ttot) {
	
	ME = me;
	No = (PicOSNode*)idToStation (ME->NNumber);
	TX = x;
	TY = y;
	// Calculate the number of steps needed
	No->_da (RFInterface)->getLocation (CX, CY);
	Count = cn;
	TLeft = ttot;
}

Teleporter::perform {

	double d, dd;

	state Advance:

		No->_da (RFInterface)->setLocation (CX, CY);
		if (Count == 0) {
			pool_out (ME);
			delete ME;
			terminate;
		}

		// Calculate next step
		if (Count == 1) {
			Count = 0;
			CX = TX;
			CY = TY;
			Timer->delay (TLeft, Advance);
			sleep;
		}

		d = dist (CX, CY, TX, TY);
		if (d < 0.0001) {
			// Just in case
			CX = TX;
			CY = TY;
		} else {
			// Distance fraction for next step
			dd =  d / Count;
			CX += dd * (TX - CX) / d;
			CY += dd * (TY - CY) / d;
		}

		// Current interval
		d = TLeft / Count;
		Count--;
		TLeft -= d;
		Timer->delay (d, Advance);
		sleep;
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

			case AGENT_RQ_LEDS:

				if (TPN == NULL)
					create Disconnector (Agent,
						ECONN_STATION);
				else
					create LedsHandler (TPN, NULL, Agent);
				terminate;

			case AGENT_RQ_MOVE:

				create MoveHandler (Agent, NO);
				terminate;

			case AGENT_RQ_CLOCK:

				create ClockHandler (Agent);
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
		if (Agent->wi (Send, (char*)(&code), 1) == ERROR)
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
	if (M->connect (INTERNET + SERVER + MASTER, AGENT_SOCKET) != OK)
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
