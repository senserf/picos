#ifndef __picos_board_c__
#define __picos_board_c__

#include "board.h"

#include "chan_shadow.cc"
#include "encrypt.cc"
#include "nvram.cc"

const char	zz_hex_enc_table [] = {
				'0', '1', '2', '3', '4', '5', '6', '7',
				'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
			      };

void _dad (PicOSNode, diag) (const char *s, ...) {

	va_list ap;

	va_start (ap, s);

	trace ("[%1.3f] DIAG [%1d]: %s", ituToEtu (Time), \
		      TheStation->getId (), ::vform (s, ap));
}

void syserror (int p, const char *s) {

	excptn (::form ("SYSERROR [%1d]: %1d, %s", TheStation->getId (), p, s));
}

void PicOSNode::reset () {

	MemChunk *mc;

	// Kill all processes run by this station
	terminate ();

	// Clean up memory
	MFree = MTotal;
	while (MHead != NULL) {
		delete (byte*)(MHead->PTR);
		mc = MHead -> Next;
		delete MHead;
		MHead = mc;
	}
	MTail = NULL;

	// Abort the transceiver if transmitting
	if (_da (RFInterface)->transmitting ())
		_da (RFInterface)->abort ();

	// Reset the transceiver to defaults
	_da (RFInterface)->rcvOn ();
	_da (RFInterface)->setXPower (DefXPower);
	_da (RFInterface)->setRPower (DefRPower);

	if (uart != NULL) {
		uart->__inpline = NULL;
		uart->pcsInserial = uart->pcsOutserial = NULL;
		uart->U->rst ();
	}

	_da (entropy) = 0;
	_da (statid) = 0;

	_da (Receiving) = _da (Xmitting) = NO;
	_da (TXOFF) = _da (RXOFF) = YES;

	_da (OBuffer).fill (NONE, NONE, 0, 0, 0);

	// This will do the dynamic initialization of static stuff in TCV
	_da (tcv_init) ();
}

void PicOSNode::setup ( word mem,
			double x, double y,
			double xp, double rp,
			Long bcmin, Long bcmax,
			Long lbtdel, double lbtths,
			RATE rate,
			Long pre,
			Long eeprs, Long iflashs, Long iflashp,
                       	Long umode, Long ubs, Long usp,
			char *uidv, char *uodv) {

	// Turn this into a trigger mailbox
	TB.setLimit (-1);

	MTotal = (mem + 3) / 4;			// This is in full words
	MHead = MTail = NULL;

	// These two survive reset. We assume that they are never changed
	// by the application.
	_da (min_backoff) = (word) bcmin;
	// This is the argument for 'toss' to generate the proper
	// offset. The consistency has been verified by readNodeParams.
	_da (max_backoff) = (word) bcmax - _da (min_backoff) + 1;

	// Same about these two
	if (lbtdel == 0) {
		// Disable it
		_da (lbt_threshold) = HUGE;
		_da (lbt_delay) = 0;
	} else {
		_da (lbt_threshold) = dBToLin (lbtths);
		_da (lbt_delay) = (word) lbtdel;
	}

	if ((umode & (UART_IMODE_MASK | UART_OMODE_MASK)) ==
	    (UART_IMODE_NONE | UART_OMODE_NONE)) {
		// No UART
		uart = NULL;
	} else {
		uart = new uart_t;
		uart->U = new UART (usp, uidv, uodv, umode, ubs);
	}

	DefXPower = dBToLin (xp);
	DefRPower = dBToLin (rp);

	_da (RFInterface) = create Transceiver (
			rate,
			pre,
			DefXPower,
			DefRPower,
			x, y );

	Ether->connect (_da (RFInterface));

	// EEPROM and IFLASH: note that they are not reset
	if (eeprs)
		eeprom = new NVRAM ((word)eeprs, 0);
	else
		eeprom = NULL;

	if (iflashs)
		iflash = new NVRAM ((word)iflashs, (word)iflashp);
	else
		iflash = NULL;

	reset ();

};

lword _dad (PicOSNode, seconds) () {

	// FIXME: make those different at different stations
	return (lword) ituToEtu (Time);
};

address PicOSNode::memAlloc (int size, word lsize) {
/*
 * size  == real size 
 * lsize == simulated size
 */
	MemChunk 	*mc;
	address		*op;

	lsize = (lsize + 3) / 4;		// Convert to 4-tuples
	if (lsize > MFree) {
		return NULL;
	}

	mc = new MemChunk;
	mc -> Next = NULL;
	mc -> PTR = (address) new byte [size];
	mc -> Size = lsize;
	MFree -= lsize;

	if (MHead == NULL)
		MHead = mc;
	else
		MTail->Next = mc;
	MTail = mc;

	return mc->PTR;
}

void PicOSNode::memFree (address p) {

	MemChunk *mc, *pc;

	for (pc = NULL, mc = MHead; mc != NULL; pc = mc, mc = mc -> Next) {

		if (p == mc->PTR) {
			// Found
			if (pc == NULL)
				MHead = mc->Next;
			else
				pc->Next = mc->Next;

			if (mc->Next == NULL)
				MTail = pc;

			delete (byte*) (mc->PTR);
			MFree += mc -> Size;
			assert (MFree <= MTotal,
				"PicOSNode->memFree: corrupted memory");
			delete mc;
			TB.signal (N_MEMEVENT);
			return;
		}

	}

	excptn ("PicOSNode->memFree: chunk not found");
}

word PicOSNode::memfree (int pool, word *res) {
/*
 * This one is solely for stats
 */
	if (res != NULL)
		*res = NFree << 1;

	// This is supposed to be in words, if I remember correctly. What
	// a mess!!!
	return MFree << 1;
}

word _dad (PicOSNode, actsize) (address p) {

	MemChunk *mc;

	for (mc = MHead; mc != NULL; mc = mc->Next)
		if (p == mc->PTR)
			// Found
			return mc->Size * 4;

	excptn ("PicOSNode->actsize: incorrect chunk pointer");
	return 0;
}

Boolean PicOSNode::memBook (word lsize) {

	lsize = (lsize + 3) / 4;
	if (lsize > MFree) {
		return NO;
	}
	MFree -= lsize;
	return YES;
}

void PicOSNode::memUnBook (word lsize) {

	lsize = (lsize + 3) / 4;
	MFree += lsize;
	assert (MFree <= MTotal, "PicOSNode->memUnBook: corrupted memory");
}

