#ifndef __picos_board_c__
#define __picos_board_c__

#include "board.h"

#include "chan_shadow.cc"
#include "encrypt.cc"
#include "nvram.cc"
#include "agent.h"

const char	zz_hex_enc_table [] = {
				'0', '1', '2', '3', '4', '5', '6', '7',
				'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
			      };

static	RATE	XmitRate;	// This one is common for all nodes

struct strpool_s {

	const byte *STR;
	int Len;
	struct strpool_s *Next;
};

typedef	struct strpool_s strpool_t;

static	strpool_t *STRPOOL = NULL;

static	const byte *find_strpool (const byte *str, int len, Boolean cp) {

	strpool_t *p;
	const byte *s0, *s1;
	int l;

	for (p = STRPOOL; p != NULL; p = p->Next) {
		if (len != p->Len)
			continue;
		s0 = str;
		s1 = p->STR;
		l = len;
		while (l--)
			if (*s0++ != *s1++)
				break;
		if (l)
			continue;
		// Found
		return p->STR;
	}

	// Not found
	p = new strpool_t;
	if (cp) {
		// Copy the string
		p->STR = new byte [len];
		memcpy ((void*)(p->STR), str, len);
	} else {
		p->STR = str;
	}
	p->Len = len;
	p->Next = STRPOOL;
	STRPOOL = p;

	return p->STR;
}

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

	if (pins != NULL)
		pins->rst ();

	if (ledsm != NULL)
		ledsm->rst ();

	_da (entropy) = 0;
	_da (statid) = 0;

	_da (Receiving) = _da (Xmitting) = NO;
	_da (TXOFF) = _da (RXOFF) = YES;

	_da (OBuffer).fill (NONE, NONE, 0, 0, 0);

	// This will do the dynamic initialization of static stuff in TCV
	_da (tcv_init) ();
}

