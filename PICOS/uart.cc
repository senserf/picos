#ifndef __uart_c__
#define __uart_c__

// This is a UART model prepared for the TARP project, but possibly useful for
// other projects as well

#include "uart.h"
#include "stdattr.h"

#define	UART_SOCKET_BUFLEN	64
#define	UART_DEVICE_BUFLEN	32

UART::UART (int speed, const char *idev, const char *odev, int mode, int bsize) {
/*
 * Initialize the UART model. mode is sum of
 *
 *    UART_IMODE_NONE, UART_IMODE_DEVICE, UART_IMODE_SOCKET or UART_IMODE_STRING
 *    UART_OMODE_NONE, UART_OMODE_DEVICE or UART_OMODE_SOCKET
 *    UART_IMODE_TIMED or UART_IMODE_UNTIMED
 *    UART_IMODE_HEX or UART_IMODE_ASCII
 *    UART_OMODE_HEX or UART_OMODE_ASCII
 *    input string length is orred into mode
 *
 *    IMODE_SOCKET requires OMODE_SOCKET (this is value zero, i.e., default)
 *    IMODE_DEVICE + OMODE_DEVICE (the two devices can, but need not have to,
 *    be the same)
 *
 *    For _SOCKET, idev == hostname, odev == port number (in ASCII)
 *
 *    _HEX data: 12af565e4ffg .... (whitespace ignored on input)
 *    _ASCII means "raw"
 *    _TIMED (input only) [+]Time { ... } (Time is in ITUs)
 */
	char *sp, *fn;
	int st, imode, omode;
	Boolean R, W;

	R = W = NO;
	I = O = NULL;
	Flags = mode;
	PI = NULL;
	PO = NULL;
	B_len = bsize;
	IBuf = OBuf = NULL;
	String = NULL;
	TI_aux = NULL;
	TimedChunkTime = TIME_0;

	imode = (mode & UART_IMODE_MASK);
	omode = (mode & UART_OMODE_MASK);

	if (imode == UART_IMODE_SOCKET || omode == UART_OMODE_SOCKET) {
		// The socket case: both ends must use the same socket
		Assert (imode == UART_IMODE_SOCKET &&
			omode == UART_OMODE_SOCKET, 
				"UART at %s: confilcting modes %x",
					TheStation->getSName (),
						mode);
		// We have a socket: one mailbox acting both ways
		R = W = YES;
		I = create Dev;
		if (I->connect (CLIENT+INTERNET+READ+WRITE, idev, atoi (odev),
		    UART_SOCKET_BUFLEN) == ERROR)
			excptn ("UART at %s: cannot connect to socket",
				TheStation->getSName ());
		O = I;
	} else if (imode == UART_IMODE_DEVICE) {
		// Need a mailbox for the input end
		Assert (idev != NULL, "UART at %s: input device cannot be NULL",
			TheStation->getSName ());
		R = YES;
		I = create Dev;
		// Check if the output end uses the same device
		if (omode == UART_OMODE_DEVICE && strcmp (idev, odev) == 0) {
			// Yep, single mailbox will do
			st = I->connect (DEVICE+READ+WRITE, idev, 0,
					UART_DEVICE_BUFLEN);
			O = I;
			W = YES;
		} else {
			st = I->connect (DEVICE+READ, idev, 0, 32);
		}
		if (st == ERROR)
			excptn ("UART at %s: cannot open device %s",
				TheStation->getSName (), idev);
	} else if (imode == UART_IMODE_STRING) {
		// String
		Assert (idev != NULL, "UART at %s: the input string is empty",
			TheStation->getSName ());
		SLen = (mode & UART_IMODE_STRLEN);
		if (SLen == 0)
			// Use strlen
			SLen = strlen (idev);
		if (SLen == 0)
			excptn ("UART at %s: input string length is 0",
				TheStation->getSName ());

		// No sentinel needed
		String = new char [SLen];
		strncpy (String, idev, SLen);
		R = YES;
	}

	// We are done with the input end
	if (O == NULL && omode != UART_OMODE_NONE) {
		// This can only mean a separate DEVICE 
		W = YES;
		O = create Dev;
		if (O->connect (DEVICE+WRITE, odev, 0, UART_DEVICE_BUFLEN))
			excptn ("UART at %s: cannot open device %s",
				TheStation->getSName (), odev);
	}

	if (R)
		IBuf = new byte [B_len];

	if (W)
		OBuf = new byte [B_len];

	// Account for start / stop bits, assuming there is no parity and one
	// stop bit, which is the case in all our setups
	ByteTime = (TIME) ((Second / speed) * 10.0);

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

	if ((Flags & UART_IMODE_MASK) == UART_IMODE_STRING) {
		if (SLen == 0) {
			// This event will never happen. We wait here forever
			// when the string has ended. Perhaps later I will do
			// something about the process.
			when ((int)(&st), st);
			release;
		}
		SLen--;
		return *String++;
	}
	// Read from the mailbox
	if (I->read (&c, 1) != ACCEPTED) {
		I->wait (NEWITEM, st);
		release;
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

void UART::getTimed (char *res, int state) {
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
			release;
		}
		if (Flags & UART_IMODE_HEX) {
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
		TCS = TCS_RAS;
		*res = c;
		return;

	    case TCS_UHX:
	    case TCS_LHX:

		// Reading hex (two nibbles)
		while (1) {
			c = getOneByte (state);
			// Ignore spaces and x-es
			if (c == '}') {
				// done
				TCS = TCS_WOB;
				goto Redo;
			}
			c = tolower (c);
			if (isspace (c) || c == 'x')
				continue;
			// This must be a hex digit
			if (!isxdigit (c))
				ti_err ();
			if (isdigit (c))
				v = (int) c - '0';
			else
				v = (int) c - 'a' + 10;
			if (TCS == TCS_UHX) {
				TI_ptr = v;
				TCS = TCS_LHX;
				continue;
			}
			TCS = TCS_UHX;
			*res = (TI_ptr << 4) | v;
			return;
		}
	}
}

UART_in::perform {

    TIME D;
    char c, r, v;

    state Get:

	if ((U->Flags & UART_IMODE_TIMED)) {
		// Timed
		if ((D = TimeLastRead + U->ByteTime) > Time) {
			Timer->wait (D - Time, Get);
			release;
		}
		// Timed sequences get in even if nobody wants them, i.e.,
		// no room in the buffer. They are lost in such cases.
		U->getTimed (&c, Get);
		if (!U->ibuf_full ())
			U->ibuf_put ((byte)c);
	} else {
		// Untimed: the characters are ready to be accepted, but they
		// wait until needed
		if (U->ibuf_full ()) {
			Monitor->wait (&(U->IB_out), Get);
			release;
		}
		if ((D = TimeLastRead + U->ByteTime) > Time) {
			Timer->wait (D - Time, Get);
			release;
		}
		// Get next byte
		c = U->getOneByte (Get);
		if ((U->Flags & UART_IMODE_HEX))
			proceed GetH0;
		// Just store the byte
		U->ibuf_put ((byte)c);
	}

Complete:

	TimeLastRead = Time;
	Monitor->signal (&(U->IB_in));
	proceed Get;

    state GetH0:
    transient GetH1:

	while (1) {
		c = U->getOneByte (TheState);
		c = tolower (c);
		if (isspace (c) || c == 'x')
			continue;
		if (!isxdigit (c))
			ti_err ();
		if (isdigit (c))
			c -= '0';
		else
			c += 10 - 'a';
		if (TheState == GetH0) {
			r = c;
			proceed GetH1;
		}
		r = (r << 4) | c;
		U->ibuf_put ((byte)c);
		goto Complete;
	}
			
}

UART_out::perform {

    byte b;
    char tc [3];
    TIME D;

    state Put:

	if (U->obuf_empty ()) {
		Monitor->wait (&(U->OB_in), Put);
		release;
	}

	if ((D = TimeLastWritten + U->ByteTime) > Time) {
		Timer->wait (D - Time, Put);
		release;
	}

	b = U->obuf_peek ();

	if ((U->Flags & UART_OMODE_HEX)) {
		// Three bytes
		tc [0] = tohex (b >> 4);
		tc [1] = tohex (b & 0xf);
		tc [3] = ' ';
		if (U->O->write (tc, 3) == REJECTED) {
			// This will practically never happen, so the cost
			// of redoing the conversion is nil
			U->O->wait (OUTPUT, Put);
			release;
		}
	} else {
		if (U->O->write ((char*)(&b), 1) == REJECTED) {
			// Output device is blocked
			U->O->wait (OUTPUT, Put);
			release;
		}
	}
		
	// Remove the byte
	U->obuf_get ();
	Monitor->signal (&(U->OB_out));

	TimeLastWritten = Time;

	proceed Put;
}

UART::~UART () {

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

	if (String != NULL)
		delete String;

	if (TI_aux != NULL)
		delete TI_aux;
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
				release;
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
				release;
			}

			// We have stored something
			Monitor->signal (&OB_in);
			return len - bc;

		default:

			excptn ("UART->ioop: illegal operation");
	}
}

#include "stdattr_undef.h"

#endif