char* _dad (PicOSNode, form) (char *buf, const char *fm, ...) {

	va_list	ap;
	va_start (ap, fm);

	return _da (vform) (buf, fm, ap);
}

int _dad (PicOSNode, scan) (const char *buf, const char *fmt, ...) {

	va_list ap;
	va_start (ap, fmt);

	return _da (vscan) (buf, fmt, ap);
}

char* _dad (PicOSNode, vform) (char *res, const char *fm, va_list aq) {

	word fml, s, d;
	char c;
	va_list ap;

#define	outc(c)	do { \
			if (d >= fml) \
				goto ReAlloc; \
			res [d++] = (char)(c); \
		} while (0)

#define enci(b)	i = (b); \
		while (1) { \
			c = (char) (val / i); \
			if (c || i == 1) \
				break; \
			i /= 10; \
		} \
		while (1) { \
			outc (c + '0'); \
			val = val - (c * i); \
			i /= 10; \
			if (i == 0) \
				break; \
			c = (char) (val / i); \
		}

#define encx(s)	for (i = 0; i < (s); i += 4) { \
			outc (zz_hex_enc_table [((val >> (((s)-4)-i)) & 0xf)]);\
		}

	if (res != NULL)
		/* Fake huge maximum length */
		fml = MAX_UINT;
	else
		/* Guess an initial length of the formatted string */
		fml = strlen (fm) + 16;

	while (1) {
		if (fml != MAX_UINT) {
			if ((res = (char*) umalloc (fml+1)) == NULL)
				/* There is not much we can do */
				return NULL;
			/* This is how far we can go */
			fml = _da (actsize) ((address)res) - 1;
		}
		s = d = 0;

		va_copy (ap, aq);

		while (1) {
			c = fm [s++];
			if (c == '\\') {
				/* Escape the next character unless it is 0 */
				if ((c = fm [s++]) == '\0') {
					res [d] = '\0';
					return res;
				}
				outc (c);
				continue;
			}
			if (c == '%') {
				/* Something special */
				c = fm [s++];
				if (c == '\0') {
					res [d] = '\0';
					return res;
				}
				switch (c) {
				    case 'x' : {
					word val; int i;
					val = va_arg (ap, int);
					encx (16);
					break;
				    }
				    case 'd' :
				    case 'u' : {
					word val, i;
					val = va_arg (ap, int);
					if (c == 'd' && (val & 0x8000) != 0) {
						/* Minus */
						outc ('-');
						val = (~val) + 1;
					}
					enci (10000);
					break;
				    }
#if	CODE_LONG_INTS
				    case 'l' :
					c = fm [s];
					if (c == 'd' || c == 'u') {
						lword val, i;
						s++;
						val = va_arg (ap, lword);
						if (c == 'd' &&
						    (val & 0x80000000L) != 0) {
							/* Minus */
							outc ('-');
							val = (~val) + 1;
						}
						enci (1000000000L);
					} else if (c == 'x') {
						lword val;
						int i;
						s++;
						val = va_arg (ap, lword);
						encx (32);
					} else {
						outc ('%');
						outc ('l');
					}
					break;
#endif
				    case 'c' : {
					word val;
					val = va_arg (ap, int);
					outc (val);
					break;
				    }
			  	    case 's' : {
					char * st;
					st = va_arg (ap, char*);
					while (*st != '\0') {
						outc (*st);
						st++;
					}
					break;
				    }
			  	    default:
					outc ('%');
					outc (c);
				}
			} else {
				outc (c);
				if (c == '\0')
					return res;
			}
		}
	ReAlloc:
		if (fml == MAX_UINT)
			/* Impossible */
			return res;
		ufree (res);
		fml += 16;
	}
}

int _dad (PicOSNode, vscan) (const char *buf, const char *fmt, va_list ap) {

	int nc;

#define	scani(at)	{ unsigned at *vap; bool mf; \
			Retry_d_ ## at: \
			while (!isdigit (*buf) && *buf != '-' && *buf != '+') \
				if (*buf++ == '\0') \
					return nc; \
			mf = NO; \
			if (*buf == '-' || *buf == '+') { \
				if (*buf++ == '-') \
					mf = YES; \
				if (!isdigit (*buf)) \
					goto Retry_d_ ## at; \
			} \
			nc++; \
			vap = va_arg (ap, unsigned at *); \
			*vap = 0; \
			while (isdigit (*buf)) { \
				*vap = (*vap) * 10 - \
				     (unsigned at)(unsigned int)(*buf - '0'); \
				buf++; \
			} \
			if (!mf) \
				*vap = (unsigned at)(-((at)(*vap))); \
			}
#define scanu(at)	{ unsigned at *vap; \
			while (!isdigit (*buf)) \
				if (*buf++ == '\0') \
					return nc; \
			nc++; \
			vap = va_arg (ap, unsigned at *); \
			*vap = 0; \
			while (isdigit (*buf)) { \
				*vap = (*vap) * 10 + \
				     (unsigned at)(unsigned int)(*buf - '0'); \
				buf++; \
			} \
			}
#define	scanx(at)	{ unsigned at *vap; int dc; char c; \
			while (!isxdigit (*buf)) \
				if (*buf++ == '\0') \
					return nc; \
			nc++; \
			vap = va_arg (ap, unsigned at *); \
			*vap = 0; \
			dc = 0; \
			while (isxdigit (*buf) && dc < 2 * sizeof (at)) { \
				c = *buf++; dc++; \
				if (isdigit (c)) \
					c -= '0'; \
				else if (c <= 'f' && c >= 'a') \
					c -= (char) ('a' - 10); \
				else \
					c -= (char) ('A' - 10); \
				*vap = ((*vap) << 4) | (at) c; \
			} \
			}


	nc = 0;
	while (*fmt != '\0') {
		if (*fmt++ != '%')
			continue;
		switch (*fmt++) {
		    case '\0': return nc;
		    case 'd': scani (int); break;
		    case 'u': scanu (int); break;
		    case 'x': scanx (int); break;
#if	CODE_LONG_INTS
		    case 'l':
			switch (*fmt++) {
			    case '\0':	return nc;
		    	    case 'd': scani (long); break;
		    	    case 'u': scanu (long); break;
		    	    case 'x': scanx (long); break;
			}
			break;
#endif
		    case 'c': {
			char c, *sap;
			/* One character exactly where we are */
			if ((c = *buf++) == '\0')
				return nc;
			nc++;
			sap = va_arg (ap, char*);
			*sap = c;
			break;
		    }
		    case 's': {
			char *sap;
			while (isspace (*buf)) buf++;
			if (*buf == '\0')
				return nc;
			nc++;
			sap = va_arg (ap, char*);

			if (*buf != ',') {
				while (!isspace (*buf) && *buf != ',' &&
					*buf != '\0')
						*sap++ = *buf++;
			}
			while (isspace (*buf)) buf++;
			if (*buf == ',') buf++;
			*sap = '\0';
			break;
		    }
		}
	}
	return nc;
}