void PicOSNode::setup (data_no_t *nd) {

	// Turn this into a trigger mailbox
	TB.setLimit (-1);

	MTotal = (nd->Mem + 3) / 4;			// This is in full words
	MHead = MTail = NULL;

	// These two survive reset. We assume that they are never changed
	// by the application.
	_da (min_backoff) = (word) (nd->rf->BCMin);
	// This is the argument for 'toss' to generate the proper
	// offset. The consistency has been verified by readNodeParams.
	_da (max_backoff) = (word) (nd->rf->BCMax) - _da (min_backoff) + 1;

	// Same about these two
	if (nd->rf->LBTDel == 0) {
		// Disable it
		_da (lbt_threshold) = HUGE;
		_da (lbt_delay) = 0;
	} else {
		_da (lbt_threshold) = dBToLin (nd->rf->LBTThs);
		_da (lbt_delay) = (word) (nd->rf->LBTDel);
	}

	if (nd->ua == NULL) {
		// No UART
		uart = NULL;
	} else {
		uart = new uart_t;
		uart->U = new UART (nd->ua);
	}

	if (nd->pn == NULL) {
		// No PINS module
		pins = NULL;
	} else {
		pins = new PINS (nd->pn);
	}

	if (nd->le == NULL) {
		ledsm = NULL;
	} else {
		ledsm = new LEDSM (nd->le);
	}

	DefXPower = dBToLin (nd->rf->XP);
	DefRPower = dBToLin (nd->rf->RP);

	_da (RFInterface) = create Transceiver (
			XmitRate,
			(Long)(nd->rf->Pre),
			DefXPower,
			DefRPower,
			nd->X, nd->Y );

	Ether->connect (_da (RFInterface));

	// EEPROM and IFLASH: note that they are not resettable
	eeprom = NULL;
	iflash = NULL;
	if (nd->ep != NULL) {
		data_ep_t *EP = nd->ep;
		if (EP->EEPRS)
			eeprom = new NVRAM (EP->EEPRS, EP->EEPPS, EP->EFLGS,
				EP->bounds);
		if (EP->IFLSS)
			iflash = new NVRAM (EP->IFLSS, EP->IFLPS, 
				NVRAM_TYPE_NOOVER | NVRAM_TYPE_ERPAGE, NULL);
	}

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

void PicOSNode::no_pin_module (const char *fn) {

	excptn ("%s: no PINS module at %s", fn, getSName ());
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

#define	scani(at)	{ unsigned at *vap; Boolean mf; \
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

word _dad (PicOSNode, ee_read) (lword adr, byte *buf, word n) {

	sysassert (eeprom != NULL, "ee_read no eeprom");
	return eeprom->get (adr, buf, (lword) n);
};

word _dad (PicOSNode, ee_write) (word st, lword adr, const byte *buf, word n) {

	sysassert (eeprom != NULL, "ee_write no eeprom");
	eeprom->put (st, adr, buf, (lword) n);
};

word _dad (PicOSNode, ee_erase) (word st, lword fr, lword up) {

	sysassert (eeprom != NULL, "ee_erase no eeprom");
	eeprom->erase (st, fr, up);
};

word _dad (PicOSNode, ee_sync) (word st) {

	sysassert (eeprom != NULL, "ee_sync no eeprom");
	eeprom->sync (st);
};

int _dad (PicOSNode, if_write) (word adr, word w) {

	sysassert (iflash != NULL, "if_write no iflash");
	iflash->put (WNONE, (lword) adr << 1, (const byte*) (&w), 2);
	return 0;
};

word _dad (PicOSNode, if_read) (word adr) {

	word w;

	sysassert (iflash != NULL, "if_read no iflash");
	iflash->get ((lword) adr << 1, (byte*) (&w), 2);
	return w;
};

void _dad (PicOSNode, if_erase) (int a) {

	sysassert (iflash != NULL, "if_erase no iflash");

	if (a < 0) {
		iflash->erase (WNONE, 0, 0);
	} else {
		a <<= 1;
		iflash->erase (WNONE, (lword)a, (lword)a);
	}
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

static void xenf (const char *s, const char *w) {
	excptn ("Root: %s specification not found within %s", s, w);
}

static void xemi (const char *s, const char *w) {
	excptn ("Root: %s attribute missing from %s", s, w);
}

static void xeai (const char *s, const char *w, const char *v) {
	excptn ("Root: attribute %s in %s has invalid value: %s", s, w, v);
}

static void xevi (const char *s, const char *w, const char *v) {
	excptn ("Root: illegal %s value in %s: %s", s, w, v);
}

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
		xenf ("<channel>", "<network>");

	// At the moment, we handle shadowing models only
	if ((cur = sxml_child (data, "shadowing")) == NULL)
		xenf ("<shadowing>", "<channel>");

	if ((att = sxml_attr (cur, "bn")) == NULL)
		xemi ("'bn'", "<shadowing> for <channel>");

	np [0].type = TYPE_double;
	if (parseNumbers (att, 1, np) != 1)
		xeai ("'bn'", "<shadowing> for <channel>", att);

	bn_db = np [0].DVal;

	if ((att = sxml_attr (cur, "syncbits")) == NULL)
		xemi ("'syncbits'", "<shadowing> for <channel>");

	np [0].type = TYPE_LONG;
	if (parseNumbers (att, 1, np) != 1)
		xeai ("'syncbits'", "<shadowing> for <channel>", att);

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
		xenf ("<ber>", "<channel>");

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
			xevi ("<cutoff>", "<channel>", att);
		cutoff = np [0].DVal;
	}

	// Frame parameters
	if ((cur = sxml_child (data, "frame")) == NULL)
		xenf ("<frame>", "<channel>");
	att = sxml_txt (cur);
	np [0] . type = np [1] . type = np [2] . type = TYPE_LONG;
	if (parseNumbers (att, 3, np) != 3)
		xevi ("<frame>", "<channel>", att);

	brate = (Long) (np [0].LVal);
	bpb = (int) (np [1].LVal);
	frml = (int) (np [2].LVal);
	if (brate <= 0 || bpb <= 0 || frml < 0)
		xevi ("<frame>", "<channel>", att);

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

data_no_t *BoardRoot::readNodeParams (sxml_t data, int nn) {

	nparse_t np [2 + EP_N_BOUNDS];
	sxml_t cur;
	const char *att;
	char *str, *as, es [32];
	int i, len;
	data_rf_t *RF;
	data_ep_t *EP;
	data_no_t *ND;

	ND = new data_no_t;
	ND->Mem = 0;
	// These ones are not set here, placeholders only to be set by
	// the caller
	ND->X = ND->Y = 0.0;

	// This one is always present (not optional)
	RF = ND->rf = new data_rf_t;
	RF->LBTThs = RF->XP = RF->RP = HUGE;
	RF->Pre = RF->LBTDel = RF->BCMin = RF->BCMax = WNONE;

	// The optionals
	ND->ua = NULL;
	ND->ep = NULL;
	ND->pn = NULL;
	ND->le = NULL;

	if (data == NULL)
		// This is how we stand so far
		return ND;

	// This is for error diagnostics
	strcpy (es, (nn < 0) ? "<defaults>" : form ("node %1d", nn));

	print ("Node configuration [");

	if (nn < 0)
		print ("default");
	else
		print (form ("    %3d", nn));
	print ("]:\n");

/* ====== */
/* MEMORY */
/* ====== */

	if ((cur = sxml_child (data, "memory")) != NULL) {
		np [0].type = np [1].type = TYPE_LONG;
		if (parseNumbers (sxml_txt (cur), 1, np) != 1)
			xevi ("<memory>", es, sxml_txt (cur));
		ND->Mem = (word) (np [0] . LVal);
		if (ND->Mem > 0x00008000)
			excptn ("Root: <memory> too large (%1d) in %s; the "
				"maximum is 32768", ND->Mem, es);
		print (form ("  Memory:     %1d bytes\n", ND->Mem));
	}

/* ========= */
/* RF MODULE */
/* ========= */

	/* POWER */
	if ((cur = sxml_child (data, "power")) != NULL) {
		// Both are double
		np [0].type = np [1].type = TYPE_double;
		if (parseNumbers (sxml_txt (cur), 2, np) != 2)
			excptn ("Root: two double numbers required in <power> "
				"in %s", es);
		RF->XP = np [0].DVal;
		RF->RP = np [1].DVal;
		// This is in dBm/dB
		print (form ("  Power:      X=%gdBm, R=%gdB\n", RF->XP,
			RF->RP));
	}

	/* BACKOFF */
	if ((cur = sxml_child (data, "backoff")) != NULL) {
		// Both are int
		np [0].type = np [1].type = TYPE_LONG;
		if (parseNumbers (sxml_txt (cur), 2, np) != 2)
			excptn ("Root: two int numbers required in <backoff> "
				"in %s", es);
		RF->BCMin = (word) (np [0].LVal);
		RF->BCMax = (word) (np [1].LVal);

		if (RF->BCMax < RF->BCMin)
			xevi ("<backoff>", es, sxml_txt (cur));

		print (form ("  Backoff:    min=%1d, max=%1d\n", RF->BCMin,
			RF->BCMax));
	}

	/* LBT */
	if ((cur = sxml_child (data, "lbt")) != NULL) {
		np [0].type = TYPE_LONG;
		np [1].type = TYPE_double;
		if (parseNumbers (sxml_txt (cur), 2, np) != 2)
			xevi ("<lbt>", es, sxml_txt (cur));
		RF->LBTDel = (word) (np [0].LVal);
		RF->LBTThs = np [1].DVal;

		print (form ("  LBT:        del=%1d, ths=%g\n", RF->LBTDel,
			RF->LBTThs));
	}

	/* PREAMBLE */
	if ((cur = sxml_child (data, "preamble")) != NULL) {
		np [0].type = np [1].type = TYPE_LONG;
		if (parseNumbers (sxml_txt (cur), 1, np) != 1)
			xevi ("<preamble>", es, sxml_txt (cur));
		RF->Pre = (word) (np [0].LVal);
		print (form ("  Preamble:   %1d bits\n", RF->Pre));
	}

	print ("\n");

/* ============ */
/* EEPROM & FIM */
/* ============ */

	EP = NULL;
	/* EEPROM */
	np [0].type = np [1].type = TYPE_LONG;
	for (i = 0; i < EP_N_BOUNDS; i++)
		np [i + 2] . type = TYPE_double;

	if ((cur = sxml_child (data, "eeprom")) != NULL) {

		lword pgsz;

		if (EP == NULL) {
			EP = ND->ep = new data_ep_t;
			// Flag: FIM still inheritable from defaults
			EP->IFLSS = WNONE;
		}

		EP->EFLGS = 0;

		if ((att = sxml_attr (cur, "erase")) != NULL) {
			if (strcmp (att, "block") == 0 || strcmp (att, "page")
			    == 0)
				EP->EFLGS |= NVRAM_TYPE_ERPAGE;
			else if (strcmp (att, "byte") != 0)
				xeai ("erase", "eeprom", att);
		}

		if ((att = sxml_attr (cur, "overwrite")) != NULL) {
			if (strcmp (att, "no") == 0)
				EP->EFLGS |= NVRAM_TYPE_NOOVER;
			else if (strcmp (att, "yes") != 0)
				xeai ("overwrite", "eeprom", att);
		}

		len = parseNumbers (sxml_txt (cur), EP_N_BOUNDS + 2, np);
		if (len == 0)
			excptn ("Root: at least one int number required in "
				"<eeprom> in %s", es);

		EP->EEPRS = (lword) (np [0] . LVal);
		EP->EEPPS = 0;
		for (i = 0; i < EP_N_BOUNDS; i++)
			EP->bounds [i] = 0.0;

		// Check for pagesize and timing params
		if (EP->EEPRS && len > 1) {
			pgsz = (lword) (np [1] . LVal);
			if (pgsz) {
				// This is the number of pages, so turn it into
				// a page size
				if (pgsz > EP->EEPRS || (EP->EEPRS % pgsz) != 0)
					excptn ("Root: number of eeprom pages, "
						"%1d, is illegal in %s",
							pgsz, es);
				pgsz = EP->EEPRS / pgsz;
			}
			EP->EEPPS = pgsz;
			for (i = 0; i < EP_N_BOUNDS; i++) {
				if (i + 2 >= len)
					break;
				EP->bounds [i] =
					np [i + 2] . DVal;
			}
		} 

		for (i = 0; i < EP_N_BOUNDS; i += 2) {
			if (EP->bounds [i] != 0.0 && EP->bounds [i+1] == 0.0)
				EP->bounds [i+1] = EP->bounds [i];
			if (EP->bounds [i] < 0.0 || EP->bounds [i+1] <
			    EP->bounds [i] )
				excptn ("Root: timing distribution parameters "
					"for eeprom: %1g %1g are illegal in %s",
						EP->bounds [i],
						EP->bounds [i+1],
						es);
		}

		if (EP->EEPRS) {
		   	print (form (
				"  EEPROM:     %1d bytes, page size: %1d\n",
					EP->EEPRS, EP->EEPPS));
			print (form (
				"              W: [%1g,%1g], E: [%1g,%1g], "
							"S: [%1g,%1g]\n",
					EP->bounds [0],
					EP->bounds [1],
					EP->bounds [2],
					EP->bounds [3],
					EP->bounds [4],
					EP->bounds [5],
					EP->bounds [6]));
		} else
			      print ("  EEPROM:     none\n");
	}

	if ((cur = sxml_child (data, "iflash")) != NULL) {

		Long ifsz, ifps;

		if (EP == NULL) {
			EP = ND->ep = new data_ep_t;
			// Flag: EEPROM still inheritable from defaults
			EP->EEPRS = WNONE;
		}

		len = parseNumbers (sxml_txt (cur), 2, np);
		if (len != 1 && len != 2)
			xevi ("<iflash>", es, sxml_txt (cur));
		ifsz = (Long) (np [0].LVal);
		if (ifsz < 0 || ifsz > 65536)
			excptn ("Root: iflash size must be >= 0 and <= 65536, "
				"is %1d, in %s", ifsz, es);
		ifps = ifsz;
		if (len == 2) {
			ifps = (Long) (np [1].LVal);
			if (ifps) {
			    if (ifps < 0 || ifps > ifsz || (ifsz % ifps) != 0)
				excptn ("Root: number of iflash pages, %1d, is "
					"illegal in %s", ifps, es);
			    ifps = ifsz / ifps;
			}
		}
		EP->IFLSS = (word) ifsz;
		EP->IFLPS = (word) ifps;
		if (ifsz) 
		    	print (form (
				"  IFLASH:     %1d bytes, page size: %1d\n",
					ifsz, ifps));
		else
			 print ("  IFLASH:     none\n");
	}

	if (EP != NULL) {
		// Make this flag consistent
		EP->absent = (EP->EEPRS == 0 && EP->IFLSS == 0);
		print ("\n");
	}

/* ==== */
/* LEDS */
/* ==== */

	ND->le = readLedsParams (data, es);

/* ==== */
/* UART */
/* ==== */

	ND->ua = readUartParams (data, es);

/* ==== */
/* PINS */
/* ==== */

	ND->pn = readPinsParams (data, es);

	return ND;
}

data_ua_t *BoardRoot::readUartParams (sxml_t data, const char *esn) {
/*
 * Decodes UART parameters
 */
	sxml_t cur;
	nparse_t np [2];
	const char *att;
	char *str, *sts;
	char es [48];
	data_ua_t *UA;
	int len;

	if ((data = sxml_child (data, "uart")) == NULL)
		return NULL;

	strcpy (es, "<uart> for ");
	strcat (es, esn);

	UA = new data_ua_t;

	/* The rate */
	if ((att = sxml_attr (data, "rate")) == NULL)
		xemi ("'rate'", es);

	np [0].type = TYPE_LONG;
	if (parseNumbers (att, 1, np) != 1 || np [0].LVal <= 0)
		xeai ("'rate'", es, att);

	UA->URate = (word) (np [0].LVal);

	/* Buffer size */
	if ((att = sxml_attr (data, "bsize")) == NULL)
		xemi ("'bsize'", es);

	if (parseNumbers (att, 1, np) != 1 || np [0].LVal <= 0)
		xeai ("'bsize'", es, att);

	UA->UBSize = (word) (np [0].LVal);
	print (form ("  UART [rate = %1d bps, bsize = %1d bytes]:\n", UA->URate,
		UA->UBSize));

	UA->UMode = 0;
	UA->UIDev = UA->UODev = NULL;

	/* The INPUT spec */
	if ((cur = sxml_child (data, "input")) != NULL) {
		str = (char*) sxml_txt (cur);
		if ((att = sxml_attr (cur, "source")) == NULL)
			xemi ("'source'", es);
		if (strcmp (att, "none") == 0) {
			// Equivalent to 'no uart input spec'
			goto NoUInput;
		}

		print ("    INPUT:  ");

		if (strcmp (att, "device") == 0) {
			// Preprocess the string (in place, as it can only
			// shrink). Unfortunately, we cannot have exotic
			// characters in it because 0 is the sentinel.
			len = sanitize_string (str);
			if (len == 0)
				xevi ("<input> device string", es, "-empty-");
			// This is a device name
			str [len] = '\0';
			UA->UMode |= XTRN_IMODE_DEVICE;
			UA->UIDev = str;
			print (form ("device '%s'", str));
		} else if (strcmp (att, "socket") == 0) {
			// No string
			UA->UMode |= XTRN_IMODE_SOCKET | XTRN_OMODE_SOCKET;
			print ("socket");
		} else if (strcmp (att, "string") == 0) {
			len = sanitize_string (str);
			// We shall copy the string, such that the UART
			// constructor won't have to. This should be more
			// economical.
			sts = (char*) find_strpool ((const byte*) str, len + 1,
				YES);
			UA->UIDev = sts;
			UA->UMode |= (XTRN_IMODE_STRING | len);
			print (form ("string '%c%c%c%c ...'",
				sts [0],
				sts [1],
				sts [2],
				sts [3] ));
		} else {
			xeai ("'source'", es, att);
		}

		// Now for the 'type'
		if ((att = sxml_attr (cur, "type")) != NULL) {
			if (strcmp (att, "timed") == 0) {
				UA->UMode |= XTRN_IMODE_TIMED;
				print (", TIMED");
			} else if (strcmp (att, "untimed"))
				xeai ("'type'", es, att);
		}
		// And the coding
		if ((att = sxml_attr (cur, "coding")) != NULL) {
			if (strcmp (att, "hex") == 0) {
				print (", HEX");
				UA->UMode |= XTRN_IMODE_HEX;
			} else if (strcmp (att, "ascii"))
				xeai ("'coding'", es, att);
		}
			
		print ("\n");
	}

NoUInput:

	// The OUTPUT spec
	if ((cur = sxml_child (data, "output")) != NULL) {
		str = (char*) sxml_txt (cur);
		if ((att = sxml_attr (cur, "target")) == NULL)
			xemi ("'target'", es);
		if ((UA->UMode & XTRN_OMODE_MASK) == XTRN_OMODE_SOCKET) {
			// This must be a socket
			if (strcmp (att, "socket"))
				// but isn't
				excptn ("Root: 'target' for <uart> <output> "
					"(%s) in %s must be 'socket'",
						att, es);
			print ("    OUTPUT: ");
			print ("socket (see INPUT)");
			goto CheckOType;
		} else if (strcmp (att, "none") == 0)
			// Equivalent to 'no uart output spec'
			goto NoUOutput;

		print ("    OUTPUT: ");

		if (strcmp (att, "device") == 0) {
			len = sanitize_string (str);
			if (len == 0)
				xevi ("<output> device string", es, "-empty-");
			// This is a device name
			str [len] = '\0';
			UA->UMode |= XTRN_OMODE_DEVICE;
			UA->UODev = str;
			print (form ("device '%s'", str));
		} else if (strcmp (att, "socket") == 0) {
			if ((UA->UMode & XTRN_IMODE_MASK) != XTRN_IMODE_NONE)
				excptn ("Root: 'target' in <uart> <output> (%s)"
					" for %s conflicts with <input> source",
						att, es);
			UA->UMode |= (XTRN_OMODE_SOCKET | XTRN_IMODE_SOCKET);
			print ("socket");
CheckOType:
			// Check the type attribute
			if ((att = sxml_attr (cur, "type")) != NULL) {
				if (strcmp (att, "held") == 0 ||
				    strcmp (att, "hold") == 0 ||
				    strcmp (att, "wait") == 0 ) {
					print (", HELD");
					// Hold output until connected
					UA->UMode |= XTRN_OMODE_HOLD;
				}
				// Ignore other types for now; we may need more
				// later
			}
		} else {
			xeai ("'target'", es, att);
		}

		// The coding
		if ((att = sxml_attr (cur, "coding")) != NULL) {
			if (strcmp (att, "hex") == 0) {
				UA->UMode |= XTRN_OMODE_HEX;
				print (", HEX");
			} else if (strcmp (att, "ascii"))
				xeai ("'coding'", es, att);
		}
		print ("\n\n");
	}

NoUOutput:

	// Check if the UART is there after all this parsing
	UA->absent = ((UA->UMode & (XTRN_OMODE_MASK | XTRN_IMODE_MASK)) == 0);

	return UA;
}

data_pn_t *BoardRoot::readPinsParams (sxml_t data, const char *esn) {
/*
 * Decodes PINS parameters
 */
	double d;
	sxml_t cur;
	nparse_t np [2], *npp;
	const char *att;
	char es [48];
	char *str, *sts;
	data_pn_t *PN;
	byte *BS;
	short *SS;
	int len;
	byte pn;

	if ((data = sxml_child (data, "pins")) == NULL)
		return NULL;

	strcpy (es, "<pins> for ");
	strcat (es, esn);

	PN = new data_pn_t;

	PN->NP = (byte) (np [0].LVal);
	PN->PMode = 0;

	PN->NA = 0;
	PN->MPIN = PN->NPIN = PN->D0PIN = PN->D1PIN = BNONE;
	PN->ST = PN->IV = NULL;
	PN->VO = NULL;
	PN->PIDev = PN->PODev = NULL;
	PN->absent = NO;

	/* Total number of pins */
	if ((att = sxml_attr (data, "total")) == NULL &&
	    (att = sxml_attr (data, "number")) == NULL)
		xemi ("'total'", es);

	np [0].type = np [1].type = TYPE_LONG;
	
	if (parseNumbers (att, 1, np) != 1 || np [0].LVal < 0 ||
	    np [0].LVal > 254)
		xeai ("'total'", es, att);

	if (np [0].LVal == 0) {
		// An explicit way to say that there are no PINS
		PN->absent = YES;
		return PN;
	}
	PN->NP = (byte) (np [0].LVal);

	/* ADC pins */
	if ((att = sxml_attr (data, "adc")) != NULL) {
		if (parseNumbers (att, 1, np) != 1 || np [0].LVal < 0 ||
	    	  np [0].LVal > PN->NP)
		    xeai ("'adc'", es, att);
		PN->NA = (byte) (np [0].LVal);
	}

	/* Pulse monitor */
	if ((att = sxml_attr (data, "pulse")) != NULL) {
		if (parseNumbers (att, 2, np) != 2 || np [0].LVal < 0 ||
		  np [0].LVal >= PN->NP || np [1].LVal < 0 || np [1].LVal >=
		    PN->NP || np [0].LVal == np [1].LVal)
		        xeai ("'pulse'", es, att);
		PN->MPIN = (byte) (np [0].LVal);
		PN->NPIN = (byte) (np [1].LVal);
	}

	/* DAC */
	if ((att = sxml_attr (data, "dac")) != NULL) {
		// This condition looks nasty. Shouldn't we be using some
		// table to store the pins booked so far?
		len = parseNumbers (att, 2, np);
		if (len < 1 || len > 2)
	        	xeai ("'dac'", es, att);
		if (np [0].LVal < 0 || np [0].LVal >= PN->NP ||
		     np [0].LVal == PN->MPIN || np [0].LVal == PN->NPIN)
	        	xeai ("'dac'", es, att);
		// The firs one is OK
		PN->D0PIN = (byte) (np [0].LVal);
		if (len == 2) {
			// Verify the second one
			if (np [1].LVal < 0 || np [1].LVal >= PN->NP ||
			  np [0].LVal == np [1].LVal || np [1].LVal == PN->MPIN
			    || np [1].LVal == PN->NPIN)
	        	      xeai ("'dac'", es, att);
			PN->D1PIN = (byte) (np [1].LVal);
		}
	}

	print (form ("  PINS [total = %1d, adc = %1d", PN->NP, PN->NA));
	if (PN->MPIN != BNONE)
		print (form (", PM = %1d, EN = %1d", PN->MPIN, PN->NPIN));
	if (PN->D0PIN != BNONE) {
		print (form (", DAC = %1d", PN->D0PIN));
		if (PN->D1PIN != BNONE)
			print (form ("+%1d", PN->D1PIN));
	}
	print ("]:\n");

	/* I/O */
			  
	/* The INPUT spec */
	if ((cur = sxml_child (data, "input")) != NULL) {
		str = (char*) sxml_txt (cur);
		if ((att = sxml_attr (cur, "source")) == NULL)
			xemi ("'source'", es);
		if (strcmp (att, "none") == 0) {
			// No input
			goto NoPInput;
		}

		print ("    INPUT:  ");

		if (strcmp (att, "device") == 0) {
			// Preprocess the string (in place, as it can only
			// shrink). Unfortunately, we cannot have exotic
			// characters in it because 0 is the sentinel.
			len = sanitize_string (str);
			if (len == 0)
				xevi ("<input> device string", es, "-empty-");
			// This is a device name
			str [len] = '\0';
			PN->PMode |= XTRN_IMODE_DEVICE;
			PN->PIDev = str;
			print (form ("device '%s'", str));
		} else if (strcmp (att, "socket") == 0) {
			// No string
			PN->PMode |= XTRN_IMODE_SOCKET | XTRN_OMODE_SOCKET;
			print ("socket");
		} else {
			xeai ("'source'", es, att);
		}

		print ("\n");
	}

NoPInput:

	// The OUTPUT spec
	if ((cur = sxml_child (data, "output")) != NULL) {
		str = (char*) sxml_txt (cur);
		if ((att = sxml_attr (cur, "target")) == NULL)
			xemi ("'target'", es);
		if ((PN->PMode & XTRN_OMODE_MASK) == XTRN_OMODE_SOCKET) {
			// This must be a socket
			if (strcmp (att, "socket"))
				// but isn't
				excptn ("Root: 'target' for <pins> <output> "
					"(%s) in %s must be 'socket'",
						att, es);
			print ("    OUTPUT: socket (see INPUT)\n");
		} else {

			print ("    OUTPUT: ");

			if (strcmp (att, "device") == 0) {

				len = sanitize_string (str);
				if (len == 0)
					xevi ("<output> device string", es,
						"-empty-");
				// This is a device name
				str [len] = '\0';
				PN->PMode |= XTRN_OMODE_DEVICE;
				PN->PODev = str;
				print (form ("device '%s'", str));

			} else if (strcmp (att, "socket") == 0) {

				if ((PN->PMode & XTRN_IMODE_MASK) !=
				    XTRN_IMODE_NONE)
					excptn ("Root: 'target' in <pins> "
					    "<output> (%s) for %s conflicts "
						"with <input> source", att, es);

				PN->PMode |= (XTRN_OMODE_SOCKET |
					XTRN_IMODE_SOCKET);
				print ("socket");
	
			} else if (strcmp (att, "none") != 0) {
	
				xeai ("'target'", es, att);
			}
			print ("\n");
		}
	}

	/* Pin status */
	if ((cur = sxml_child (data, "status")) != NULL) {
		BS = new byte [len = ((PN->NP + 7) >> 3)];
		// The default is "pin availalble"
		memset (BS, 0xff, len);
		str = (char*)sxml_txt (cur);
		if (sanitize_string (str) == 0)
			xevi ("<status> string", es, "-empty-");
		
		sts = str;
		for (pn = 0; pn < PN->NP; pn++) {
			// Find next digit in sts
			while (isspace (*sts))
				sts++;
			if (*sts == '\0')
				break;
			if (*sts == '1')
				PINS::sbit (BS, pn);
			else if (*sts != '0')
				xevi ("<status>", es, str);
		}
		print ("    STATUS: ");
		for (pn = 0; pn < PN->NP; pn++)
			print (PINS::gbit (BS, pn) ? "1" : "0");
		print ("\n");
		PN->ST = find_strpool ((const byte*)BS, len, NO);
		if (PN->ST != BS)
			// Recycled
			delete BS;
	}

	/* Default (initial) pin values */
	if ((cur = sxml_child (data, "values")) != NULL) {
		BS = new byte [len = ((PN->NP + 7) >> 3)];
		bzero (BS, len);
		str = (char*)sxml_txt (cur);
		if (sanitize_string (str) == 0)
			xevi ("<values> string", es, "-empty-");
		
		sts = str;
		for (pn = 0; pn < PN->NP; pn++) {
			// Find next digit in sts
			while (isspace (*sts))
				sts++;
			if (*sts == '\0')
				break;
			if (*sts == '1')
				PINS::sbit (BS, pn);
			else if (*sts != '0')
				xevi ("<values>", es, str);
		}
		print ("    VALUES: ");
		for (pn = 0; pn < PN->NP; pn++)
			print (PINS::gbit (BS, pn) ? "1" : "0");
		print ("\n");
		PN->IV = find_strpool ((const byte*)BS, len, NO);
		if (PN->IV != BS)
			// Recycled
			delete BS;
	}

	/* Default (initial) ADC input voltage */
	if (PN->NA != 0 && ((cur = sxml_child (data, "voltages")) != NULL ||
				(cur = sxml_child (data, "voltage")) != NULL)) {
		SS = new short [PN->NA];
		bzero (SS, PN->NA * sizeof (short));
		npp = new nparse_t [PN->NA];
		for (pn = 0; pn < PN->NA; pn++)
			npp [pn] . type = TYPE_double;
		str = (char*)sxml_txt (cur);
		len = parseNumbers (str, PN->NA, npp);
		if (len > PN->NA)
			excptn ("Root: too many FP values in <voltages> for %s",
				es);
		for (pn = 0; pn < len; pn++) {
			d = (npp [pn] . DVal * 32767.0) / 3.3;
			if (d < -32768.0)
				SS [pn] = 0x8000;
			else if (d > 32767.0)
				SS [pn] = 0x7fff;
			else
				SS [pn] = (short) d;
		}

		PN->VO = (const short*) find_strpool ((const byte*) SS,
			(int)(PN->NA) * sizeof (short), NO);

		if (PN->VO != SS)
			// Recycled
			delete SS;

		print ("    ADC INPUTS: ");
		for (pn = 0; pn < PN->NA; pn++)
			print (form ("%5.2f ", (double)(PN->VO [pn]) *
				3.3/32768.0));
		print ("\n");

		delete npp;
	}

	print ("\n");

	return PN;
}

data_le_t *BoardRoot::readLedsParams (sxml_t data, const char *esn) {
/*
 * Decodes LEDs parameters
 */
	sxml_t cur;
	nparse_t np [1];
	data_le_t *LE;
	const char *att;
	char *str;
	int len;
	
	char es [48];

	strcpy (es, "<leds> for ");
	strcat (es, esn);

	if ((data = sxml_child (data, "leds")) == NULL)
		return NULL;

	LE = new data_le_t;
	LE->LODev = NULL;
	LE->absent = NO;

	if ((att = sxml_attr (data, "number")) == NULL)
		xemi ("'total'", es);

	np [0].type = TYPE_LONG;
	
	if (parseNumbers (att, 1, np) != 1 || np [0].LVal < 0 ||
	    np [0].LVal > 64)
		xeai ("'total'", es, att);

	if (np [0].LVal == 0) {
		// Explicit NO
		print ("  LEDS: none\n");
		LE->absent = YES;
		return LE;
	}

	LE->NLeds = (word) (np [0].LVal);

	print (form ("  LEDS: %1d\n", LE->NLeds));

	// Output mode

	if ((cur = sxml_child (data, "output")) == NULL) {
		print ("    OUTPUT: none\n\n");
		LE->absent = YES;
		return LE;
	}

	if ((att = sxml_attr (cur, "target")) == NULL)
		xemi ("'target'", es);
	if (strcmp (att, "none") == 0) {
		// Equivalent to no leds
		LE->absent = YES;
		print ("    OUTPUT: none\n\n");
		return LE;
	} 

	print ("    OUTPUT: ");

	if (strcmp (att, "device") == 0) {
		str = (char*) sxml_txt (cur);
		len = sanitize_string (str);
		if (len == 0)
			xevi ("<output> device string", es, "-empty-");
		// This is a device name
		str [len] = '\0';
		LE->LODev = str;
		print (form ("device '%s'\n\n", str));
	} else if (strcmp (att, "socket") == 0) {
		print (form ("socket\n\n", str));
	} else
		xeai ("'target'", es, att);

	return LE;
}

void BoardRoot::initNodes (sxml_t data, int NT) {

	double d;
	data_no_t *DEF, *NOD;
	const char *def_type, *nod_type, *att;
	sxml_t chd, cno, cur;
	nparse_t np [2];
	Long i, j;
	int tq;
	data_rf_t *NRF, *DRF;

	d = (double) etuToItu (1.0);
	XmitRate = (RATE) round (d / SEther->BitRate);

	print ("Timing:\n");
	print (d,  "  ETU (1s) = ", 11, 14);
	print ((double) duToItu (1.0),
		   "  DU  (1m) = ", 11, 14);
	print (XmitRate,
		   "  RATE     = ", 11, 14);
	print (getTolerance (&tq),
	           "  TOL      = ", 11, 14);
	print (tq, "  TOL QUAL = ", 11, 14);
	print ("\n");

	TheStation = System;

	if ((data = sxml_child (data, "nodes")) == NULL)
		xenf ("<nodes>", "<network>");

	// Check for the defaults
	chd = sxml_child (data, "defaults");
	// OK if NULL, readNodeParams will initialize them
	DEF = readNodeParams (chd, -1);
	// DEF is never NULL, remember to deallocate it
	
	if (chd != NULL)
		// Check for type attribute
		def_type = sxml_attr (chd, "type");
	else
		def_type = NULL;

	chd = sxml_child (data, "node");
	if (chd == NULL)
		xenf ("<node>", "<nodes>");

	// Create all nodes
	for (i = 0; i < NT; i++) {
		// Try to locate the node's explicit definition. This
		// isn't terribly efficient, but should be OK for
		// initialization, unless the number of nodes is really
		// huge. Let's wait and see ...
		np [0] . type = TYPE_LONG;
		for (j = 0; ; j++) {
			cno = sxml_idx (chd, j);
			if (cno == NULL)
				xenf (form ("<node number=\"%1d\">", i),
					"<nodes");

			att = sxml_attr (cno, "number");
			if (att == NULL)
				xemi ("'number'", "<node>");

			if (parseNumbers (att, 1, np) != 1) 
				xeai ("'number'", "<node>", att);

			if ((Long) (np [0] . LVal) == i)
				// Found
				break;
		}

		NOD = readNodeParams (cno, i);

		if ((nod_type = sxml_attr (cno, "type")) == NULL)
			nod_type = def_type;

		// Substitute defaults as needed; validate later
		if (NOD->Mem == 0)
			NOD->Mem = DEF->Mem;

		// Neither of these is ever NULL
		NRF = NOD->rf;
		DRF = DEF->rf;

		if (NRF->XP == HUGE) {
			NRF->XP = DRF->XP;
			NRF->RP = DRF->RP;
		}

		if (NRF->BCMin == WNONE) {
			NRF->BCMin = DRF->BCMin;
			NRF->BCMax = DRF->BCMax;
		}

		if (NRF->Pre == WNONE)
			NRF->Pre = DRF->Pre;

		if (NRF->LBTDel == WNONE) {
			NRF->LBTDel = DRF->LBTDel;
			NRF->LBTThs = DRF->LBTThs;
		}

		// EEPROM
		if (NOD->ep == NULL) {
			// Inherit the defaults
			if (DEF->ep != NULL && !(DEF->ep->absent))
				NOD->ep = DEF->ep;
		} else if (NOD->ep->absent) {
			// Explicit "no", ignore the default
			delete NOD->ep;
			NOD->ep = NULL;
		}

		// UART
		if (NOD->ua == NULL) {
			// Inherit the defaults
			if (DEF->ua != NULL && !(DEF->ua->absent))
				NOD->ua = DEF->ua;
		} else if (NOD->ua->absent) {
			// Explicit "no", ignore the default
			delete NOD->ua;
			NOD->ua = NULL;
		}

		// PINS
		if (NOD->pn == NULL) {
			// Inherit the defaults
			if (DEF->pn != NULL && !(DEF->pn->absent))
				NOD->pn = DEF->pn;
		} else if (NOD->pn->absent) {
			// Explicit "no", ignore the default
			delete NOD->pn;
			NOD->pn = NULL;
		}

		// LEDs
		if (NOD->le == NULL) {
			// Inherit the defaults
			if (DEF->le != NULL && !(DEF->le->absent))
				NOD->le = DEF->le;
		} else if (NOD->le->absent) {
			// Explicit "no", ignore the default
			delete NOD->le;
			NOD->le = NULL;
		}

		// A few checks; some of this stuff is checked (additionally) at
		// the respective constructors

		if (NOD->Mem == 0)
			  excptn ("Root: no memory for node %1d", i);

		if (NRF->XP == HUGE)
			  excptn ("Root: power for node %1d is undefined", i);

		if (NRF->BCMin == WNONE) 
			  excptn ("Root: backoff for node %1d is undefined", i);

		if (NRF->Pre == WNONE)
			  excptn ("Root: preamble for node %1d is undefined",
				i);

		if (NRF->LBTDel == WNONE)
			  excptn ("Root: LBT parameters for node %1d are "
				"undefined", i);
				
		// Location
		if ((cur = sxml_child (cno, "location")) == NULL)
			  excptn ("Root: no location for node %1d", i);

		att = sxml_txt (cur);
		np [0].type = np [1].type = TYPE_double;
		if (parseNumbers (att, 2, np) != 2 ||
		    np [0].DVal < 0.0 || np [1].DVal < 0.0)
			excptn ("Root: illegal location (%s) for node %1d",
				att, i);
		NOD->X = np [0].DVal;
		NOD->Y = np [1].DVal;

		buildNode (nod_type, NOD);

		// Deallocate the data spec. Note that ua and pn may contain
		// arrays, but those arrays need not (must not) be deallocated.
		// They are either strings pointed to in the sxmtl tree, which
		// is deallocated elsewhere, or intentionally copied (constant)
		// strings that have been linked to by the respective objects.

		delete NOD->rf;		// This is always private and easy
		if (NOD->ep != NULL && NOD->ep != DEF->ep)
			delete NOD->ep;
		if (NOD->ua != NULL && NOD->ua != DEF->ua)
			delete NOD->ua;
		if (NOD->pn != NULL && NOD->pn != DEF->pn)
			delete NOD->pn;
		if (NOD->le != NULL && NOD->le != DEF->le)
			delete NOD->le;
		delete NOD;
	}

	// Delete the default data block
	delete DEF->rf;
	if (DEF->ep != NULL)
		delete DEF->ep;
	if (DEF->ua != NULL)
		delete DEF->ua;
	if (DEF->pn != NULL)
		delete DEF->pn;
	if (DEF->le != NULL)
		delete DEF->le;
	delete DEF;

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
		xemi ("'nodes'", "<network>");

	np [0] . type = TYPE_LONG;
	if (parseNumbers (att, 1, np) != 1)
		xeai ("'nodes'", "<network>", att);

	NN = (int) (np [0] . LVal);
	if (NN <= 0)
		excptn ("Root: 'nodes' in <network> must be strictly positive, "
			"is %1d", NN);

	initTiming (xml);
	initChannel (xml, NN);
	initNodes (xml, NN);

	sxml_free (xml);

	setResync (500, 0.5);

	create (System) AgentInterface;
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

#include "agent.cc"
#include "rfmodule_dm2200.cc"
#include "net.cc"
#include "plug_null.cc"
#include "plug_tarp.cc"
#include "tarp.cc"

#endif