int _dad (PicOSNode, ser_in) (word st, char *buf, int len) {

	int prcs;

	assert (st != WNONE, "PicOSNode->ser_in: NONE state unimplemented");

	if (uart->__inpline == NULL) {
		if (uart->pcsInserial == NULL)
			create Inserial;
		uart->pcsInserial->wait (DEATH, st);
		sleep;
	}

	/* Input available */
	if (*(uart->__inpline) == 0) // bin cmd
		prcs = uart->__inpline [1] + 3; // 0x00, len, 0x04
	else
		prcs = strlen (uart->__inpline);
	if (prcs >= len)
		prcs = len-1;

	memcpy (buf, uart->__inpline, prcs);

	ufree (uart->__inpline);
	uart->__inpline = NULL;

	if (*buf) // if it's NULL, it's a bin cmd
		buf [prcs] = '\0';

	return 0;
}

int _dad (PicOSNode, ser_out) (word st, const char *m) {

	int prcs;
	char *buf;

	assert (st != WNONE, "PicOSNode->ser_out: NONE state unimplemented");

	if (uart->pcsOutserial != NULL) {
		uart->pcsOutserial->wait (DEATH, st);
		sleep;
	}
	
	if (*m)
		prcs = strlen (m) + 1;
	else
		prcs =  m [1] + 3;

	if ((buf = (char*) umalloc (prcs)) == NULL) {
		/*
		 * We have to wait for memory
		 */
		umwait (st);
		sleep;
	}

	if (*m)
		strcpy (buf, m);
	else
		memcpy (buf, m, prcs);

	create Outserial (buf);

	return 0;
}

int _dad (PicOSNode, ser_inf) (word st, const char *fmt, ...) {
/* ========= */
/* Formatted */
/* ========= */

	int prcs;
	va_list	ap;

	assert (st != WNONE, "PicOSNode->ser_inf: NONE state unimplemented");

	if (uart->__inpline == NULL) {
		if (uart->pcsInserial == NULL)
			create Inserial;
		uart->pcsInserial->wait (DEATH, st);
		sleep;
	}

	/* Input available */
	va_start (ap, fmt);

	prcs = _da (vscan) (uart->__inpline, fmt, ap);

	ufree (uart->__inpline);
	uart->__inpline = NULL;

	return 0;
}

int _dad (PicOSNode, ser_outf) (word st, const char *m, ...) {

	int prcs;
	char *buf;
	va_list ap;

	assert (st != WNONE, "PicOSNode->ser_outf: NONE state unimplemented");

	if (uart->pcsOutserial != NULL) {
		uart->pcsOutserial->wait (DEATH, st);
		sleep;
	}
	
	va_start (ap, m);

	if ((buf = _da (vform) (NULL, m, ap)) == NULL) {
		/*
		 * This means we are out of memory
		 */
		umwait (st);
		sleep;
	}

	create Outserial (buf);
	return 0;
}

void _dad (PicOSNode, ee_read) (word adr, byte *buf, word n) {

	sysassert (eeprom != NULL, "ee_read no eeprom");
	eeprom->get (adr, buf, n);
};

void _dad (PicOSNode, ee_write) (word adr, const byte *buf, word n) {

	sysassert (eeprom != NULL, "ee_write no eeprom");
	eeprom->put (adr, buf, n);
};

void _dad (PicOSNode, ee_erase) () {

	sysassert (eeprom != NULL, "ee_erase no eeprom");
	eeprom->erase ();
};

int _dad (PicOSNode, if_write) (word adr, word w) {

	sysassert (iflash != NULL, "if_write no iflash");
	iflash->put (adr << 1, (const byte*) (&w), 2);
	return 0;
};

word _dad (PicOSNode, if_read) (word adr) {

	word w;

	sysassert (iflash != NULL, "if_read no iflash");
	iflash->get (adr << 1, (byte*) (&w), 2);
	return w;
};

void _dad (PicOSNode, if_erase) (int a) {

	sysassert (iflash != NULL, "if_erase no iflash");

	if (a < 0)
		iflash->erase ();
	else
		iflash->erase ((word)(a << 1));
};

void NNode::setup () {

#include "plug_null_node_data_init.h"

}

void NNode::reset () {

	PicOSNode::reset ();

#include "plug_null_node_data_init.h"

}

void TNode::setup () {

#include "net_node_data_init.h"
#include "plug_tarp_node_data_init.h"
#include "tarp_node_data_init.h"

}

void TNode::reset () {

	PicOSNode::reset ();

#include "net_node_data_init.h"
#include "plug_tarp_node_data_init.h"
#include "tarp_node_data_init.h"

}

// =====================================
// Root stuff: input data interpretation
// =====================================

BoardRoot::perform {

	state Start:

		initAll ();
		Kernel->wait (DEATH, Stop);

	state Stop:

		terminate;
};
				
static int sanitize_string (char *str) {
/*
 * Strip leading and trailing spaces, process UNIX escape sequences, return
 * the actual length of the string. Note that sanitize_string is NOT
 * idempotent.
 */
	char c, *sptr, *optr;
	int len, k, n;

	sptr = str;

	// The first pass: skip leading and trailing spaces
	while (*sptr != '\0' && isspace (*sptr))
		sptr++;

	optr = str + strlen (str) - 1;
	while (optr >= sptr && isspace (*optr))
		optr--;

	len = optr - sptr + 1;

	if (len == 0)
		return 0;

	// Move the string to the front
	for (k = 0; k < len; k++)
		str [k] = *sptr++;

	// Handle escapes
	sptr = optr = str;
	while (len--) {
		if (*sptr != '\\') {
			*optr++ = *sptr++;
			continue;
		}
		// Skip the backslash
		sptr++;
		if (len == 0)
			// Backslash at the end - ignore it
			break;

		// Check for octal escape
		if (*sptr >= '0' && *sptr <= '7') {
			n = 0;
			k = 3;
			while (len && k) {
				if (*sptr < '0' || *sptr > '7')
					break;
				n = (n << 3) + (*sptr - '0');
				sptr++;
				len--;
				k--;
			}
			*optr++ = (char) n;
			continue;
		}
		
		switch (*sptr) {

		    case 't' :	*optr = '\t'; break;
		    case 'n' :	*optr = '\n'; break;
		    case 'r' :	*optr = '\r'; break;
		    default :
			// Regular character
			*optr = *sptr;
		}

		len--; sptr++; optr++;
	}

	return optr - str;
}
	
void BoardRoot::initTiming (sxml_t xml) {

	const char *att;
	double grid = 1.0;
	nparse_t np [1];
	sxml_t data;
	int qual;

	np [0].type = TYPE_double;

	if ((data = sxml_child (xml, "grid")) != NULL) {
		att = sxml_txt (data);
		if (parseNumbers (att, 1, np) != 1)
			excptn ("Root: <grid> parameter error");
		grid = np [0].DVal;
	}

	// ITU is equal to the propagation time across grid unit, assuming 1
	// ETU == 1 second
	setEtu (SOL_VACUUM / grid);

	// DU is equal to the propagation time across 1m
	setDu (1.0/grid);

	// Clock tolerance
	if ((data = sxml_child (xml, "tolerance")) != NULL) {
		att = sxml_txt (data);
		if (parseNumbers (att, 1, np) != 1)
			excptn ("Root: <tolerance> parameter error: %s",
				att);
		grid = np [0].DVal;
		if ((att = sxml_attr (data, "quality")) != NULL) {
			np [0].type = TYPE_LONG;
			if (parseNumbers (att, 1, np) != 1)
				excptn ("Root: <tolerance> 'quality' format "
					"error: %s", att);
			qual = (int) (np [0].LVal);
		} else
			qual = 2;	// This is the default

		setTolerance (grid, qual);
	}
}

void BoardRoot::initChannel (sxml_t data, int NT) {

#define	NPTABLE_SIZE	64

	const char *att;
	double bn_db, beta, dref, sigm, loss_db, psir, pber, cutoff;
	nparse_t np [NPTABLE_SIZE];
	int n, i, syncbits, bpb, frml;
	Long brate;
	sxml_t cur;
	static	sir_to_ber_t	*STB;

	if ((data = sxml_child (data, "channel")) == NULL)
		excptn ("Root: <channel> specification not found");

	// At the moment, we handle shadowing models only
	if ((cur = sxml_child (data, "shadowing")) == NULL)
		excptn ("Root: <shadowing> specification not found within "
			"<channel>");

	if ((att = sxml_attr (cur, "bn")) == NULL)
		excptn ("Root: 'bn' attribute missing from <shadowing>");

	np [0].type = TYPE_double;
	if (parseNumbers (att, 1, np) != 1)
		excptn ("Root: <shadowing> 'bn' attribute invalid: %s", att);
	bn_db = np [0].DVal;

	if ((att = sxml_attr (cur, "syncbits")) == NULL)
		excptn ("Root: 'syncbits' attribute missing from <shadowing>");
	np [0].type = TYPE_LONG;
	if (parseNumbers (att, 1, np) != 1)
		excptn ("Root: 'syncbits' attribute is <shadowing> is "
			"invalid: %s", att);
	syncbits = np [0].LVal;
	
	// Now for the model paramaters
	att = sxml_txt (cur);
	for (i = 0; i < NPTABLE_SIZE; i++)
		np [i].type = TYPE_double;
	if ((n = parseNumbers (att, 5, np)) != 5)
		excptn ("Root: expected 5 numbers in <shadowing>, found %1d",
			n);

	if (np [0].DVal != -10.0)
		excptn ("Root: the factor in <shadowing> must be -10, is %f",
			np [0].DVal);

	beta = np [1].DVal;
	dref = np [2].DVal;
	sigm = np [3].DVal;
	loss_db = np [4].DVal;

	// Decode the BER table
	if ((cur = sxml_child (data, "ber")) == NULL)
		excptn ("Root: <ber> specification missing from <channel>");
	att = sxml_txt (cur);
	n = parseNumbers (att, NPTABLE_SIZE, np);
	if (n < 4 || (n & 1) != 0)
		excptn ("Root: illegal size of <ber> table (%1d), must be "
			"an even number >= 4", np);
	psir = HUGE;
	pber = -1.0;
	STB = new sir_to_ber_t [n / 2];

	// This is the size of BER table
	n /= 2;
	for (i = 0; i < n; i++) {
		// The SIR is stored as a linear ratio
		STB [i].sir = dBToLin (np [2 * i] . DVal);
		STB [i].ber = np [2 * i + 1] . DVal;
		// Validate
		if (STB [i] . sir >= psir)
			excptn ("Root: SIR entries in <ber> must be "
				"monotonically decreasing, %f and %f aren't",
					psir, STB [i] . sir);
		psir = STB [i] . sir;
		if (STB [i] . ber < 0)
			excptn ("Root: BER entries in <ber> must not be "
				"negative, %f is", STB [i] . ber);
		if (STB [i] . ber <= pber)
			excptn ("Root: BER entries in <ber> must be "
				"monotonically increasing, %f and %f aren't",
					pber, STB [i] . ber);
		pber = STB [i] . ber;
	}

	// The cutoff threshold wrt to background noise: the dafault means no
	// cutoff
	cutoff = 0.0;
	if ((cur = sxml_child (data, "cutoff")) != NULL) {
		att = sxml_txt (cur);
		if (parseNumbers (att, 1, np) != 1)
			excptn ("Root: illegal <cutoff> value (%s) within "
				"<channel>", att);
		cutoff = np [0].DVal;
	}

	// Frame parameters
	if ((cur = sxml_child (data, "frame")) == NULL)
		excptn ("Root: <frame> specification missing from <channel>");
	att = sxml_txt (cur);
	np [0] . type = np [1] . type = np [2] . type = TYPE_LONG;
	if (parseNumbers (att, 3, np) != 3)
		excptn ("Root: <frame> format error in <channel> (%s); "
			"three numbers expected",  att);
	brate = (Long) (np [0].LVal);
	bpb = (int) (np [1].LVal);
	frml = (int) (np [2].LVal);
	if (brate <= 0 || bpb <= 0 || frml < 0)
		excptn ("Root: illegal number in <frame> of <channel>: %s",
			att);

	// Create the channel (this sets SEther)
	create RFShadow (NT, STB, n, dref, loss_db, beta, sigm, bn_db, cutoff,
		syncbits, brate, bpb, frml);

	print ("Channel:\n");
	print (form ("  RP(d)/XP [dB] = -10 x %g x log(d/%gm) + X(%g) - %g\n",
			beta, dref, sigm, loss_db));
	print ("  BER Table:           SIR         BER\n");
	for (i = 0; i < n; i++) {
 		print (form ("             %11gdB %11g\n",
			linTodB (STB[i].sir), STB[i].ber));
	}
	print ("\n");
}

void BoardRoot::readNodeParams (sxml_t data, int nn,
/*
 * We may need more of these later
 */
		Long	&mem,
		double	&xp,
		double	&rp,
		Long	&bcmn,
		Long	&bcmx,
		Long	&lbtdel,
		double	&lbtth,
		Long	&pre,
		Long	&eesz,
		Long	&ifsz,
		Long	&ifps,
		Long	&uimo,
		Long	&uomo,
		Long	&uitm,
		Long	&uicn,
		Long	&uocn,
		Long	&uspe,
		Long	&ubsi,
		Long	&uisl,
		char	*&uidv,
		char	*&uodv
	)				{

	nparse_t np [2];
	sxml_t cur;
	const char *att;
	char *str, *as, es [32];
	int len;

	mem	= 0;

	xp 	= HUGE;
	rp 	= HUGE;
	lbtdel 	= MAX_Long;
	lbtth  	= HUGE;
	bcmn	= MAX_Long;
	bcmx	= MAX_Long;

	pre	= MAX_Long;

	uidv = uodv = NULL;
	uisl = 0;
	uimo = UART_IMODE_NONE;
	uomo = UART_OMODE_NONE;
	uitm = UART_IMODE_UNTIMED;
	uicn = UART_IMODE_ASCII;
	uocn = UART_OMODE_ASCII;
	uspe = 0;
	ubsi = 0;

	// EEPROM + UART initialized as absent
	eesz = ifsz = ifps = 0;
	
	if (data == NULL)
		return;

	// This is for error diagnostics
	strcpy (es, (nn < 0) ? "<defaults>" : form ("node %1d", nn));

	print ("Node configuration [");
	if (nn < 0)
		print ("default");
	else
		print (form ("    %3d", nn));
	print ("]:\n");

	/* POWER */
	if ((cur = sxml_child (data, "power")) != NULL) {
		// Both are double
		np [0].type = np [1].type = TYPE_double;
		if (parseNumbers (sxml_txt (cur), 2, np) != 2)
			excptn ("Root: two double numbers required in <power> "
				"in %s", es);
		xp = np [0].DVal;
		rp = np [1].DVal;
		// This is in dBm/dB
		print (form ("  Power:      X=%gdBm, R=%gdB\n", xp, rp));
	}

	/* BACKOFF */
	if ((cur = sxml_child (data, "backoff")) != NULL) {
		// Both are int
		np [0].type = np [1].type = TYPE_LONG;
		if (parseNumbers (sxml_txt (cur), 2, np) != 2)
			excptn ("Root: two int numbers required in <backoff> "
				"in %s", es);
		bcmn = (Long) (np [0].LVal);
		bcmx = (Long) (np [1].LVal);

		if (bcmn < 0 || bcmx < bcmn)
			excptn ("Root: backoff must be '0 <= min <= max', "
				"is (%1d,%1d), in %s", bcmn, bcmx, es);
		print (form ("  Backoff:    min=%1d, max=%1d\n", bcmn, bcmx));
	}

	/* LBT */
	if ((cur = sxml_child (data, "lbt")) != NULL) {
		np [0].type = TYPE_LONG;
		np [1].type = TYPE_double;
		if (parseNumbers (sxml_txt (cur), 2, np) != 2)
			excptn ("Root: two numbers (int, double) required in "
				"<lbt> in %s", es);
		lbtdel = (Long) (np [0].LVal);
		lbtth = np [1].DVal;

		if (lbtdel < 0)
			excptn ("Root: lbt delay be >= 0, is %1d, in %s",
				lbtdel, es);
		print (form ("  LBT:        del=%1d, ths=%g\n", lbtdel, lbtth));
	}

	// Now for the integer ones
	np [0].type = np [1].type = TYPE_LONG;

	/* MEMORY */
	if ((cur = sxml_child (data, "memory")) != NULL) {
		if (parseNumbers (sxml_txt (cur), 1, np) != 1)
			excptn ("Root: one int number required in <memory> in "
				"%s", es);
		mem = (Long) (np [0] . LVal);

		if (mem > 0x00008000)
			excptn ("Root: <memory> too large (%1d) in %s; the "
				"maximum is 32768", mem, es);
		print (form ("  Memory:     %1d bytes\n", mem));
	}

	/* PREAMBLE */
	if ((cur = sxml_child (data, "preamble")) != NULL) {
		if (parseNumbers (sxml_txt (cur), 1, np) != 1)
			excptn ("Root: one int number required in <preamble> "
				"in %s", es);
		pre = (Long) (np [0].LVal);

		if (pre <= 0)
			excptn ("Root: preamble length must be > 0, is %1d, "
				"in %s", pre, es);
		print (form ("  Preamble:   %1d bits\n", pre));
	}

	/* EEPROM + IFLASH */
	if ((cur = sxml_child (data, "eeprom")) != NULL) {
		if (parseNumbers (sxml_txt (cur), 1, np) != 1)
			excptn ("Root: one int number required in <eeprom> "
				"in %s", es);
		eesz = (Long) (np [0].LVal);
		if (eesz < 0 || eesz > 65536)
			excptn ("Root: eeprom size must be >= 0 and <= 65536, "
				"is %1d, in %s", eesz, es);
		if (eesz)
		    printf (form ("  EEPROM:     %1d bytes\n", eesz));
	}

	if ((cur = sxml_child (data, "iflash")) != NULL) {
		len = parseNumbers (sxml_txt (cur), 2, np);
		if (len != 1 && len != 2)
			excptn ("Root: one or two int number required in "
				"<iflash> in %s", es);
		ifsz = (Long) (np [0].LVal);
		if (ifsz < 0 || ifsz > 65536)
			excptn ("Root: iflash size must be >= 0 and <= 65536, "
				"is %1d, in %s", ifsz, es);
		if (len == 2) {
			ifps = (Long) (np [1].LVal);
			if (ifps) {
			    if (ifps < 0 || ifps > ifsz || (ifsz % ifps) != 0)
				excptn ("Root: number of iflash pages, %1d, is "
					"illegal in %s", ifps, es);
			    ifps = ifsz / ifps;
			}
		}
		if (ifsz)
		    printf (form ("  IFLASH:     %1d bytes, page size: %1d\n",
			ifsz, ifps));
	}

	/* The UART */
	if ((data = sxml_child (data, "uart")) == NULL)
		// This part is last, so we can descend with 'data' and
		// forget about the parent
		return;

	/* The rate */
	if ((att = sxml_attr (data, "rate")) == NULL)
		excptn ("Root: 'rate' missing from <uart> in %s", es);

	np [0].type = TYPE_LONG;
	if (parseNumbers (att, 1, np) != 1 || np [0].LVal <= 0)
		excptn ("Root: 'rate' specification (%s) in <uart> for %s is "
			"invalid", att, es);

	uspe = (Long) (np [0].LVal);

	/* Buffer size */
	if ((att = sxml_attr (data, "bsize")) == NULL)
		excptn ("Root: 'bsize' missing from <uart> in %s", es);

	if (parseNumbers (att, 1, np) != 1 || np [0].LVal <= 0)
		excptn ("Root: 'bsize' specification (%s) in <uart> for %s is "
			"invalid", att, es);

	ubsi = (Long) (np [0].LVal);
	print (form ("  UART [rate = %1d bps, bsize = %1d bytes]:\n", uspe,
		ubsi));

	/* The INPUT spec */
	if ((cur = sxml_child (data, "input")) != NULL) {
		str = (char*) sxml_txt (cur);
		if ((att = sxml_attr (cur, "source")) == NULL)
			excptn ("Root: 'source' attribute missing from <uart> "
				"<input ...> in %s", es);
		if (strcmp (att, "none") == 0)
			// Equivalent to 'no uart input spec'
			goto NoUInput;

		print ("    INPUT: ");

		if (strcmp (att, "device") == 0) {
			// Preprocess the string (in place, as it can only
			// shrink). Unfortunately, we cannot have exotic
			// characters in it because 0 is the sentinel.
			len = sanitize_string (str);
			if (len == 0)
				excptn ("Root: device name string in <uart> "
				    "<input> for %s is empty", es);
			// This is a device name
			str [len] = '\0';
			uimo = UART_IMODE_DEVICE;
			uidv = str;
			print (form ("device '%s'", uidv));
		} else if (strcmp (att, "socket") == 0) {
			// Expect something like "hostname:port"
			len = sanitize_string (str);
			if (len == 0)
				excptn ("Root: socket string in <uart> "
				    "<input> for %s is empty", es);
			str [len] = '\0';
			// Locate ':'
			for (as = str; *as != '\0' && *as != ':'; as++);
			if (*as == '\0')
				excptn ("Root: socket string (%s) in <uart> "
				    "<input> for %s contains no ':'", str, es);
			*as = '\0';
			uidv = str;
			uodv = as + 1;
			uimo = UART_IMODE_SOCKET;
			uomo = UART_OMODE_SOCKET;
			print (form ("socket, host: %s, port: %s", uidv, uodv));
		} else if (strcmp (att, "string") == 0) {
			len = sanitize_string (str);
			uidv = str;
			uisl = len;
			uimo = UART_IMODE_STRING;
			print (form ("string '%c%c%c%c ...'",
				uidv [0],
				uidv [1],
				uidv [2],
				uidv [3] ));
		} else {
			excptn ("Root: illegal 'source' (%s) in <uart> "
				"<input> for %s", att, es);
		}

		// Now for the 'type'
		if ((att = sxml_attr (cur, "type")) != NULL) {
			if (strcmp (att, "timed") == 0) {
				uitm = UART_IMODE_TIMED;
				print (", TIMED");
			} else if (strcmp (att, "untimed"))
				excptn ("Root: illegal input 'type' (%s) in "
					"<uart> <input> for %s", att, es);
		}
		// And the coding
		if ((att = sxml_attr (cur, "coding")) != NULL) {
			if (strcmp (att, "hex") == 0) {
				print (", HEX");
				uicn = UART_IMODE_HEX;
			} else if (strcmp (att, "ascii"))
				excptn ("Root: illegal 'coding' (%s) in "
					"<uart> <input> for %s", att, es);
		}
			
		print ("\n");
	}

NoUInput:

	// The OUTPUT spec
	if ((cur = sxml_child (data, "output")) != NULL) {
		str = (char*) sxml_txt (cur);
		if ((att = sxml_attr (cur, "target")) == NULL)
			excptn ("Root: 'target' attribute missing from <uart> "
				"<output ...> in %s", es);
		if (uomo == UART_OMODE_SOCKET) {
			// This must be a socket
			if (strcmp (att, "socket"))
				// but isn't
				excptn ("Root: 'target' for <uart> <output> "
					"(%s) in %s must be 'socket'",
						att, es);
			print ("    OUTPUT: ");
			print (" socket (see INPUT)");
			goto NoUOutput;
		} else if (strcmp (att, "none") == 0)
			// Equivalent to 'no uart output spec'
			goto NoUOutput;

		print ("    OUTPUT: ");

		if (strcmp (att, "device") == 0) {
			len = sanitize_string (str);
			if (len == 0)
				excptn ("Root: device name string in <uart> "
				    "<output> for %s is empty", es);
			// This is a device name
			str [len] = '\0';
			uomo = UART_OMODE_DEVICE;
			uodv = str;
			print (form ("device '%s'", uodv));
		} else if (strcmp (att, "socket") == 0) {
			if (uimo != UART_IMODE_NONE)
				excptn ("Root: 'target' in <uart> <output> (%s)"
					" for %s conflicts with <input> source",
						att, es);
			len = sanitize_string (str);
			if (len == 0)
				excptn ("Root: socket string in <uart> "
				    "<output> for %s is empty", es);
			str [len] = '\0';
			// Locate ':'
			for (as = str; *as != '\0' && *as != ':'; as++);
			if (*as == '\0')
				excptn ("Root: socket string (%s) in <uart> "
				    "<output> for %s contains no ':'", str, es);
			*as = '\0';
			uidv = str;
			uodv = as + 1;
			uomo = UART_OMODE_SOCKET;
			uimo = UART_IMODE_SOCKET;
			print (form ("socket, host: %s, port: %s", uidv, uodv));
		} else {
			excptn ("Root: illegal 'target' (%s) in <uart> "
				"<output> for %s", att, es);
		}
		// The coding
		if ((att = sxml_attr (cur, "coding")) != NULL) {
			if (strcmp (att, "hex") == 0) {
				uocn = UART_OMODE_HEX;
				print (", HEX");
			} else if (strcmp (att, "ascii"))
				excptn ("Root: illegal 'coding' (%s) in "
					"<uart> <output> for %s", att, es);
		}
		print ("\n\n");
	}

NoUOutput:

	NOP;
}

void BoardRoot::initNodes (sxml_t data, int NT) {

	int i, j;
	sxml_t chd, cno, cur;
	nparse_t np [2];
	double *d;
	const char *att;

	/* ============ */
	/* The defaults */
	/* ============ */
	Long	def_mem;

	double	def_xp,
		def_rp,
		def_lbtth;

	Long	def_bcmn,
		def_bcmx,
		def_lbtdel;
	
	Long 	def_pre,

		def_eesz,
		def_ifsz,
		def_ifps,

		def_uimo,
		def_uomo,
		def_uitm,
		def_uicn,
		def_uocn;

	Long	def_uspe,
		def_ubsi,
		def_uisl;

	char	*def_uidv,
		*def_uodv;

	const char	*def_type;

	/* ======== */
	/* Per node */
	/* ======== */
	Long	nod_mem;

	double	nod_xp,
		nod_rp,
		nod_lbtth;

	Long	nod_bcmn,
		nod_bcmx,
		nod_lbtdel;

	Long	nod_pre,

		nod_eesz,
		nod_ifsz,
		nod_ifps,

		nod_uimo,
		nod_uomo,
		nod_uitm,
		nod_uicn,
		nod_uocn;

	Long	nod_uspe,
		nod_ubsi,
		nod_uisl;

	char	*nod_uidv,
		*nod_uodv;

	const char	*nod_type;

	RATE	Rate;

	nod_xp = (double) etuToItu (1.0);
	nod_rp = (double) duToItu (1.0);
	Rate = (RATE) round (nod_xp / SEther->BitRate);

	print ("Timing:\n");
	print (nod_xp, "  ETU (1s) = ", 11, 14);
	print (nod_rp, "  DU  (1m) = ", 11, 14);
	print (Rate,   "  RATE     = ", 11, 14);
	print (getTolerance (&i),
		       "  TOL      = ", 11, 14);
	print (i,      "  TOL QUAL = ", 11, 14);
	print ("\n");

	TheStation = System;

	if ((data = sxml_child (data, "nodes")) == NULL)
		excptn ("Root: <nodes> specification not found");

	// Check for the defaults
	chd = sxml_child (data, "defaults");
	// OK if NULL, readNodeParams will initialize them
	readNodeParams (chd, -1,
			def_mem,
			def_xp,
			def_rp,
			def_bcmn,
			def_bcmx,
			def_lbtdel,
			def_lbtth,
			def_pre,
			def_eesz,
			def_ifsz,
			def_ifps,
			def_uimo,
			def_uomo,
			def_uitm,
			def_uicn,
			def_uocn,
			def_uspe,
			def_ubsi,
			def_uisl,
			def_uidv,
			def_uodv
		);
	def_type = NULL;
	if (chd != NULL)
		// Check for type attribute
		def_type = sxml_attr (chd, "type");

	chd = sxml_child (data, "node");
	if (chd == NULL)
		excptn ("Root: <node> not found");

	// Create all stations
	for (i = 0; i < NT; i++) {
		// Try to locate the node's explicit definition. This
		// isn't terribly efficient, but should be OK for
		// initialization, unless the number of nodes is really
		// huge
		np [0] . type = TYPE_LONG;
		for (j = 0; ; j++) {
			cno = sxml_idx (chd, j);
			if (cno == NULL)
				// Not found - use defaults
				excptn ("Root: <node> entry for node %1d not "
					"found", i);
			att = sxml_attr (cno, "number");
			if (att == NULL)
				excptn ("Root: <node> entry %1d has no "
					"number", j);
			if (parseNumbers (att, 1, np) != 1) 
				excptn ("Root: <node> entry %1d has "
					"incorrect number '%s'", j,
						att);
			if ((Long) (np [0] . LVal) == i)
				// Found
				break;
		}
		// Read the node definitions
		readNodeParams (cno, i,
				nod_mem,
				nod_xp,
				nod_rp,
				nod_bcmn,
				nod_bcmx,
				nod_lbtdel,
				nod_lbtth,
				nod_pre,
				nod_eesz,
				nod_ifsz,
				nod_ifps,
				nod_uimo,
				nod_uomo,
				nod_uitm,
				nod_uicn,
				nod_uocn,
				nod_uspe,
				nod_ubsi,
				nod_uisl,
				nod_uidv,
				nod_uodv
			);

		if ((nod_type = sxml_attr (cno, "type")) == NULL)
			nod_type = def_type;

		if (nod_mem == 0) {
			nod_mem = def_mem;
			if (nod_mem == 0)
			  excptn ("Root: no memory for node %1d", i);
		}

		if (nod_xp == HUGE) {		// Both or neither
			nod_xp = def_xp;
			nod_rp = def_rp;
			if (nod_xp == HUGE)
			  excptn ("Root: power for node %1d is undefined", i);
		}
				
		if (nod_bcmn == MAX_Long) {
			nod_bcmn = def_bcmn;
			nod_bcmx = def_bcmx;
			if (nod_bcmn == MAX_Long)
			  excptn ("Root: backoff for node %1d is undefined", i);
		}

		if (nod_pre == MAX_Long) {
			nod_pre = def_pre;
			if (nod_pre == MAX_Long)
			  excptn ("Root: preamble for node %1d is undefined",
				i);
		}
				
		if (nod_lbtdel == MAX_Long) {
			nod_lbtdel = def_lbtdel;
			nod_lbtth = def_lbtth;
			if (nod_lbtdel == MAX_Long)
			  excptn ("Root: LBT parameters for node %1d are "
				"undefined", i);
		}

		if (nod_eesz == 0)
			nod_eesz = def_eesz;

		if (nod_ifsz == 0) {
			nod_ifsz = def_ifsz;
			nod_ifps = def_ifps;
		}
				
		if (nod_uspe == 0) {
			nod_uimo = def_uimo;
			nod_uomo = def_uomo;
			nod_uitm = def_uitm;
			nod_uicn = def_uicn;
			nod_uocn = def_uocn;
			nod_uspe = def_uspe;
			nod_uisl = def_uisl;
			nod_uidv = def_uidv;
			nod_uodv = def_uodv;
			nod_ubsi = def_ubsi;
		}

		// Location
		if ((cur = sxml_child (cno, "location")) == NULL)
			excptn ("Root: no <location> for node %1d", i);

		att = sxml_txt (cur);
		np [0] . type = np [1] . type = TYPE_double;
		if (parseNumbers (att, 2, np) != 2 ||
		    np[0].DVal < 0.0 || np[1].DVal < 0.0)
			excptn ("Root: illegal location spec (%s) for node %1d",
				att, i);

		buildNode (
			nod_type,			// Type or NULL
			nod_mem,			// Memory
			np [0].DVal,			// X
			np [1].DVal,			// Y
			nod_xp, nod_rp,			// Power
			nod_bcmn, nod_bcmx,		// Backoff
			nod_lbtdel, nod_lbtth,		// LBT
			Rate,
			nod_pre,			// Preamble
			nod_eesz, nod_ifsz, nod_ifps,
			nod_uimo | nod_uomo | nod_uitm | nod_uicn |
				nod_uocn | nod_uisl,
			nod_ubsi, nod_uspe,
			nod_uidv, nod_uodv
		    );
	}

	// Minimum distance
	SEther->setMinDistance (SEther->RDist);
	// We need this for ANYEVENT
	SEther->setAevMode (NO);
}

void BoardRoot::initAll () {

	sxml_t xml;
	const char *att;
	nparse_t np [1];
	int NN;

	xml = sxml_parse_input ();
	if (!sxml_ok (xml))
		excptn ("Root: XML input data error, %s",
			(char*)sxml_error (xml));
	if (strcmp (sxml_name (xml), "network"))
		excptn ("Root: <network> data expected");

	// Decode the number of stations
	if ((att = sxml_attr (xml, "nodes")) == NULL)
		excptn ("Root: 'nodes' attribute not found in <network>");

	np [0] . type = TYPE_LONG;
	if (parseNumbers (att, 1, np) != 1)
		excptn ("Root: the 'nodes' attribute (%s) of <network> is "
			"invalid", att);

	NN = (int) (np [0] . LVal);
	if (NN <= 0)
		excptn ("Root: 'nodes' in <network> must be strictly positive, "
			"is %1d", NN);

	initTiming (xml);
	initChannel (xml, NN);
	initNodes (xml, NN);

	sxml_free (xml);

	setResync (500, 0.5);
}

// ======================================================== //
// Here is the part to run under the fake PicOS environment //
// ======================================================== //

#include "stdattr.h"

void Inserial::setup () {

	uart = S->uart;
	assert (uart->pcsInserial == NULL,
		"Inserial->setup: duplicated process");
	uart->pcsInserial = this;
}

void Inserial::close () {
	terminate ();
	uart->pcsInserial = NULL;
	sleep;
}

Inserial::perform {

    int quant;

    state IM_INIT:

	if (uart->__inpline != NULL)
		/* Never overwrite previous unclaimed stuff */
		close ();

	if ((tmp = ptr = (char*) umalloc (MAX_LINE_LENGTH + 1)) == NULL) {
		/*
		 * We have to wait for memory
		 */
		umwait (IM_INIT);
		sleep;
	}
	len = MAX_LINE_LENGTH;

    transient IM_READ:

	io (IM_READ, 0, READ, ptr, 1);
	if (ptr == tmp) { // new line
		if (*ptr == '\0') { // bin cmd
			ptr++;
			len--;
			proceed IM_BIN;
		}

		if (*ptr < 0x20)
			/* Ignore codes below space at the beginning of line */
			proceed IM_READ;
	}
	if (*ptr == '\n' || *ptr == '\r') {
		*ptr = '\0';
		uart->__inpline = tmp;
		close ();
	}

	if (len) {
		ptr++;
		len--;
	}

	proceed IM_READ;

    state IM_BIN:

	io (IM_BIN, 0, READ, ptr, 1);
	if (--len > *ptr +1) // 1 for 0x04
		len = *ptr +1;
	ptr++;

    transient IM_BIN1:

	quant = io (IM_BIN1, 0, READ, ptr, len);
	len -= quant;
	if (len == 0) {
		uart->__inpline = tmp;
		close ();
	}
	ptr += quant;
	proceed IM_BIN1;

}

void Outserial::setup (const char *d) {

	uart = S->uart;
	assert (uart->pcsOutserial == NULL,
		"Outserial->setup: duplicated process");
	uart->pcsOutserial = this;
	ptr = data = d;
}

void Outserial::close () {
	terminate ();
	uart->pcsOutserial = NULL;
	sleep;
}

Outserial::perform {

    int quant;

    state OM_INIT:

	if (*ptr)
		len = strlen (ptr);
	else
		len = ptr [1] +3; // 3: 0x00, len, 0x04

    transient OM_WRITE:

	if (len == 0) {
		/* This is always a fresh buffer allocated dynamically */
		ufree (data);
		close ();
	}

    transient OM_RETRY:

	quant = io (OM_RETRY, 0, WRITE, (char*)ptr, len);
	ptr += quant;
	len -= quant;
	proceed OM_WRITE;
}

#include "stdattr_undef.h"

#include "uart.cc"
#include "rfmodule_dm2200.cc"
#include "net.cc"
#include "plug_null.cc"
#include "plug_tarp.cc"
#include "tarp.cc"

#endif
