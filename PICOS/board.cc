#ifndef __picos_board_c__
#define __picos_board_c__

/* ==================================================================== */
/* Copyright (C) Olsonet Communications Corporation, 2008 - 2017.       */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "stdattr_undef.h"

#include "board.h"
#include "rwpmm.cc"
#include "wchansh.cc"
#include "wchansd.cc"
#include "wchannt.cc"
#include "encrypt.cc"
#include "nvram.cc"
#include "agent.h"

//
// Size of the number table for extracting table contents:
// make it divisible by 2 and 3
//
#define	NPTABLE_SIZE	(3*2*128)

//
// Size of the hash table for keeping track of process IDs
//
#define PPHASH_SIZE	4096

const char	__pi_hex_enc_table [] = {
				'0', '1', '2', '3', '4', '5', '6', '7',
				'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
			      };
int __pi_channel_type = -1;

// Legitimate UART rates ( / 100)
static const word urates [] = { 12, 24, 48, 96, 144, 192, 288, 384, 768, 1152,
                              2560 };

static Boolean force_port = NO;

struct strpool_s {

	const byte *STR;
	int Len;
	struct strpool_s *Next;
};

typedef	struct strpool_s strpool_t;

typedef struct {

	const char *Tag;
	IPointer Value;

} preitem_t;

struct preinit_s {

	Long NodeId;
	preitem_t *PITS;
	int NPITS;

	struct preinit_s *Next;
};

typedef struct preinit_s preinit_t;

typedef struct {

	double	x, y
#if ZZ_R3D
		    , z
#endif
		       ;
	Boolean Movable;

} location_t;

#define	NFTABLE_SIZE	32

static	preinit_t *PREINITS = NULL;

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
		while (l) {
			if (*s0++ != *s1++)
				break;
			l--;
		}
		if (l)
			continue;
		// Found
		// trace ("strpool, found: %x", p);
		return p->STR;
	}

	// Not found
	p = new strpool_t;
	if (cp) {
		// Copy the string
		p->STR = new byte [len];
		memcpy ((void*)(p->STR), str, len);
		// trace ("strpool, new, copy: %x", p);
	} else {
		p->STR = str;
		// trace ("strpool, new nocopy: %x", p);
	}
	p->Len = len;
	p->Next = STRPOOL;
	STRPOOL = p;

	return p->STR;
}

static int nstrcmp (const char *a, const char *b) {
//
// String compare that accounts for NULL
//
	if (a == NULL)
		return b != NULL;

	if (b == NULL)
		return -(a != NULL);

	return strcmp (a, b);
}

// ============================================================================

static rfm_const_t *RFMCONSTS = NULL;

// ============================================================================

static _PP_ *pptable [PPHASH_SIZE];
static sint lcpid = 0;

#define PPHMASK	(PPHASH_SIZE - 1)

void _PP_::_pp_hashin_ () {
//
// Inserts the process into the hash table
//
	_PP_ **ht = pptable + (ID & PPHMASK);	// The hash table entry

	if (*ht == NULL) {
		*ht = HNext = HPrev = this;
	} else {
		HNext = (*ht) -> HNext;
		HPrev = *ht;
		HNext->HPrev = HPrev->HNext = this;
	}
}

void _PP_::_pp_unhash_ () {
//
// Removes the process from hash table
//
	if (HPrev == this) {
		// A singleton
		pptable [ID & PPHMASK] = NULL;
	} else {
		HNext->HPrev = HPrev;
		pptable [ID & PPHMASK] = HPrev->HNext = HNext;
	}
	// This will be triggered upon termination of any "PicOS" process
	// with the intention of waking up those willing to "joinall"
	TheNode->TB.signal ((IPointer) getTypeId ());
}

static _PP_ *find_pcs_by_id (sint id) {
//
// Finds the process with the given id owned by this station
//
	_PP_ *tp, *tq;

	tp = pptable [id & PPHMASK];

	if (tp == NULL)
		return NULL;

	tq = tp;

	do {
		if (tp->ID == id && tp->getOwner () == TheStation)
			return tp;

		tp = tp->HNext;

	} while (tp != tq);

	return NULL;
}

sint _PP_::_pp_apid_ () {
//
// Allocate process ID
//
	// Initialize
	Flags = 0;
	WaitingUntil = TIME_0;

	while (1) {

		if (lcpid == MAX_SINT)
			// Make sure you don't exceed PicOS's range of sint
			lcpid = 1;
		else
			lcpid++;

		if (find_pcs_by_id (lcpid) == NULL) {
			// OK, allocate this one
			ID = lcpid;
			_pp_hashin_ ();
			return ID;
		}
	}
}
	
// ============================================================================

static const char *xname (int nn, const char *lab = NULL) {

	if (nn < 0) {
		if (nn < -1)
			return form ("<defaults--%s>", lab ? lab : "*");
		return "<defaults>";
	}
	return form ("node %1d", nn);
}

void _dad (PicOSNode, diag) (const char *s, ...) {

	va_list ap;

	va_start (ap, s);

	trace ("DIAG: %s", ::vform (s, ap));
}

void _dad (PicOSNode, emul) (sint n, const char *s, ...) {

	va_list ap;

	if (emulm != NULL && emulm->MS != NULL) {
		va_start (ap, s);
		emulm->MS->queue (n, s, ap);
	}
}

void __pi_dbg (word x, word v) {

	trace ("DBG: %1d == %04x / %1u", x, v, v);
}

void syserror (word p, const char *s) {
	excptn ("SYSERROR [%1d]: %1d, %s", TheStation->getId (), p, s);
}

void PicOSNode::stopall () {
//
// Cleanup all activities at the node, as for halt
//
	MemChunk *mc;
	
	cleanhlt ();
	terminate ();
	if (uart != NULL)
		uart_abort ();

	// Clean up memory
	while (MHead != NULL) {
		delete [] (byte*)(MHead->PTR);
		mc = MHead -> Next;
		delete MHead;
		MHead = mc;
	}

	if (RFInt != NULL)
		RFInt->abort ();

	if (lcdg != NULL)
		lcdg->m_lcdg_off ();

	Halted = YES;

	pwrt_zero ();
}

void _dad (PicOSNode, reset) () {
//
// This is the actual reset method callable by the praxis
//
	stopall ();
	reset ();
	Halted = NO;
	init ();
	Monitor->signal (&Halted);
	__pi_panel_signal (getId ());
	__mup_update (getId ());
	sleep;
}

void _dad (PicOSNode, halt) () {
//
// This halts the node
//
	stopall ();
	// Signal panel status change; note: no need to do that for reset
	// because the status change is momentary and not perceptible by
	// agents
	__pi_panel_signal (getId ());
	__mup_update (getId ());
	sleep;
}

void PicOSNode::uart_reset () {
//
// Reset the UART based on its interface mode
//
	uart_dir_int_t *f;

	switch (uart->IMode) {

		case UART_IMODE_D:

			UART_INTF_D (uart) -> init ();
			break;

		case UART_IMODE_N:
		case UART_IMODE_P:
		case UART_IMODE_L:
		case UART_IMODE_E:
		case UART_IMODE_F:

			UART_INTF_P (uart) -> init ();
			break;

		default:

			excptn ("PicOSNode->uart_reset: illegal mode %1d",
				uart->IMode);
	}

	uart->U->rst ();
}

void PicOSNode::uart_abort () {
//
// Stops the UART based on its interface mode (for halt)
//
	uart_dir_int_t *f;

	switch (uart->IMode) {

		case UART_IMODE_D:

			UART_INTF_D (uart) -> abort ();
			break;

		case UART_IMODE_N:
		case UART_IMODE_P:
		case UART_IMODE_L:
		case UART_IMODE_E:
		case UART_IMODE_F:

			UART_INTF_P (uart) -> abort ();
			break;

		default:

			excptn ("PicOSNode->uart_abort: illegal mode %1d",
				uart->IMode);
	}

	uart->U->abt ();
}

void PicOSNode::reset () {

	assert (Halted, "reset at %s: should be Halted", getSName ());
	assert (MHead == NULL, "reset at %s: MHead should be NULL",
		getSName ());

	NFree = MFree = MTotal;
	MTail = NULL;

	if (uart != NULL)
		uart_reset ();

	if (pins != NULL)
		pins->rst ();

	if (snsrs != NULL)
		snsrs->rst ();

	if (ledsm != NULL)
		ledsm->rst ();

	if (emulm != NULL)
		emulm->rst ();

	if (pwr_tracker != NULL)
		pwr_tracker->rst ();

	initParams ();
}

void PicOSNode::initParams () {

	// Process tally
	NPcss = 0;
	// Entropy source
	_da (entropy) = 0;

	// This is TIME_0
	SecondOffset = (lint) ituToEtu (Time);

	// Watchdog
	Watchdog = NULL;

	// Initialize the transceiver
	if (RFInt != NULL)
		RFInt->init ();

	// This will do the dynamic initialization of the static stuff in TCV
	_da (tcv_init) ();
}

// === rfm_intd_t =============================================================

rfm_intd_t::rfm_intd_t (const data_no_t *nd) {

	const data_rf_t *rf = nd->rf;
	rfm_const_t rfmc;

	// These two survive reset. We assume that they are never
	// changed by the praxis.
	rfmc.min_backoff = (word) (rf->BCMin);

	// This is turned into the argument for 'toss' to generate the
	// proper offset. The consistency has been verified by
	// readNodeParams.
	rfmc.max_backoff = (word) (rf->BCMax) - rfmc.min_backoff + 1;

	// Same about these two
	if (rf->LBTDel == 0) {
		// Disable it
		rfmc.lbt_threshold = NULL;
		rfmc.lbt_delay = 0;
		rfmc.lbt_tries = 0;
	} else {
		// This is already converted to linear
		rfmc.lbt_threshold = rf->LBTThs;
		rfmc.lbt_delay = rf->LBTDel;
		rfmc.lbt_tries = rf->LBTTries;
	}

	rfmc.DefXPower   = rf->Power;			// Index
	// This one has to be converted
	rfmc.DefRPower   = dBToLin (rf->Boost);		// Value in dB
	rfmc.DefRate     = rf->Rate;			// Index
	rfmc.DefChannel  = rf->Channel;

	// trace ("strpool cpars");
	cpars = (rfm_const_t*) find_strpool ((const byte*)&rfmc,
		sizeof (rfm_const_t), YES);

	RFInterface = create Transceiver (
				1,			// Dummy
				(Long)(rf->Pre),
				1.0,			// Dummy
				1.0,			// Dummy
				nd->X, nd->Y
#if ZZ_R3D
					    , nd->Z
#endif
						   );
	setrfrate (cpars->DefRate);
	setrfchan (cpars->DefChannel);

	Ether->connect (RFInterface);

}

void rfm_intd_t::init () {
//
// After reset
//

	RFInterface->rcvOn ();
	setrfpowr (LastPower = cpars->DefXPower);
	RFInterface->setRPower (cpars->DefRPower);

	setrfrate (cpars->DefRate);
	setrfchan (cpars->DefChannel);
	
	statid = 0;

#if (RADIO_OPTIONS & 0x04)
	memset (rerror, 0, sizeof (rerror));
#endif
	Receiving = Xmitting = NO;
	RXOFF = YES;

	OBuffer.fill (NONE, NONE, 0, 0, 0);
}

void rfm_intd_t::setrfpowr (word ix) {
//
// Set X power
//
	IVMapper *m;
	double v;

	if (__pi_channel_type == CTYPE_NEUTRINO) {
		v = 1.0;
	} else {
		m = Ether -> PS;
		assert (m->exact (ix),
			"RFInt->setrfpowr: illegal power index %1d", ix);
		v = m->setvalue (ix);
	}

	RFInterface -> setXPower (v);
}

void rfm_intd_t::setrfrate (word ix) {
//
// Set RF rate
//
	double rate;
	IVMapper *m;

	m = Ether -> Rates;

	assert (m->exact (ix), "RFInt->setrfrate: illegal rate index %1d", ix);

	rate = m->setvalue (ix);
	RFInterface->setTRate ((RATE) round ((double)etuToItu (1.0) / rate));
	RFInterface->setTag ((RFInterface->getTag () & 0xffff) | (ix << 16));
}

void rfm_intd_t::setrfchan (word ch) {
//
// Set RF channel
//
	MXChannels *m;

	m = Ether -> Channels;

	assert (ch <= m->max (), "RFInt->setrfchan: illegal channel %1d", ch);

	RFInterface->setTag ((RFInterface->getTag () & ~0xffff) | ch);
}

void rfm_intd_t::abort () {
//
// For reset
//
	// Abort the transceiver if transmitting
	if (RFInterface->transmitting ())
		RFInterface->abort ();

	RFInterface->rcvOff ();
}

// ============================================================================

void uart_tcv_int_t::abort () {

	// This is an indication that the driver isn't running; note that
	// memory allocated to r_buffer is returned by stopall (as it is
	// taken from the node's pool)
	r_buffer = NULL;
}
	
void PicOSNode::setup (data_no_t *nd) {

	// Turn this into a trigger mailbox
	TB.setLimit (-1);

	NFree = MFree = MTotal = (nd->Mem + 3) / 4; // This is in full words
	MHead = MTail = NULL;

	highlight = NULL;

	__host_id__ = nd->HID_present ? nd->HID : (lword) getId ();

	// Radio

	if (nd->rf == NULL) {
		// No radio
		RFInt = NULL;
	} else {

		// Impossible
		assert (Ether != NULL, "Root: RF interface without RF channel");

		RFInt = new rfm_intd_t (nd);
	}

	Movable = nd->Movable;

	if (nd->ua == NULL) {
		// No UART
		uart = NULL;
	} else {
		uart = new uart_t;
		uart->U = new UARTDV (nd->ua);
		uart->IMode = nd->ua->iface;
		switch (uart->IMode) {
			case UART_IMODE_N:
			case UART_IMODE_P:
			case UART_IMODE_L:
			case UART_IMODE_E:
			case UART_IMODE_F:
				uart->Int = (void*) new uart_tcv_int_t;
				break;
			default:
				uart->Int = (void*) new uart_dir_int_t;
		}
	}

	if (nd->pn == NULL) {
		// No PINS module
		pins = NULL;
	} else {
		pins = new PINS (nd->pn);
	}

	if (nd->sa == NULL) {
		// No sensors/actuators
		snsrs = NULL;
	} else {
		snsrs = new SNSRS (nd->sa);
	}

	if (nd->le == NULL) {
		ledsm = NULL;
	} else {
		ledsm = new LEDSM (nd->le);
	}

	if (nd->em == NULL) {
		emulm = NULL;
	} else {
		emulm = new EMULM (nd->em);
	}

	if (nd->pt == NULL) {
		pwr_tracker = NULL;
	} else {
		pwr_tracker = new pwr_tracker_t (nd->pt);
	}

	NPcLim = nd->PLimit;

	// EEPROM and IFLASH: note that they are not resettable
	eeprom = NULL;
	iflash = NULL;
	if (nd->ep != NULL) {
		data_ep_t *EP = nd->ep;
		if (EP->EEPRS) {
			eeprom = new NVRAM (EP->EEPRS, EP->EEPPS, EP->EFLGS,
				EP->EECL, EP->bounds, EP->EPINI, EP->EPIF);
		}
		if (EP->IFLSS) {
			iflash = new NVRAM (EP->IFLSS, EP->IFLPS, 
				NVRAM_TYPE_NOOVER | NVRAM_TYPE_ERPAGE,
					EP->IFCL, NULL, EP->IFINI, EP->IFIF);
		}
	}

	if (nd->Lcdg)
		lcdg = new LCDG;
	else
		lcdg = NULL;

	initParams ();

	// This can be optional based on whether the node is supposed to be
	// initially on or off 

	if (nd->On == 0) {
		// This value means OFF (it can be WNONE - for default - or 1)
		Halted = YES;
		// Dummy "abort" for the UART to set the input absorber process
		if (uart)
			uart->U->abt ();
		// Make sure the node uses zero power
		pwrt_zero ();
		return;
	}

	Halted = NO;
	init ();
}

address PicOSNode::memAlloc (int size, word lsize) {
/*
 * size  == real size 
 * lsize == simulated size
 */
	MemChunk 	*mc;
	address		*op;

	lsize = (lsize + 3) / 4;		// Convert to 4-tuples
	if (lsize > MFree) {
		// trace ("MEMALLOC OOM: %1u [%1d, %1u]", MFree, size, lsize);
		return NULL;
	}

	mc = new MemChunk;
	mc -> Next = NULL;
	mc -> PTR = (address) new byte [size];
	mc -> Size = lsize;

	MFree -= lsize;
	if (NFree > MFree)
		NFree = MFree;

	if (MHead == NULL)
		MHead = mc;
	else
		MTail->Next = mc;
	MTail = mc;
	// trace ("MEMALLOC %x %1u [%1u, %1d]", mc->PTR, MFree, lsize, size);

	return mc->PTR;
}

void PicOSNode::memFree (address p) {

	MemChunk *mc, *pc;

	if (p == NULL) {
	// trace ("MEMFREE: NULL");
		return;
	}

	for (pc = NULL, mc = MHead; mc != NULL; pc = mc, mc = mc -> Next) {

		if (p == mc->PTR) {
			// Found
			if (pc == NULL)
				MHead = mc->Next;
			else
				pc->Next = mc->Next;

			if (mc->Next == NULL)
				MTail = pc;

			delete [] (byte*) (mc->PTR);
			MFree += mc -> Size;
			// trace ("MEMFREE: %x %1u [%1u]", p, MFree, mc->Size);
			assert (MFree <= MTotal,
				"PicOSNode->memFree: corrupted memory");
			// trace ("MEMFREE OK: %x %1u", p, mc->Size);
			delete mc;
			TB.signal (N_MEMEVENT);
			return;
		}

	}

	excptn ("PicOSNode->memFree: chunk %x not found", p);
}

word _dad (PicOSNode, memfree) (int pool, word *res) {

	if (res != NULL)
		*res = NFree << 1;

	// This is supposed to be in awords, if I remember correctly. What
	// a mess!!!

#if SIZE_OF_AWORD == 2
	return MFree << 1;
#else
	return MFree;
#endif
}

word _dad (PicOSNode, maxfree) (int pool, word *res) {
/*
 * This is a stub
 */
	if (res != NULL)
		*res = 1;

#if SIZE_OF_AWORD == 2
	return MFree << 1;
#else
	return MFree;
#endif
}

word _dad (PicOSNode, actsize) (address p) {

	MemChunk *mc;

	for (mc = MHead; mc != NULL; mc = mc->Next)
		if (p == mc->PTR)
			// Found
			return (word)(mc->Size * 4);

	excptn ("PicOSNode->actsize: incorrect chunk pointer");
	return 0;
}

Boolean PicOSNode::memBook (word lsize) {

	lsize = (lsize + 3) / 4;
	if (lsize > MFree) {
		return NO;
	}
	MFree -= lsize;
	if (NFree > MFree)
		NFree = MFree;
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

void PicOSNode::no_sensor_module (const char *fn) {

	excptn ("%s: no SENSORS module at %s", fn, getSName ());
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

static word vfparse (char *res, word n, const char *fm, va_list ap) {

	char c;
	word d;

#define	outc(c)	do { if (res && (d < n)) res [d] = (char)(c); d++; } while (0)

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

	d = 0;

	while (1) {

		c = *fm++;

		if (c == '\\') {
			/* Escape the next character unless it is 0 */
			if ((c = *fm++) == '\0') {
				outc ('\\');
				goto Eol;
			}
			outc (c);
			continue;
		}

		if (c == '%') {
			/* Something special */
			c = *fm++;
			switch (c) {
			    case 'x' : {
				word val; int i;
				val = va_arg (ap, int);
				for (i = 12; ; i -= 4) {
					outc (__pi_hex_enc_table
						[(val>>i)&0xf]);
					if (i == 0)
						break;
				}
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
				c = *fm;
				if (c == 'd' || c == 'u') {
					lword val, i;
					fm++;
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
					fm++;
					val = va_arg (ap, lword);
					for (i = 28; ; i -= 4) {
						outc (__pi_hex_enc_table
							[(val>>i)&0xf]);
						if (i == 0)
							break;
					}
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
				char *st;
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
				if (c == '\0')
					goto Ret;
			}
		} else {
			// Regular character
Eol:
			outc (c);
			if (c == '\0')
Ret:
				return d;
		}
	}
}

char* _dad (PicOSNode, vform) (char *res, const char *fm, va_list aq) {

	word fml, d;

	if (res != NULL) {
		// We trust the caller
		vfparse (res, MAX_WORD, fm, aq);
		return res;
	}

	// Size unknown; guess a decent size
	fml = strlen (fm) + 17;
	// Sentinel included (it is counted by outc)
Again:
	if ((res = (char*) umalloc (fml)) == NULL)
		/* There is not much we can do */
		return NULL;

	if ((d = vfparse (res, fml, fm, aq)) > fml) {
		// No luck, reallocate
		ufree (res);
		fml = d;
		goto Again;
	}
	return res;
}

word _dad (PicOSNode, vfsize) (const char *fm, va_list aq) {

	return vfparse (NULL, 0, fm, aq);
}

word _dad (PicOSNode, fsize) (const char *fm, ...) {

	va_list	ap;
	va_start (ap, fm);

	return _da (vfsize) (fm, ap);
}

int _dad (PicOSNode, vscan) (const char *buf, const char *fmt, va_list ap) {

	int nc;

#define	scani(at)	{ at *vap; Boolean mf; \
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
			vap = va_arg (ap, at *); \
			*vap = 0; \
			while (isdigit (*buf)) { \
				*vap = (*vap) * 10 - \
				     (at)(word)(*buf - '0'); \
				buf++; \
			} \
			if (!mf) \
				*vap = (at)(-((at)(*vap))); \
			}
#define scanu(at)	{ at *vap; \
			while (!isdigit (*buf)) \
				if (*buf++ == '\0') \
					return nc; \
			nc++; \
			vap = va_arg (ap, at *); \
			*vap = 0; \
			while (isdigit (*buf)) { \
				*vap = (*vap) * 10 + \
				     (at)(word)(*buf - '0'); \
				buf++; \
			} \
			}
#define	scanx(at)	{ at *vap; int dc; char c; \
			while (!isxdigit (*buf)) \
				if (*buf++ == '\0') \
					return nc; \
			nc++; \
			vap = va_arg (ap, at *); \
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

	if (buf == NULL || fmt == NULL)
		return 0;

	nc = 0;
	while (*fmt != '\0') {
		if (*fmt++ != '%')
			continue;
		switch (*fmt++) {
		    case '\0': return nc;
		    case 'd': scani (word); break;
		    case 'u': scanu (word); break;
		    case 'x': scanx (word); break;
#if	CODE_LONG_INTS
		    case 'l':
			switch (*fmt++) {
			    case '\0':	return nc;
		    	    case 'd': scani (lword); break;
		    	    case 'u': scanu (lword); break;
		    	    case 'x': scanx (lword); break;
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
	uart_dir_int_t *f;

	assert (uart != NULL, "PicOSNode->ser_in: no UART present at the node");

	assert (uart->IMode == UART_IMODE_D, "PicOSNode->ser_in: not allowed "
		"in mode %1d", uart->IMode);

	if (len == 0)
		return 0;

	f = UART_INTF_D (uart);

	if (f->__inpline == NULL) {
		if (f->pcsInserial == NULL) {
			if (tally_in_pcs ()) {
				(create d_uart_inp_p) -> _pp_apid_ ();
			} else {
				npwait (st);
				sleep;
			}
		}
		f->pcsInserial->wait (DEATH, st);
		sleep;
	}

	/* Input available */
	if (*(f->__inpline) == 0) // bin cmd
		prcs = f->__inpline [1] + 3; // 0x00, len, 0x04
	else
		prcs = strlen (f->__inpline);
	if (prcs >= len)
		prcs = len-1;

	memcpy (buf, f->__inpline, prcs);

	ufree (f->__inpline);
	f->__inpline = NULL;

	if (*buf) // if it's NULL, it's a bin cmd
		buf [prcs] = '\0';

	return 0;
}

int _dad (PicOSNode, ser_out) (word st, const char *m) {

	int prcs;
	char *buf;
	uart_dir_int_t *f;

	assert (uart != NULL,
		"PicOSNode->ser_out: no UART present at the node");

	assert (uart->IMode == UART_IMODE_D, "PicOSNode->ser_out: not allowed "
		"in mode %1d", uart->IMode);

	f = UART_INTF_D (uart);

	if (f->pcsOutserial != NULL) {
		f->pcsOutserial->wait (DEATH, st);
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

	if (tally_in_pcs ()) {
		(create d_uart_out_p (buf)) -> _pp_apid_ ();
	} else {
		ufree (buf);
		npwait (st);
		sleep;
	}

	return 0;
}

int _dad (PicOSNode, ser_outb) (word st, const char *m) {

	int prcs;
	char *buf;
	uart_dir_int_t *f;

	assert (uart != NULL,
		"PicOSNode->ser_outb: no UART present at the node");

	assert (uart->IMode == UART_IMODE_D, "PicOSNode->ser_outb: not allowed "
		"in mode %1d", uart->IMode);

	assert (st != WNONE, "PicOSNode->ser_outb: NONE state unimplemented");

	f = UART_INTF_D (uart);

	if (m == NULL)
		return 0;

	if (f->pcsOutserial != NULL) {
		f->pcsOutserial->wait (DEATH, st);
		sleep;
	}
	if (tally_in_pcs ()) {
		(create d_uart_out_p (m)) -> _pp_apid_ ();
	} else {
		ufree (buf);
		npwait (st);
		sleep;
	}

	return 0;
}

int _dad (PicOSNode, ser_inf) (word st, const char *fmt, ...) {
/* ========= */
/* Formatted */
/* ========= */

	int prcs;
	va_list	ap;
	uart_dir_int_t *f;

	assert (uart != NULL,
		"PicOSNode->ser_inf: no UART present at the node");

	assert (uart->IMode == UART_IMODE_D, "PicOSNode->ser_inf: not allowed "
		"in mode %1d", uart->IMode);

	if (fmt == NULL)
		return 0;

	f = UART_INTF_D (uart);

	if (f->__inpline == NULL) {
		if (f->pcsInserial == NULL) {
			if (tally_in_pcs ()) {
				(create d_uart_inp_p) -> _pp_apid_ ();
			} else {
				npwait (st);
				sleep;
			}
		}
		f->pcsInserial->wait (DEATH, st);
		sleep;
	}

	/* Input available */
	va_start (ap, fmt);

	prcs = _da (vscan) (f->__inpline, fmt, ap);

	ufree (f->__inpline);
	f->__inpline = NULL;

	return 0;
}

int _dad (PicOSNode, ser_outf) (word st, const char *m, ...) {

	int prcs;
	char *buf;
	va_list ap;
	uart_dir_int_t *f;

	assert (uart != NULL,
		"PicOSNode->ser_outf: no UART present at the node");

	assert (uart->IMode == UART_IMODE_D, "PicOSNode->ser_outf: not allowed "
		"in mode %1d", uart->IMode);

	assert (st != WNONE, "PicOSNode->ser_outf: NONE state unimplemented");

	if (m == NULL)
		return 0;

	f = UART_INTF_D (uart);

	if (f->pcsOutserial != NULL) {
		f->pcsOutserial->wait (DEATH, st);
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

	if (tally_in_pcs ()) {
		(create d_uart_out_p (buf)) -> _pp_apid_ ();
	} else {
		ufree (buf);
		npwait (st);
		sleep;
	}

	return 0;
}

word _dad (PicOSNode, ee_open) () {

	sysassert (eeprom != NULL, "ee_open no eeprom");
	return eeprom->nvopen ();
};

void _dad (PicOSNode, ee_close) () {

	sysassert (eeprom != NULL, "ee_close no eeprom");
	eeprom->nvclose ();
};

lword _dad (PicOSNode, ee_size) (Boolean *er, lword *rt) {

	sysassert (eeprom != NULL, "ee_size no eeprom");
	return eeprom->size (er, rt);
};

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

static void xeni (const char *s) {
	excptn ("Root: %s table too large, increase NPTABLE_SIZE", s);
}

static void xesi (const char *s, const char *w) {
	excptn ("Root: a single integer number required in %s in %s", s, w);
}

static void xefi (const char *s, const char *w) {
	excptn ("Root: a single FP number required in %s in %s", s, w);
}

static void xmon (int nr, const word *wt, const double *dta, const char *s) {
//
// Validates the monotonicity of data to be put into an IVMapper
//
	int j;
	Boolean dec;

	if (nr < 2)
		return;

	for (j = 1; j < nr; j++) {
		if (wt [j] <= wt [j-1])
			excptn ("Root: representation entries in mapper %s "
				"are not strictly increasing", s);
	}

	dec = dta [1] < dta [0];
	for (j = 1; j < nr; j++) {
		if (( dec && dta [j] >= dta [j-1]) ||
	            (!dec && dta [j] <= dta [j-1])  )
			excptn ("Root: value entries in mapper %s are not "
				"strictly monotonic", s);
	}
}

static void xpos (int nr, const double *dta, const char *s, Boolean nz = NO) {

	int i;

	for (i = 1; i < nr; i++) {
		if (dta [i] <= 0.0) {
			if (nz == NO && dta [i] == 0.0)
				continue;
			excptn ("Root: illegal value in mapper %s", s);
		}
	}
}

static void oadj (const char *s, int n) {
//
// Tabulate output to the specified position
//
	n -= strlen (s);
	while (n > 0) {
		Ouf << ' ';
		n--;
	}
}

static void sptypes (nparse_t *np, int tp) {

	int i;

	for (i = 0; i < NPTABLE_SIZE; i++)
		np [i].type = tp;
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

	// DU is equal to the propagation time (in ITU) across 1m
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

static void packetCleaner (Packet *p) {

	// Assumes there are no other packet types
	delete [] ((PKT*)p)->Payload;

}

int BoardRoot::initChannel (sxml_t data, int NN, Boolean nc) {

	const char *sfname, *att, *xnam;
	double bn_db, beta, dref, sigm, loss_db, psir, pber, cutoff;
	nparse_t np [NPTABLE_SIZE];
	int K, nb, nr, i, j, syncbits, bpb, frml;
	sxml_t prp, cur;
	sir_to_ber_t	*STB;
	IVMapper	*ivc [6];
	MXChannels	*mxc;
	word wn, *wt;
	Boolean rmo, symm;
	double *dta, *dtb, *dtc;

	Ether = NULL;

	if (NN == 0)
		// Ignore the channel, no stations are equipped with radio
		return NN;

	sptypes (np, TYPE_double);

	// Preset this to NULL
	memset (ivc, 0, sizeof (ivc));

	if ((data = sxml_child (data, "channel")) == NULL) {
		if (nc)
			// We are allowed to force NN to zero
			return 0;
		// This is an error
		excptn ("Root: no <channel> element with %1d wireless nodes",
			NN);
		// No continuation
	}

	// Determine the channel type
	if ((prp = sxml_child (data, "propagation")) == NULL)
		xenf ("<propagation>", "<channel>");

	if ((att = sxml_attr (prp, "type")) == NULL)
		xemi ("type", "<propagation>");

	if (strncmp (att, "sh", 2) == 0)
		__pi_channel_type = CTYPE_SHADOWING;
	else if (strncmp (att, "sa", 2) == 0)
		__pi_channel_type = CTYPE_SAMPLED;
	else if (strncmp (att, "ne", 2) == 0)
		__pi_channel_type = CTYPE_NEUTRINO;
	else
		xeai ("type", "<propagation>", att);

	if (__pi_channel_type != CTYPE_NEUTRINO) {

		// Some "common" attributes are ignored for neutrino type
		// channel

		if (__pi_channel_type == CTYPE_SAMPLED)
			// We need this one in advance, can be NULL
			sfname = sxml_attr (prp, "samples");

		bn_db = -HUGE;
		// The default value for background noise translating into
		// lin 0.0
		if ((att = sxml_attr (data, "bn")) != NULL) {
			if (parseNumbers (att, 1, np) != 1)
				xeai ("bn", "<channel>", att);
			bn_db = np [0].DVal;
		}

		sigm = -1.0;

		if ((att = sxml_attr (prp, "sigma")) != NULL) {
			// Expect a double
			if (parseNumbers (att, 1, np) != 1 ||
				(sigm = np [0].DVal) < 0.0)
					xeai ("sigma", "<propagation>", att);
		}

		// The BER table
		if ((cur = sxml_child (data, "ber")) == NULL)
			xenf ("<ber>", "<channel>");

		att = sxml_txt (cur);
		nb = parseNumbers (att, NPTABLE_SIZE, np);
		if (nb > NPTABLE_SIZE)
			excptn ("Root: <ber> table too large, "
				"increase NPTABLE_SIZE");

		if (nb < 4 || (nb & 1) != 0) {

			if (nb < 0)
				excptn ("Root: illegal numerical value in "
					"<ber> table");
			else
				excptn ("Root: illegal size of <ber> table "
					"(%1d), must be an even number >= 4",
					nb);
		}

		psir = HUGE;
		pber = -1.0;
		// This is the size of BER table
		nb /= 2;
		STB = new sir_to_ber_t [nb];

		for (i = 0; i < nb; i++) {
			// The SIR is stored as a logarithmic ratio
			STB [i].sir = np [2 * i] . DVal;
			STB [i].ber = np [2 * i + 1] . DVal;
			// Validate
			if (STB [i] . sir >= psir)
				excptn ("Root: SIR entries in <ber> must be "
					"monotonically decreasing, %f and %f "
					"aren't", psir, STB [i] . sir);
			psir = STB [i] . sir;
			if (STB [i] . ber < 0)
				excptn ("Root: BER entries in <ber> must not "
					"be negative, %g is", STB [i] . ber);
			if (STB [i] . ber <= pber)
				excptn ("Root: BER entries in <ber> must be "
					"monotonically increasing, %g and %g "
					"aren't", pber, STB [i] . ber);
			pber = STB [i] . ber;
		}

		// The cutoff threshold wrt to background noise: the default
		// means no cutoff
		cutoff = -HUGE;
		if ((cur = sxml_child (data, "cutoff")) != NULL) {
			att = sxml_txt (cur);
			if (parseNumbers (att, 1, np) != 1)
				xevi ("<cutoff>", "<channel>", att);
			cutoff = np [0].DVal;
		}

		// Power
		if ((cur = sxml_child (data, "power")) == NULL)
			xenf ("<power>", "<network>");

		att = sxml_txt (cur);

		// Check for a single double value first
		if (parseNumbers (att, 2, np) == 1) {
			// We have a single entry case
			wt = new word [1];
			dta = new double [1];
			wt [0] = 0;
			dta [0] = np [0] . DVal;
			nr = 1;
		} else {
			for (i = 0; i < NPTABLE_SIZE; i += 2)
				np [i ]  . type = TYPE_int;
			nr = parseNumbers (att, NPTABLE_SIZE, np);
			if (nr > NPTABLE_SIZE)
				xeni ("<power>");

			if (nr < 2 || (nr % 2) != 0) 
				excptn ("Root: number of items in <power> must "
					"be either 1, or a nonzero multiple of "
					"2");
			nr /= 2;
			wt = new word [nr];
			dta = new double [nr];

			for (j = 0; j < nr; j++) {
				wt [j] = (word) (np [2*j] . IVal);
				dta [j] = np [2*j + 1] . DVal;
			}
			xmon (nr, wt, dta, "<power>");
			for (i = 0; i < NPTABLE_SIZE; i += 2)
				np [i ]  . type = TYPE_double;
			sptypes (np, TYPE_double);
		}

		ivc [XVMAP_PS] = new IVMapper (nr, wt, dta, YES);

		// RSSI map (optional for shadowing)
		if ((cur = sxml_child (data, "rssi")) != NULL) {

			att = sxml_txt (cur);

			for (i = 0; i < NPTABLE_SIZE; i += 2)
				np [i ]  . type = TYPE_int;

			nr = parseNumbers (att, NPTABLE_SIZE, np);
			if (nr > NPTABLE_SIZE)
				xeni ("<rssi>");

			if (nr < 2 || (nr % 2) != 0) 
				excptn ("Root: number of items in <rssi> must "
						"be a nonzero multiple of 2");
			nr /= 2;
			wt = new word [nr];
			dta = new double [nr];

			for (j = 0; j < nr; j++) {
				wt [j] = (word) (np [2*j] . IVal);
				dta [j] = np [2*j + 1] . DVal;
			}
			xmon (nr, wt, dta, "<rssi>");

			ivc [XVMAP_RSSI] = new IVMapper (nr, wt, dta, YES);

			sptypes (np, TYPE_double);

		} else if (__pi_channel_type == CTYPE_SAMPLED &&
							sfname != NULL) {

			excptn ("Root: \"sampled\" propagation model with "
				"non-empty sample file requires RSSI table");
		}
	}

	if (__pi_channel_type == CTYPE_SHADOWING) {

		if (sigm < 0.0)
			// Use the default sigma of zero if missing
			sigm = 0.0;
		att = sxml_txt (prp);
		if ((nb = parseNumbers (att, 4, np)) != 4) {
			if (nb < 0)
				excptn ("Root: illegal number in "
					"<propagation>");
			else
				excptn ("Root: expected 4 numbers in "
					"<propagation>, found %1d", nb);
		}

		if (np [0].DVal != -10.0)
			excptn ("Root: the factor in propagation formula "
				"must be -10, is %f", np [0].DVal);
		beta = np [1].DVal;
		dref = np [2].DVal;
		loss_db = np [3].DVal;

	} else if (__pi_channel_type == CTYPE_SAMPLED) {

		K = 2;					// Sigma threshold
		if (((att = sxml_attr (prp, "sthreshold")) != NULL) ||
		    ((att = sxml_attr (prp, "threshold")) != NULL) ) {
			np [0].type = TYPE_int;
			nr = parseNumbers (att, 1, np);
			if (nr == 0)
				xeai ("sthreshold", "<propagation>", att);
			K = (int) (np [0].LVal);
			if (K < 0 || K == 1)
				excptn ("Root: the sigma threshold must be"
					"> 1 or 0, is %1d", K);
			np [0].type = TYPE_double;
		}

		symm = NO;
		if ((att = sxml_attr (prp, "symmetric")) != NULL &&
			*att == 'y')
				symm = YES;

		att = sxml_txt (prp);

		// Expect the attenuation table

		nr = parseNumbers (att, NPTABLE_SIZE, np);
		if (nr > NPTABLE_SIZE)
			xeni ("<propagation>");

		if (sigm < 0.0) {
			// Just in case, make sure this is always sane
			sigm = 0.0;
			// We expect triplets: distance, attenuation, sigma
			if (nr < 6 || (nr % 3) != 0)
				excptn ("Root: the number of entries in "
					"<propagation> table must be divisible "
					"by 3 and at least equal 6");
			nr /= 3;
			dta = new double [nr];
			dtb = new double [nr];
			dtc = new double [nr];

			for (i = 0; i < nr; i++) {

				dta [i] = np [3 * i    ] . DVal;
				dtb [i] = np [3 * i + 1] . DVal;
				dtc [i] = np [3 * i + 2] . DVal;

				if (dta [i] < 0.0) {
PTErr1:
					excptn ("Root: negative distance in the"
						" <propagation> table, "
						"%g [%1d]",
						dta [i], i);
				}
				if (dtc [i] < 0.0)
					excptn ("Root: negative sigma in the"
						" <propagation> table, "
						"%g [%1d]",
						dtc [i], i);
				// Note: we do not insist on the sigmas being
				// monotonic. As we never lookup distances
				// based on sigma, it should be OK.
				if (i > 0) {
				    if (dta [i] <= dta [i - 1]) {
PTErr2:
					excptn ("Root: distances in the "
						"<propagation> table are not "
						"strictly increasing, "
						"%g [%1d] <= %g [%1d]",
							dta [i], i,
							dta [i - 1], i - 1);
				    }
				    if (dtb [i] >= dtb [i - 1]) {
PTErr3:
					excptn ("Root: levels in the "
						"<propagation> table are not "
						"strictly decreasing, "
						"%g [%1d] >= %g [%1d]",
							dtb [i], i,
							dtb [i - 1], i - 1);
				    }
			        }
			}

			ivc [XVMAP_SIGMA] =
				(IVMapper*) new DVMapper (nr, dta, dtc, NO);

		} else {
			// Dublets (sigma is fixed)
			if (nr < 4 || (nr % 2) != 0)
				excptn ("Root: <propagation> table needs an "
				    "even number of entries and at least 4");
			nr /= 2;

			dta = new double [nr];
			dtb = new double [nr];

			for (i = 0; i < nr; i++) {

				dta [i] = np [2 * i    ] . DVal;
				dtb [i] = np [2 * i + 1] . DVal;

				if (dta [i] < 0.0)
					goto PTErr1;

				if (i > 0) {
			    		if (dta [i] <= dta [i - 1])
						goto PTErr2;
			    		if (dtb [i] >= dtb [i - 1])
						goto PTErr3;
				}
			}

		}

		ivc [XVMAP_ATT] = (IVMapper*) new DVMapper (nr, dta, dtb, YES);

	} else {

		// Neutrino channel
		if ((att = sxml_attr (prp, "range")) != NULL) {
			nr = parseNumbers (att, 1, np);
			if (nr == 0)
				xeai ("range", "<propagation>", att);
			dref = np [0].DVal;
			if (dref <= 0.0)
				excptn ("Root: the propagation range must be > "
					"0.0, is %g", dref);
		} else {
			dref = HUGE;
		}
	}

	// Frame parameters
	if ((cur = sxml_child (data, "frame")) == NULL)
		xenf ("<frame>", "<channel>");
	att = sxml_txt (cur);
	np [0] . type = np [1] . type = np [2] . type = TYPE_LONG;
	if ((nr = parseNumbers (att, 3, np)) != 3) {
		if (nr != 2) {
			xevi ("<frame>", "<channel>", att);
		} else {
			syncbits = 0;
			bpb      = (int) (np [0].LVal);
			frml     = (int) (np [1].LVal);
		}
	} else {
		syncbits = (int) (np [0].LVal);
		bpb      = (int) (np [1].LVal);
		frml     = (int) (np [2].LVal);
	}

	if (syncbits < 0 || bpb <= 0 || frml < 0)
		xevi ("<frame>", "<channel>", att);

	// Prepare np for reading value mappers
	for (i = 0; i < NPTABLE_SIZE; i += 2) {
		np [i  ] . type = TYPE_int;
		np [i+1] . type = TYPE_double;
	}

	if ((cur = sxml_child (data, "rates")) == NULL)
		xenf ("<rates>", "<network>");

	// This tells us whether we should expect boost factors
	rmo = !nstrcmp (sxml_attr (cur, "boost"), "yes");
	att = sxml_txt (cur);

	if (rmo) {
		// Expect sets of triplets: int int double
		for (i = 0; i < NPTABLE_SIZE; i += 3) {
			np [i  ] . type = np [i+1] . type = TYPE_int;
			np [i+2] . type = TYPE_double;
		}
		nr = parseNumbers (att, NPTABLE_SIZE, np);
		if (nr > NPTABLE_SIZE)
			xeni ("<rates>");

		if ((nr < 3) || (nr % 3) != 0)
			excptn ("Root: number of items in <rates> must be a"
				" nonzero multiple of 3");
		nr /= 3;
		wt = new word [nr];
		dta = new double [nr];
		dtb = new double [nr];

		for (j = 0; j < nr; j++) {
			wt [j] = (word) (np [3*j] . IVal);
			// Actual rates go first (stored as double)
			dta [j] = (double) (np [3*j + 1] . IVal);
			// Boost
			dtb [j] = np [3*j + 2] . DVal;
		}

		xmon (nr, wt, dta, "<rates>");
		xmon (nr, wt, dtb, "<rates>");

		ivc [XVMAP_RATES] = new IVMapper (nr, wt, dta);
		ivc [XVMAP_RBOOST] = new IVMapper (nr, wt, dtb, YES);

	} else {

		// No boost specified, the boost IVMapper is null, which
		// translates into the boost of 1.0

		for (i = 0; i < NPTABLE_SIZE; i++)
			np [i] . type = TYPE_int;

		nr = parseNumbers (att, NPTABLE_SIZE, np);
		if (nr > NPTABLE_SIZE)
			xeni ("<rates>");

		if (nr < 2) {
			if (nr < 1) {
RVErr:
				excptn ("Root: number of items in <rates> must "
					"be either 1, or a nonzero multiple of "
						"2");
			}
			// Single entry - a special case
			wt = new word [1];
			dta = new double [1];
			wt [0] = 0;
			dta [0] = (double) (np [0] . IVal);
		} else {
			if ((nr % 2) != 0)
				goto RVErr;

			nr /= 2;
			wt = new word [nr];
			dta = new double [nr];

			for (j = 0; j < nr; j++) {
				wt [j] = (word) (np [2*j] . IVal);
				dta [j] = (double) (np [2*j + 1] . IVal);
			}
			xmon (nr, wt, dta, "<rates>");
		}

		ivc [XVMAP_RATES] = new IVMapper (nr, wt, dta);
	}

	// Channels

	if ((cur = sxml_child (data, "channels")) == NULL) {
		mxc = new MXChannels (1, 0, NULL);
	} else {
		if ((att = sxml_attr (cur, "number")) != NULL ||
		    (att = sxml_attr (cur, "n")) != NULL ||
		    (att = sxml_attr (cur, "count")) != NULL) {
			// Extract the number of channels
			np [0] . type = TYPE_LONG;
			if (parseNumbers (att, 1, np) != 1)
				xevi ("<channels>", "<channel>", att);
			if (np [0] . LVal < 1 || np [0] . LVal > MAX_WORD)
				xevi ("<channels>", "<channel>", att);
			wn = (word) (np [0] . LVal);
			if (wn == 0)
				xeai ("number", "<channels>", att);

			att = sxml_txt (cur);

			sptypes (np, TYPE_double);

			j = parseNumbers (att, NPTABLE_SIZE, np);

			if (j > NPTABLE_SIZE)
				excptn ("Root: <channels> separation table too"
					" large, increase NPTABLE_SIZE");

			if (j < 0)
				excptn ("Root: illegal numerical value in the "
					"<channels> separation table");

			if (j == 0) {
				// No separations
				dta = NULL;
			} else {
				dta = new double [j];
				for (i = 0; i < j; i++)
					dta [i] = np [i] . DVal;
			}

			mxc = new MXChannels (wn, j, dta);
		} else
			xemi ("number", "<channels>");
	}

	print ("\n");

	// Create the channel

	if (__pi_channel_type == CTYPE_SHADOWING)
		create RFShadow (NN, STB, nb, dref, loss_db, beta, sigm, bn_db,
			bn_db, cutoff, syncbits, bpb, frml, ivc, mxc, NULL);
	else if (__pi_channel_type == CTYPE_SAMPLED)
		create RFSampled (NN, STB, nb, K, sigm, bn_db, bn_db,
			cutoff, syncbits, bpb, frml, ivc, sfname, symm,
				mxc, NULL);
	else if (__pi_channel_type == CTYPE_NEUTRINO)
		create RFNeutrino (NN, dref, bpb, frml, ivc, mxc);

	// Packet cleaner
	Ether->setPacketCleaner (packetCleaner);

	return NN;
}

static FLAGS get_io_desc (sxml_t data, const char *es, const char **ID,
	const char **OD) {
//
// Parse the standard part of the I/O specification of a module
//
	sxml_t cur;
	char *str, ostr [6];
	const char *att;
	int i, len;
	FLAGS Mode;

	Mode = 0;
	if (ID != NULL)
		*ID = NULL;
	if (OD != NULL)
		*OD = NULL;

	if ((cur = sxml_child (data, "input")) != NULL) {
		str = (char*) sxml_txt (cur);
		if ((att = sxml_attr (cur, "source")) == NULL)
			xemi ("source", es);
		if (strcmp (att, "none") == 0)
			// No input spec
			goto NoInput;

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
			Mode |= XTRN_IMODE_DEVICE;
			if (ID == NULL) {
InpIll:
				excptn ("Root: illegal <input> specification "
					"in %s ", es);
			}
			*ID = str;
			print (form ("device '%s'", str));
		} else if (strcmp (att, "socket") == 0) {
			// Both modes
			Mode |= XTRN_IMODE_SOCKET | XTRN_OMODE_SOCKET;
			print ("socket");
		} else if (strcmp (att, "string") == 0) {
			len = sanitize_string (str);
			if (len == 0)
				excptn ("Root: empty <input> string in %s", es);
			if (len & ~XTRN_IMODE_STRLEN)
				excptn ("Root: <input> string too long in %s",
					es);
			// We copy the string, as it has to survive the
			// initialization
			// trace ("strpool iodesc");
			str = (char*) find_strpool ((const byte*) str, len + 1,
				YES);
			if (ID == NULL)
				goto InpIll;
			*ID = str;
			Mode |= (XTRN_IMODE_STRING | len);
			for (i = 0; i < 5 && i < len; i++)
				ostr [i] = str [i];
			ostr [i] = '\0';
			print (form ("string '%s ...'", ostr));
		} else {
			xeai ("source", es, att);
		}

		print ("\n");
	}
NoInput:
	if ((cur = sxml_child (data, "output")) != NULL) {
		str = (char*) sxml_txt (cur);
		if ((att = sxml_attr (cur, "target")) == NULL)
			xemi ("target", es);
		if ((Mode & XTRN_OMODE_MASK) == XTRN_OMODE_SOCKET) {
			// This must be a socket
			if (strcmp (att, "socket"))
				// but isn't
				excptn ("Root: 'target' in <output> in %s must"
					"be 'socket', but is %s",
						es, att);
			print ("    OUTPUT: ");
			print ("socket (forced by INPUT)\n");
			return Mode;
		} else if (strcmp (att, "none") == 0) {
			// Equivalent to 'no output spec'
			return Mode;
		}

		print ("    OUTPUT: ");

		if (strcmp (att, "device") == 0) {
			len = sanitize_string (str);
			if (len == 0)
				xevi ("<output> device string", es, "-empty-");
			// This is a device name
			str [len] = '\0';
			Mode |= XTRN_OMODE_DEVICE;
			if (OD == NULL) {
				excptn ("Root: illegal <output> specification "
					"in %s ", es);
			}
			*OD = str;
			print (form ("device '%s'", str));
		} else if (strcmp (att, "socket") == 0) {
			if ((Mode & XTRN_IMODE_MASK) != XTRN_IMODE_NONE)
				excptn ("Root: 'target' in <output> in %s (%s)"
					" conflicts with <input> spec",
						es, att);
			Mode |= (XTRN_OMODE_SOCKET | XTRN_IMODE_SOCKET);
			print ("socket");
		} else {
			xeai ("target", es, att);
		}
		print ("\n");
	}
	return Mode;
}

void BoardRoot::initPanels (sxml_t data) {

	char es [32];
	char *str;
	int CNT, len;
	Dev *d;
	Boolean lf;
	FLAGS mode;

	TheStation = System;

	for (lf = YES, CNT = 0, data = sxml_child (data, "panel");
					data != NULL; data = sxml_next (data)) {

		if (lf) {
			print ("\n");
			lf = NO;
		}

		sprintf (es, "Panel %1d", CNT);

		print (es); print (":\n");

		mode = get_io_desc (data, es, (const char**)(&str), NULL);

		len = (mode & XTRN_IMODE_STRLEN);
		mode &= XTRN_IMODE_MASK;

		if (mode == XTRN_IMODE_DEVICE) {

			d = create Dev;

			if (d->connect (DEVICE+READ, str, 0, XTRN_MBX_BUFLEN) ==
			    ERROR)
				excptn ("Root: panel %1d, cannot open device "
					"%s", str);
			create PanelHandler (d, XTRN_IMODE_DEVICE);
			continue;
		}

		if (mode == XTRN_IMODE_STRING) {

			create PanelHandler ((Dev*)str, XTRN_IMODE_STRING|len);
			continue;
		}

		if (mode != XTRN_IMODE_SOCKET)
			excptn ("Root: <input> spec missing in %s", es);
	}
	print ("\n");
}

void BoardRoot::initRoamers (sxml_t data) {

	char es [32];
	char *str;
	int CNT, len;
	Dev *d;
	Boolean lf;
	FLAGS mode;

	TheStation = System;

	for (lf = YES, CNT = 0, data = sxml_child (data, "roamer");
					data != NULL; data = sxml_next (data)) {

		if (lf) {
			print ("\n");
			lf = NO;
		}

		sprintf (es, "Roamer %1d", CNT);

		print (es); print (":\n");

		mode = get_io_desc (data, es, (const char**)(&str), NULL);

		len = (mode & XTRN_IMODE_STRLEN);
		mode &= XTRN_IMODE_MASK;

		if (mode == XTRN_IMODE_DEVICE) {

			d = create Dev;

			if (d->connect (DEVICE+READ, str, 0, XTRN_MBX_BUFLEN) ==
			    ERROR)
				excptn ("Root: roamer %1d, cannot open device "
					"%s", str);
			create MoveHandler (d, XTRN_IMODE_DEVICE);
			continue;
		}

		if (mode == XTRN_IMODE_STRING) {

			create MoveHandler ((Dev*)str, XTRN_IMODE_STRING | len);
			continue;
		}

		if (mode != XTRN_IMODE_SOCKET)
			excptn ("Root: <input> spec missing in %s", es);
	}

	print ("\n");
}

void BoardRoot::initAgent (sxml_t data) {
//
// Extract the list of background images for roamers. The files will be sent to
// the agent when a roamer session is established.
//
	sxml_t t;
	const char *att;
	int cnt, len;

	if ((data = sxml_child (data, "display")) == NULL)
		return;

	for (__pi_N_BGR_Images = 0, t = sxml_child (data, "roamer"); t != NULL;
		t = sxml_next (t))
			__pi_N_BGR_Images++;

	if (__pi_N_BGR_Images == 0)
		return;

	__pi_BGR_Image = new char* [__pi_N_BGR_Images];

	for (cnt = 0, t = sxml_child (data, "roamer"); cnt < __pi_N_BGR_Images;
	     t = sxml_next (t), cnt++) {
		if ((att = sxml_attr (t, "image")) != NULL &&
		    (len = strlen (att)) != 0) {
			__pi_BGR_Image [cnt] = new char [len + 1];
			strcpy (__pi_BGR_Image [cnt], att);
		} else
			__pi_BGR_Image [cnt] = NULL;
	}
}

void BoardRoot::readPreinits (sxml_t data, int nn) {

	const char *att;
	sxml_t chd, che;
	preinit_t *P;
	int i, j, d, tp;
	nparse_t np [1];

	chd = sxml_child (data, "preinit");

	if (chd == NULL)
		// No preinits for this node
		return;

	P = new preinit_t;

	P->NodeId = nn;

	// Calculate the number of preinits at this level
	for (P->NPITS = 0, che = chd; che != NULL; che = sxml_next (che))
		P->NPITS++;

	P->PITS = new preitem_t [P->NPITS];

	print ("  Preinits:\n");

	for (i = 0; chd != NULL; chd = sxml_next (chd), i++) {

		if ((att = sxml_attr (chd, "tag")) == NULL)
			excptn ("Root: <preinit> for %s, tag missing",
				xname (nn));

		if ((d = strlen (att)) == 0)
			excptn ("Root: <preinit> for %s, empty tag",
				xname (nn));

		// Check for uniqueness
		for (j = 0; j < i; j++)
			if (strcmp (att, P->PITS [j].Tag) == 0)
				excptn ("Root: <preinit> for %s, duplicate tag "
					"%s", xname (nn), att);

		// Allocate storage for the tag
		// trace ("strpool preinit");
		P->PITS [i] . Tag = (char*) find_strpool ((const byte*) att,
			d + 1, YES);

		print (form ("    Tag: %s, Value: ", P->PITS [i] . Tag));

		if ((att = sxml_attr (chd, "type")) == NULL || *att == 'w')
			// Expect a short number (decimal or hex)
			tp = 0;
		else if (*att == 'l')
			// Long size
			tp = 1;
		else if (*att == 's')
			// String
			tp = 2;
		else
			excptn ("Root: <preinit> for %s, illegal type %s",
				xname (nn), att);

		att = sxml_txt (chd);

		if (tp < 2) {
			// A single number, hex or int (signed or unsigned)
			print (att);
			while (isspace (*att))
				att++;
			if (*att == '0' && (*(att+1) == 'x' ||
							    *(att+1) == 'X')) {
				// Hex
				np [0] . type = TYPE_hex;
				if (parseNumbers (att, 1, np) != 1)
					excptn ("Root: <preinit> for %s, "
						"illegal value for tag %s",
							xname (nn),
							P->PITS [i] . Tag);
				P->PITS [i] . Value = (IPointer) np [0]. IVal;

			} else {

				np [0] . type = TYPE_LONG;
				if (parseNumbers (att, 1, np) != 1)
					excptn ("Root: <preinit> for %s, "
						"illegal value for tag %s",
							xname (nn),
							P->PITS [i] . Tag);

				P->PITS [i] . Value = (IPointer) np [0]. LVal;
			}
		} else {
			// Collect the string
			d = sanitize_string ((char*) att);
			print (att);
			if (d == 0) {
				P->PITS [i] . Value = 0;
			} else {
				// trace ("strpool PITS");
				P->PITS [i] . Value = (IPointer)
					find_strpool ((const byte*) att, d + 1,
						YES);
			}
		}
		print ("\n");
	}

	// Add the preinit to the list. We create them in the increasing order
	// of node numbers, with <default> going first. This is ASSUMED here.
	// The list will start with the largest numbered node and end with the
	// <default> entry.

	print ("\n");

	P->Next = PREINITS;

	PREINITS = P;
}

IPointer PicOSNode::preinit (const char *tag) {

	preinit_t *P;
	int i;

	P = PREINITS;

	while (P != NULL) {
		if (P->NodeId < 0 || P->NodeId == getId ()) {
			// Search for the tag
			for (i = 0; i < P->NPITS; i++)
				if (strcmp (P->PITS [i] . Tag, tag) == 0)
					return P->PITS [i] . Value;
		}
		P = P -> Next;
	}

	return 0;
}

static data_epini_t *get_nv_inits (sxml_t nvs, const char *w, const char *esn) {
//
// Extract the list of initializers for NVRAM
//
	char es [64];
	data_epini_t *hd, *cc, *ta;
	sxml_t cur;
	const char *att;
	nparse_t np [1];
	int len;
	const char *sp, *ep;

	strcpy (es, "<chunk> for ");
	strcat (es, w);
	strcat (es, " at ");
	strcat (es, esn);


	hd = NULL;
	for (cur = sxml_child (nvs, "chunk"); cur != NULL;
							cur = sxml_next (cur)) {
		if ((att = sxml_attr (cur, "address")) == NULL)
			att = sxml_attr (cur, "at");

		if (att == NULL)
			xemi ("address", es);

		np [0].type = TYPE_LONG;
		if (parseNumbers (att, 1, np) != 1)
			xeai ("address", es, att);

		ta = new data_epini_t;

		ta->Address = (lword) (np [0].LVal);
		ta->chunk = NULL;
		ta->Size = 0;
		ta->Next = NULL;

		if ((att = sxml_attr (cur, "file")) != NULL) {
			// The file name
			len = strlen (att);
			// No need to do this via a string pool, as the
			// name will be deallocated after initialization
			ta->chunk = (byte*) (new char [len + 1]);
			strcpy ((char*)(ta->chunk), att);
		} else {
			// Count the bytes
			att = sxml_txt (cur);
			for (len = 0, sp = att; *sp != '\0'; len++) {
				while (isspace (*sp)) sp++;
				for (ep = sp; isxdigit (*ep) || *ep == 'x' ||
					*ep == 'X'; ep++);
				if (ep == sp)
					// Something weird
					break;
				sp = ep;
				while (isspace (*sp)) sp++;
				if (*sp == ',')
					sp++;
			}

			if (*sp != '\0')
				excptn ("Root: a %s contains "
					"an illegal character '%c'",
						es, *sp);
			if (len == 0)
				excptn ("Root: a %s is empty", es);
			// Allocate that many bytes
			ta->chunk = new byte [ta->Size = len];

			// And go through the second round of actually decoding
			// the stuff
			len = 0;
			while (len < ta->Size) {
				ta->chunk [len] = (byte) strtol (att,
					(char**)&ep, 16);
				if (ep == att) {
					excptn ("Root: a %s contains garbage: "
						"%16s", es, att);
				}
				while (isspace (*ep)) ep++;
				if (*ep == ',')
					ep++;
				att = ep;
				len++;
			}
		}

		// Append at the end of current list
		if (hd == NULL)
			hd = cc = ta;
		else {
			cc->Next = ta;
			cc = ta;
		}
	}

	return hd;
}

static void print_nv_inits (const data_epini_t *in) {
//
// Print info regarding EEPROM/IFLASH initializers
//
	while (in != NULL) {
		print (in->Size ?
			form ("              Init: @%1d, chunk size %1d\n",
				in->Address, in->Size)
			:
			form ("              Init: @%1d, file %s\n",
				in->Address, (char*)(in->chunk))
			);
		in = in->Next;
	}
}

static void deallocate_ep_def (data_ep_t *ep) {
//
// Deallocate EEPROM definition
//
	data_epini_t *c, *q;

	if ((ep->EFLGS & NVRAM_NOFR_EPINIT) == 0) {
		// Deallocate arrays
		c = ep->EPINI;
		while (c != NULL) {
			c = (q = c)->Next;
			delete q->chunk;
			delete q;
		}
		if (ep->EPIF)
			delete ep->EPIF;
	}

	if ((ep->EFLGS & NVRAM_NOFR_IFINIT) == 0) {
		c = ep->IFINI;
		while (c != NULL) {
			c = (q = c)->Next;
			delete q->chunk;
			delete q;
		}
		if (ep->IFIF)
			delete ep->IFIF;
	}
	
	delete ep;
}

static Boolean parseLocation (sxml_t cur, int tp, Long nn,
						    double *x, double *y
#if ZZ_R3D
						             , double *z
#define	__ndim 3
#else
#define	__ndim 2
#endif
									) {
//
// tp == 0	=> defaults
// tp == 1	=> within <node>
// tp == 2	=> within <location>
//
	const char *att;
	nparse_t np [__ndim];
	int i;
	Boolean res, bad;

	res = YES + YES;	// undefined

	if ((att = sxml_attr (cur, "movable")) != NULL) {
		// yes or no
		if (strcmp (att, "no") == 0)
			res = NO;
		else if (strcmp (att, "yes") == 0)
			res = YES;
		else
				xeai ("movable", "location", att);
	}

	if (tp == 0) {
		// That's it (defaults)
		print (form ("  Location: %c\n\n",
			 res == NO ? 'F' : 'M'));
		return res;
	}

	// Not defaults
	att = sxml_txt (cur);

	for (i = 0; i < __ndim; i++)
		np [i].type = TYPE_double;

	bad = (parseNumbers (att, __ndim, np) != __ndim);

	if (!bad) {
		for (i = 0; i < __ndim; i++)
			if (np [i].DVal < 0.0) {
				bad = YES;
				break;
			}
	}

	if (bad) {
		if (tp == 1)
			excptn ("Root: illegal location (%s) for <node> %1d",
				att, nn);
		else
			excptn ("Root: illegal location (%s) for <location> "
				"(node %1d)", att, nn);
	}

	*x = np [0].DVal;
	*y = np [1].DVal;
#if ZZ_R3D
	*z = np [2].DVal;
#endif
	if (tp == 1)
		print (form ("  Location: <%1.2f,%1.2f"
#if ZZ_R3D
					",%1.2f"
#endif
					"> %c\n\n",
				*x, *y, 
#if ZZ_R3D
				    *z,
#endif
				 res == NO  ? 'F' :
				(res == YES ? 'M' : 'D')));
	else
		print (form ("  Node %1d: <%1.2f,%1.2f"
#if ZZ_R3D
					",%1.2f"
#endif
					"> %c\n", nn,
				*x, *y, 
#if ZZ_R3D
				    *z,
#endif
				 res == NO  ? 'F' :
				(res == YES ? 'M' : 'D')));
	return res;
#undef __ndim
}
		 
data_no_t *BoardRoot::readNodeParams (sxml_t data, int nn, const char *lab,
							   const char *ion) {

#if RF_N_THRESHOLDS > EP_N_BOUNDS
#define	__np_size (RF_N_THRESHOLDS + 2)
#else
#define	__np_size (EP_N_BOUNDS + 2)
#endif
	nparse_t np [__np_size];
#undef	__np_size

	double rfths [RF_N_THRESHOLDS], *rft;

	sxml_t cur, mai;
	const char *att;
	char *str, *as;
	int i, len, nts;
	data_rf_t *RF;
	data_ep_t *EP;
	data_no_t *ND;
	Boolean ppf;

	ND = new data_no_t;
	ND->Mem = 0;
	// These ones are not set here, placeholders only to be set by
	// the caller
	ND->X = ND->Y =
#if ZZ_R3D
		ND->Z =
#endif
			-1.0;
	// The default for Movability is YES, but for a node (as opposed to
	// "default") make it "undefined" for now
	ND->Movable = (nn < 0) ? YES : YES + YES;
	ND->HID_present = NO;

	if (ion == NULL)
		ND->On = WNONE;
	else if (strcmp (ion, "on") == 0)
		ND->On = 1;
	else if (strcmp (ion, "off") == 0)
		ND->On = 0;
	else
		xeai ("start", xname (nn, lab), ion);

	ND->PLimit = WNONE;

	// This is just a flag for now
	ND->Lcdg = WNONE;

	// The optionals
	ND->rf = NULL;
	ND->ua = NULL;
	ND->ep = NULL;
	ND->pn = NULL;
	ND->sa = NULL;
	ND->le = NULL;
	ND->em = NULL;
	ND->pt = NULL;

	if (data == NULL)
		// This is how we stand so far
		return ND;

	print ("Node configuration [");

	if (nn < 0) {
		print ("default");
		if (nn < 1)
			print (form ("--%s", lab ? lab : "*"));
	} else {
		print (form ("    %3d", nn));
	}
	print ("]:\n\n");

/* === */
/* HID */
/* === */
	if (nn >= 0 && (att = sxml_attr (data, "hid")) != NULL) {
		// Host id
		np [0] . type = TYPE_LONG;
		if (parseNumbers (att, 1, np) != 1)
			xeai ("hid", xname (nn, lab), att);
		ND->HID_present = YES;
		ND->HID = (lword) (np [0] . LVal);
		print (form ("  HID:        %08X\n", ND->HID));
	}

/* ======== */
/* Preinits */
/* ======== */

	readPreinits (data, nn);

	ppf = NO;

/* ====== */
/* MEMORY */
/* ====== */

	if ((cur = sxml_child (data, "memory")) != NULL) {
		np [0].type = np [1].type = TYPE_LONG;
		if (parseNumbers (sxml_txt (cur), 1, np) != 1)
			xevi ("<memory>", xname (nn, lab), sxml_txt (cur));
		ND->Mem = (lword) (np [0] . LVal);
		if (ND->Mem > 0x40000000)
			// Some sane limit
                        excptn ("Root: <memory> too large (%1d) in %s; the "
                                "maximum is %1u", ND->Mem, xname (nn, lab),
					0x40000000);

		print (form ("  Memory:     %1u bytes\n", ND->Mem));
		ppf = YES;
	}

	np [0] . type = np [1] . type = TYPE_LONG;

	/* PLIMIT */
	if ((cur = sxml_child (data, "processes")) != NULL) {
		if (parseNumbers (sxml_txt (cur), 1, np) != 1)
			xesi ("<power>", xname (nn, lab));
		ND->PLimit = (word) (np [0] . LVal);
		print (form ("  Processes:  %1d\n", ND->PLimit));
		ppf = YES;
	}

	/* LCDG */
	if ((cur = sxml_child (data, "lcdg")) != NULL) {
		att = sxml_attr (cur, "type");
		if (att == NULL)
			ND->Lcdg = 0;
		else if (strcmp (att, "n6100p") == 0)
			ND->Lcdg = 1;
		else
			excptn ("Root: 'type' for <lcdg> (%s) in %s can only "
				"be 'n6100p' at present", att, xname (nn, lab));
		if (ND->Lcdg)
			print (form ("  LCDG display: %s\n", att));
	}

/* ========= */
/* RF MODULE */
/* ========= */

	// ====================================================================

	if ((mai = sxml_child (data, "radio")) != NULL) {

		// The node has a radio interface

		assert (Ether != NULL, "Root: <radio> (in %s) illegal if there"
			" is no RF <channel>", xname (nn, lab));

		RF = ND->rf = new data_rf_t;
		RF->Boost = HUGE;
		RF->Power = RF->Rate = RF->Channel = RF->Pre = RF->LBTDel =
			RF->BCMin = RF->BCMax = WNONE;
		// LBTDel == WNONE implies that LBTThs is irrelevant
		RF->LBTThs = NULL;

		RF->absent = YES;

		/* POWER */
		if ((cur = sxml_child (mai, "power")) != NULL) {
			// This is the index
			if (parseNumbers (sxml_txt (cur), 1, np) != 1)
				xesi ("<power>", xname (nn, lab));
			RF->Power = (word) (np [0] . LVal);
			if (__pi_channel_type != CTYPE_NEUTRINO &&
			    !(Ether->PS->exact (RF->Power)))
				excptn ("Root: power index %1d (in %s) does not"
					" occur in <channel><power>",
						RF->Power, xname (nn, lab));
			print (form ("  Power idx:  %1d\n", RF->Power));
			ppf = YES;
			RF->absent = NO;
		}
	
		/* RATE */
		if ((cur = sxml_child (mai, "rate")) != NULL) {
			if (parseNumbers (sxml_txt (cur), 1, np) != 1)
				xesi ("<rate>", xname (nn, lab));
			RF->Rate = (word) (np [0].LVal);
			// Check if the rate index is legit
			if (!(Ether->Rates->exact (RF->Rate))) 
				excptn ("Root: rate index %1d (in %s) does not"
					" occur in <channel><rates>",
						RF->Rate, xname (nn, lab));
			print (form ("  Rate idx:   %1d\n", RF->Rate));
			RF->absent = NO;
		}
	
		/* CHANNEL */
		if ((cur = sxml_child (mai, "channel")) != NULL) {
			if (parseNumbers (sxml_txt (cur), 1, np) != 1)
				xesi ("<channel>", xname (nn, lab));
			RF->Channel = (word) (np [0].LVal);
			// Check if the channel number is legit
			if (Ether->Channels->max () < RF->Channel) 
				excptn ("Root: channel number %1d (in %s) is "
					"illegal (see <channel><channels>)",
						RF->Channel, xname (nn, lab));
			print (form ("  Channel:    %1d\n", RF->Channel));
			RF->absent = NO;
		}
	
		/* BACKOFF */
		if ((cur = sxml_child (mai, "backoff")) != NULL) {
			// Both are int
			if (parseNumbers (sxml_txt (cur), 2, np) != 2)
				excptn ("Root: two int numbers required in "
					"<backoff> in %s", xname (nn, lab));
			RF->BCMin = (word) (np [0].LVal);
			RF->BCMax = (word) (np [1].LVal);
	
			if (RF->BCMax < RF->BCMin)
				xevi ("<backoff>", xname (nn, lab),
					sxml_txt (cur));
	
			print (form ("  Backoff:    min=%1d, max=%1d\n",
				RF->BCMin, RF->BCMax));
			ppf = YES;
			RF->absent = NO;
		}
	
		/* PREAMBLE */
		if ((cur = sxml_child (mai, "preamble")) != NULL) {
			// Both are int
			if (parseNumbers (sxml_txt (cur), 1, np) != 1)
				xevi ("<preamble>", xname (nn, lab),
					sxml_txt (cur));
			RF->Pre = (word) (np [0].LVal);
			print (form ("  Preamble:   %1d bits\n", RF->Pre));
			ppf = YES;
			RF->absent = NO;
		}
	
		// ============================================================
	
		/* LBT */
		if ((cur = sxml_child (mai, "lbt")) != NULL) {
			// del, thresholds, tries
			for (i = 1; i < RF_N_THRESHOLDS + 1; i++)
				np [i] . type = TYPE_double;
			len = parseNumbers (sxml_txt (cur),
				RF_N_THRESHOLDS + 2,
					np);

			if (len == 0) {
				// Disabled
				nts = 0;
				RF->LBTTries = 0;
				RF->LBTDel = 0;
			} else {
				// Read the delay which goes first
				RF->LBTDel = (word) (np [0].LVal);
				if (RF->LBTDel == 0) {
					// Disabled as well
					nts = 0;
					RF->LBTTries = 0;
				} else {
					if (len == 2) {
					  	// This is for downward
						// compatibility: if there are
						// just two numbers, the second
						// is the threshold and the
					  	// number of tries (by default)
						// is 5
						nts = 1;
						rfths [0] = np [1].DVal;
						RF->LBTTries = 5;
					} else {
						if (len < 2)
						  xevi ("<lbt>",
						    xname (nn, lab),
						      sxml_txt (cur));
						// At least three values: all
						// but the last one are
						// thresholds
						for (nts = 0; nts < len-2;
						  nts++)
						  rfths [nts] = np [nts+1].DVal;
					  	RF->LBTTries =
						  (word) (np [len-1].DVal);
					}
				}
			}

			if (RF->LBTTries > RF_N_THRESHOLDS || RF->LBTTries < 0)
				excptn ("Root: illegal retry count (%1d) in "
					"<lbt> for %s, should be > 0 and <= "
					"%1d", RF->LBTTries,
						xname (nn, lab),
							RF_N_THRESHOLDS);
			if (nts > RF->LBTTries)
				excptn ("Root: retry count (%1d) in <lbt> for "
					"%s is smaller than number of "
					"thresholds (%1d)",
						RF->LBTTries,
							xname (nn, lab),
								nts);

			// Replicate the missing thresholds from the last
			for (i = nts; nts < RF->LBTTries; nts++)
				rfths [nts] = rfths [i-1];

			if (nts == 0) {
				RF->LBTThs = NULL;
				print ("  LBT:        disabled\n");
			} else {
				print (form ("  LBT:        del=%1d, ths=<",
					RF->LBTDel));
				for (i = 0; i < nts-1; i++) {
					print (form ("%gdBm,", rfths [i]));
					rfths [i] = dBToLin (rfths [i]);
				}
				print (form ("%gdBm>\n", rfths [i]));
				rfths [i] = dBToLin (rfths [i]);
				// trace ("strpool LBTTHS");
				RF->LBTThs =
				    (double*) find_strpool ((const byte*) rfths,
					sizeof (double) * nts, YES);
			}
			
			ppf = YES;
			RF->absent = NO;
		}
	
		// ============================================================
	
		np [0] . type = TYPE_double;
	
		if ((cur = sxml_child (mai, "boost")) != NULL) {
			if (parseNumbers (sxml_txt (cur), 1, np) != 1)
				xefi ("<boost>", xname (nn, lab));
			RF->Boost = np [0] . DVal;
			print (form ("  Boost:      %gdB\n", RF->Boost));
			ppf = YES;
			RF->absent = NO;
		}
	
		// ============================================================

	}

	if (ppf)
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

		EP = ND->ep = new data_ep_t;
		memset (EP, 0, sizeof (data_ep_t));

		// Flag: FIM still inheritable from defaults
		EP->IFLSS = WNONE;

		EP->EEPPS = 0;
		att = sxml_attr (cur, "size");

		if (att == NULL)
			len = 0;
		else if ((len = parseNumbers (att, 2, np)) < 0)
			excptn ("Root: illegal value in <eeprom> size for %s",
				xname (nn, lab));

		// No size or size=0 means no EEPROM
		EP->EEPRS = (len == 0) ? 0 : (lword) (np [0] . LVal);

		// Number of pages
		if (EP->EEPRS) {
			// EEPRS == 0 means no EEPROM, no inheritance from
			// defaults

			if ((att = sxml_attr (cur, "erase")) != NULL) {
				if (strcmp (att, "block") == 0 ||
				    strcmp (att, "page") == 0)
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

			if (len > 1) {
				pgsz = (lword) (np [1] . LVal);
				if (pgsz) {
					// This is the number of pages, so turn
					// it into a page size
					if (pgsz > EP->EEPRS ||
					    (EP->EEPRS % pgsz) != 0)
						excptn ("Root: number of eeprom"
 						    " pages, %1d, is illegal "
						    "in %s", pgsz,
						    xname (nn, lab));
					pgsz = EP->EEPRS / pgsz;
				}
				EP->EEPPS = pgsz;
			} 

			if ((att = sxml_attr (cur, "clean")) != NULL) {
				if (parseNumbers (att, 1, np) != 1)
					xeai ("clean", "eeprom", att);
				EP->EECL = (byte) (np [0]. LVal);
			} else 
				// The default
				EP->EECL = 0xff;

			// Timing bounds
			for (i = 0; i < EP_N_BOUNDS; i++)
				EP->bounds [i] = 0.0;

			if ((att = sxml_attr (cur, "timing")) != NULL) {
				if ((len = parseNumbers (att, EP_N_BOUNDS,
					np + 2)) < 0)
						excptn ("Root: illegal timing "
							"value in <eeprom> for "
							"%s", xname (nn, lab));

				for (i = 0; i < len; i++) {
					EP->bounds [i] = np [i + 2] . DVal;
				}
			} 
		
			for (i = 0; i < EP_N_BOUNDS; i += 2) {

				if (EP->bounds [i] != 0.0 &&
				    EP->bounds [i+1] == 0.0)
					EP->bounds [i+1] = EP->bounds [i];

				if (EP->bounds [i] < 0.0 ||
				    EP->bounds [i+1] < EP->bounds [i] )
					excptn ("Root: timing distribution "
						"parameters for eeprom: %1g %1g"
						" are illegal in %s",
							EP->bounds [i],
							EP->bounds [i+1],
							xname (nn, lab));
			}

			if ((att = sxml_attr (cur, "image")) != NULL) {
				// Image file
				EP->EPIF = new char [strlen (att) + 1];
				strcpy (EP->EPIF, att);
			}

			EP->EPINI = get_nv_inits (cur, "EEPROM",
				xname (nn, lab));

		   	print (form (
		"  EEPROM:     %1d bytes, page size: %1d, clean: %02x\n",
					EP->EEPRS, EP->EEPPS, EP->EECL));
			if (EP->EPIF)
				print (form (
				"              Image file: %s\n", EP->EPIF));
			print (form (
				"              R: [%1g,%1g], W: [%1g,%1g], "
					     " E: [%1g,%1g], S: [%1g,%1g]\n",
					EP->bounds [0],
					EP->bounds [1],
					EP->bounds [2],
					EP->bounds [3],
					EP->bounds [4],
					EP->bounds [5],
					EP->bounds [6],
					EP->bounds [7]));

			print_nv_inits (EP->EPINI);

		} else
			      print ("  EEPROM:     none\n");
	}

	if ((cur = sxml_child (data, "iflash")) != NULL) {

		Long ifsz, ifps;

		if (EP == NULL) {
			EP = ND->ep = new data_ep_t;
			memset (EP, 0, sizeof (data_ep_t));
			// Flag: EEPROM still inheritable from defaults
			EP->EEPRS = LWNONE;
		}

		// Get the size (as for EEPROM)
		ifsz = 0;
		att = sxml_attr (cur, "size");
		len = parseNumbers (att, 2, np);
		if (len != 1 && len != 2)
			xevi ("<iflash>", xname (nn, lab), sxml_txt (cur));
		ifsz = (Long) (np [0].LVal);
		if (ifsz < 0 || ifsz > 65536)
			excptn ("Root: iflash size must be >= 0 and <= 65536, "
				"is %1d, in %s", ifsz, xname (nn, lab));
		ifps = ifsz;
		if (len == 2) {
			ifps = (Long) (np [1].LVal);
			if (ifps) {
			    if (ifps < 0 || ifps > ifsz || (ifsz % ifps) != 0)
				excptn ("Root: number of iflash pages, %1d, is "
					"illegal in %s", ifps, xname (nn, lab));
			    ifps = ifsz / ifps;
			}
		}
		EP->IFLSS = (word) ifsz;
		EP->IFLPS = (word) ifps;

		if (ifsz) {
			// There is an IFLASH
			EP->IFINI = get_nv_inits (cur, "IFLASH",
				xname (nn, lab));

			if ((att = sxml_attr (cur, "clean")) != NULL) {
				if (parseNumbers (att, 1, np) != 1)
					xeai ("clean", "iflash", att);
				EP->IFCL = (byte) (np [0] . LVal);
			} else 
				// The default
				EP->IFCL = 0xff;

			if ((att = sxml_attr (cur, "image")) != NULL) {
				// Image file
				EP->IFIF = new char [strlen (att) + 1];
				strcpy (EP->IFIF, att);
			}

		    	print (form (
		"  IFLASH:     %1d bytes, page size: %1d, clean: %02x\n",
					ifsz, ifps, EP->IFCL));
			if (EP->IFIF)
				print (form (
				"              Image file: %s\n", EP->IFIF));
			print_nv_inits (EP->IFINI);
		} else
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

	ND->le = readLedsParams (data, xname (nn, lab));

/* ==== */
/* EMUL */
/* ==== */

	ND->em = readEmulParams (data, xname (nn, lab));

/* ======== */
/* PTRACKER */
/* ======== */

	ND->pt = readPwtrParams (data, xname (nn, lab));

/* ==== */
/* UART */
/* ==== */

	ND->ua = readUartParams (data, xname (nn, lab));

/* ==== */
/* PINS */
/* ==== */

	ND->pn = readPinsParams (data, xname (nn, lab));

/* ================= */
/* Sensors/Actuators */
/* ================= */

	ND->sa = readSensParams (data, xname (nn, lab));

/* ======== */
/* Location */
/* ======== */

	if ((cur = sxml_child (data, "location")) != NULL)
		ND->Movable = parseLocation (cur, nn >= 0, nn, &(ND->X),
			&(ND->Y)
#if ZZ_R3D
		      , &(ND->Z)
#endif
				);
	return ND;
}

Boolean __pi_validate_uart_rate (word rate) {

	int i;

	for (i = 0; i < sizeof (urates)/sizeof (word); i++)
		if (urates [i] == rate)
			return YES;

	return NO;
}

data_ua_t *BoardRoot::readUartParams (sxml_t data, const char *esn) {
/*
 * Decodes UART parameters
 */
	sxml_t cur;
	nparse_t np [2];
	const char *att;
	char es [48], um;
	data_ua_t *UA;
	int len;

	if ((data = sxml_child (data, "uart")) == NULL)
		return NULL;

	strcpy (es, "<uart> for ");
	strcat (es, esn);

	UA = new data_ua_t;

	np [0].type = np [1].type = TYPE_LONG;

	/* The rate */
	UA->URate = 0;
	if ((att = sxml_attr (data, "rate")) != NULL) {
		if (parseNumbers (att, 1, np) != 1 || np [0].LVal <= 0)
			xeai ("rate", es, att);
		len = (int) (np [0].LVal);
		// Make sure it is decent
		if (len < 0 || (len % 100) != 0)
			xeai ("rate", es, att);
		// We store it in hundreds
		UA->URate = (word) (len / 100);
		if (!__pi_validate_uart_rate (UA->URate))
			xeai ("rate", es, att);
	}

	// The default is "direct"
	UA->iface = UART_IMODE_D;
	um = 'd';
	if ((att = sxml_attr (data, "mode")) != NULL) {
		if (*att == 'p') {
			// Packet P-mode
			UA->iface = UART_IMODE_P;
			um = 'p';
		} else if (*att == 'n') {
			// Packet N-mode
			UA->iface = UART_IMODE_N;
			um = 'n';
		} else if (*att == 'l') {
			// Line packet
			UA->iface = UART_IMODE_L;
			um = 'l';
		} else if (*att == 'e') {
			// escaped mode
			UA->iface = UART_IMODE_E;
			um = 'e';
		} else if (*att == 'f') {
			// escaped mode
			UA->iface = UART_IMODE_F;
			um = 'f';
		} else if (*att != 'd')
			// It can only be "direct", "n-packet", "p-packet",
			// "l-packet"
			xeai ("mode", es, att);
	}

	/* Buffer size */
	UA->UIBSize = UA->UOBSize = 0;

	if ((att = sxml_attr (data, "bsize")) != NULL) {
		len = parseNumbers (att, 2, np);
		if ((len != 1 && len != 2) || np [0].LVal < 0)
			xeai ("bsize", es, att);
		UA->UOBSize = UA->UIBSize = (word) (np [0].LVal);
		if (len == 2) {
			if (np [1].LVal < 0)
				xeai ("bsize", es, att);
			UA->UOBSize = (word) (np [1].LVal);
		}
	}
	print (form ("  UART [rate = %1d bps, mode = %c, bsize i = %1d, "
		"o = %d bytes]:\n", (int)(UA->URate) * 100, um,
			UA->UIBSize, UA->UOBSize));

	UA->UMode = get_io_desc (data, es, (const char**)(&(UA->UIDev)),
		(const char**)(&(UA->UODev)));

	if ((UA->UMode & XTRN_IMODE_MASK) != XTRN_IMODE_NONE) {
		// The 'type'
		cur = sxml_child (data, "input");
		if ((att = sxml_attr (cur, "type")) != NULL) {
			if (strcmp (att, "timed") == 0) {
				UA->UMode |= XTRN_IMODE_TIMED;
			} else if (strcmp (att, "untimed"))
				xeai ("type", es, att);
		}
		// And the coding
		if ((att = sxml_attr (cur, "coding")) != NULL) {
			if (strcmp (att, "hex") == 0) {
				UA->UMode |= XTRN_IMODE_HEX;
			} else if (strcmp (att, "ascii"))
				xeai ("coding", es, att);
		}
	}

	if ((UA->UMode & XTRN_OMODE_MASK) == XTRN_OMODE_SOCKET) {
		// This also applies to input: check for the type attribute
		cur = sxml_child (data, "output");
		if ((att = sxml_attr (cur, "type")) != NULL) {
			if (strcmp (att, "held") == 0 ||
			    strcmp (att, "hold") == 0 ||
			    strcmp (att, "wait") == 0 ) {
				// Hold output until connected
				UA->UMode |= XTRN_OMODE_HOLD;
			}
			// Ignore other types for now; we may need more
			// later
		}
	}

	if ((UA->UMode & XTRN_OMODE_MASK) != XTRN_OMODE_NONE) {
		// The coding
		cur = sxml_child (data, "output");
		if ((att = sxml_attr (cur, "coding")) != NULL) {
			if (strcmp (att, "hex") == 0) {
				UA->UMode |= XTRN_OMODE_HEX;
			} else if (strcmp (att, "ascii"))
				xeai ("coding", es, att);
		}
	}

	// The missing pieces of listing

	if ((UA->UMode & (XTRN_IMODE_TIMED | XTRN_IMODE_HEX))) {
		print ("    INPUT:");
		if ((UA->UMode & XTRN_IMODE_TIMED))
			print (" TIMED");
		if ((UA->UMode & XTRN_IMODE_HEX))
			print (" HEX");
		print ("\n");
	}

	if ((UA->UMode & (XTRN_OMODE_HOLD | XTRN_OMODE_HEX))) {
		print ("    OUTPUT:");
		if ((UA->UMode & XTRN_OMODE_HOLD))
			print (" HELD");
		if ((UA->UMode & XTRN_OMODE_HEX))
			print (" HEX");
		print ("\n");
	}

	print ("\n");

	// Check if the UART is there after all this parsing
	UA->absent = ((UA->UMode & (XTRN_OMODE_MASK | XTRN_IMODE_MASK)) == 0);

	if (!UA->absent && UA->URate == 0)
		xemi ("rate", es);

	return UA;
}

data_pn_t *BoardRoot::readPinsParams (sxml_t data, const char *esn) {
/*
 * Decodes PINS parameters
 */
	double d;
	sxml_t cur;
	nparse_t np [3], *npp;
	const char *att;
	char es [48];
	char *str, *sts;
	data_pn_t *PN;
	byte *BS;
	short *SS;
	int len, ni, nj;
	byte pn;

	if ((data = sxml_child (data, "pins")) == NULL)
		return NULL;

	strcpy (es, "<pins> for ");
	strcat (es, esn);

	PN = new data_pn_t;

	PN->NA = 0;
	PN->MPIN = PN->NPIN = PN->D0PIN = PN->D1PIN = BNONE;
	PN->ST = PN->IV = PN->BN = NULL;
	PN->VO = NULL;
	PN->absent = NO;

	/* Total number of pins */
	if ((att = sxml_attr (data, "total")) == NULL &&
	    (att = sxml_attr (data, "number")) == NULL) {
		PN->absent = YES;
		return PN;
	}

	np [0].type = np [1].type = np [2].type = TYPE_LONG;

	for (len = 0; len < 7; len++)
		PN->DEB [len] = 0;
	
	if (parseNumbers (att, 1, np) != 1 || np [0].LVal < 0 ||
	    np [0].LVal > MAX_PINS)
		xeai ("total", es, att);

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
		    xeai ("adc", es, att);
		PN->NA = (byte) (np [0].LVal);
	}

	/* Counter */
	if ((att = sxml_attr (data, "counter")) != NULL) {
		if ((ni = parseNumbers (att, 3, np)) < 1) {
CntErr:
		      xeai ("counter", es, att);
		}
		if (np [0].LVal < 0 || np [0].LVal >= PN->NP)
			goto CntErr;
		PN->MPIN = (byte) (np [0].LVal);
		if (ni > 1) {
			// Debouncers
			if ((PN->DEB [0] = (Long)(np [1].LVal)) < 0)
				goto CntErr;
			if (ni > 2) {
				if ((PN->DEB [1] = (Long)(np [2].LVal)) < 0)
					goto CntErr;
			}
		}
	}

	/* Notifier */
	if ((att = sxml_attr (data, "notifier")) != NULL) {
		if ((ni = parseNumbers (att, 3, np)) < 1) {
NotErr:
		      xeai ("notifier", es, att);
		}
		if (np [0].LVal < 0 || np [0].LVal >= PN->NP)
			goto NotErr;
		PN->NPIN = (byte) (np [0].LVal);
		if (PN->NPIN == PN->MPIN)
			goto NotErr;
		if (ni > 1) {
			// Debouncers
			if ((PN->DEB [2] = (Long)(np [1].LVal)) < 0)
				goto NotErr;
			if (ni > 2) {
				if ((PN->DEB [3] = (Long)(np [2].LVal)) < 0)
					goto NotErr;
			}
		}
	}

	/* DAC */
	if ((att = sxml_attr (data, "dac")) != NULL) {
		len = parseNumbers (att, 2, np);
		if (len < 1 || len > 2)
	        	xeai ("dac", es, att);
		if (np [0].LVal < 0 || np [0].LVal >= PN->NP ||
		     np [0].LVal == PN->MPIN || np [0].LVal == PN->NPIN)
	        	xeai ("dac", es, att);
		// The firs one is OK
		PN->D0PIN = (byte) (np [0].LVal);
		if (len == 2) {
			// Verify the second one
			if (np [1].LVal < 0 || np [1].LVal >= PN->NP ||
			  np [0].LVal == np [1].LVal || np [1].LVal == PN->MPIN
			    || np [1].LVal == PN->NPIN)
	        	      xeai ("dac", es, att);
			PN->D1PIN = (byte) (np [1].LVal);
		}
	}

	print (form ("  PINS [total = %1d, adc = %1d", PN->NP, PN->NA));
	if (PN->MPIN != BNONE) {
		print (form (", PM = %1d", PN->MPIN));
		if (PN->DEB [0] != 0 || PN->DEB [1] != 0)
			print (form (" /%1d,%1d/", PN->DEB [0], PN->DEB [1]));
	}
	if (PN->NPIN != BNONE) {
		print (form (", EN = %1d", PN->NPIN));
		if (PN->DEB [2] != 0 || PN->DEB [3] != 0)
			print (form (" /%1d,%1d/", PN->DEB [2], PN->DEB [3]));
	}
	if (PN->D0PIN != BNONE) {
		print (form (", DAC = %1d", PN->D0PIN));
		if (PN->D1PIN != BNONE)
			print (form ("+%1d", PN->D1PIN));
	}
	print ("]:\n");

	/* I/O */
	PN->PMode = get_io_desc (data, es, &(PN->PIDev), &(PN->PODev));
			  
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
			sts++;
		}
		print ("    STATUS: ");
		for (pn = 0; pn < PN->NP; pn++)
			print (PINS::gbit (BS, pn) ? "1" : "0");
		print ("\n");
		// trace ("strpool PINS");
		PN->ST = find_strpool ((const byte*)BS, len, NO);
		if (PN->ST != BS)
			// Recycled
			delete [] BS;
	}

	/* Buttons */
	if ((cur = sxml_child (data, "buttons")) != NULL) {

		// Polarity: default == high
		PN->BPol = 1;
		if ((att = sxml_attr (cur, "polarity")) != NULL &&
			(*att == '0' || strcmp (att, "low") == 0))
				PN->BPol = 0;

		// Debounce/repeat
		nj = 0;
		if ((att = sxml_attr (cur, "timing")) != NULL) {
			len = parseNumbers (att, 3, np);
			if (len < 1 || len > 3)
				xeai ("timing", es, att);
			for (pn = 0; pn < len; pn++) {
				if (np [pn].LVal < 0 || np [pn].LVal > MAX_WORD)
					xeai ("timing", es, att);
				PN->DEB [4 + pn] = (Long)(np [pn].LVal);
			}
			nj = 1;
		}
		// We expect at most NP integer (byte-sized) values
		// identifying button numbers
		BS = new byte [PN->NP];
		npp = new nparse_t [PN->NP];
		for (pn = 0; pn < PN->NP; pn++) {
			// This means "not assigned to a button"
			BS [pn] = BNONE;
			npp [pn] . type = TYPE_int;
		}
		str = (char*) sxml_txt (cur);
		if ((len = parseNumbers (str, PN->NP, npp)) < 0)
			excptn ("Root: illegal int value in <buttons> for %s",
				es);
		if (len > PN->NP)
			excptn ("Root: too many values in <buttons> for %s",
				es);
		for (pn = 0; pn < len; pn++) {
			if ((ni = npp [pn] . IVal) < 0)
				// This means skip
				continue;
			// Check if not taken
			if (PN->ST != NULL && PINS::gbit (PN->ST, pn) == 0)
				// Absent
				excptn ("Root: pin %1d in <buttons> for %s is "
					"declared as absent", pn, es);
			if (ni > MAX_PINS)
				excptn ("Root: button number %1d in <buttons> "
					"for %s is too big (%1d is the max)",
						ni, es, MAX_PINS);
			BS [pn] = (byte) ni;
		}

		delete [] npp;

		print ("    BUTTONS: ");
		for (pn = 0; pn < PN->NP; pn++)
			print (BS [pn] == BNONE ? "- " :
				form ("%1d ", BS [pn]));
		print ("\n");
		if (nj)
			// Timing
			print (form ("      Timing: %1d, %1d, %1d\n",
				PN->DEB [4], PN->DEB [5], PN->DEB [6]));

		// trace ("strpool BUTS");
		PN->BN = find_strpool ((const byte*)BS, PN->NP, NO);
		if (PN->BN != BS)
			// Recycled
			delete [] BS;
	}

	/* Default (initial) pin values */
	if ((cur = sxml_child (data, "values")) != NULL) {
		BS = new byte [len = ((PN->NP + 7) >> 3)];
		memset (BS, 0, len);
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
			sts++;
		}
		print ("    VALUES: ");
		for (pn = 0; pn < PN->NP; pn++)
			print (PINS::gbit (BS, pn) ? "1" : "0");
		print ("\n");
		// trace ("strpool PDEFS");
		PN->IV = find_strpool ((const byte*)BS, len, NO);
		if (PN->IV != BS)
			// Recycled
			delete [] BS;
	}

	/* Default (initial) ADC input voltage */
	if (PN->NA != 0 && ((cur = sxml_child (data, "voltages")) != NULL ||
				(cur = sxml_child (data, "voltage")) != NULL)) {
		SS = new short [PN->NA];
		memset (SS, 0, PN->NA * sizeof (short));
		npp = new nparse_t [PN->NA];
		for (pn = 0; pn < PN->NA; pn++)
			npp [pn] . type = TYPE_double;
		str = (char*)sxml_txt (cur);
		len = parseNumbers (str, PN->NA, npp);
		if (len > PN->NA)
			excptn ("Root: too many FP values in <voltages> for %s",
				es);
		if (len < 0)
			excptn ("Root: illegal FP value in <voltages> for %s",
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

		// trace ("strpool VOLT");
		PN->VO = (const short*) find_strpool ((const byte*) SS,
			(int)(PN->NA) * sizeof (short), NO);

		if (PN->VO != SS)
			// Recycled
			delete [] SS;

		print ("    ADC INPUTS: ");
		for (pn = 0; pn < PN->NA; pn++)
			print (form ("%5.2f ", (double)(PN->VO [pn]) *
				3.3/32768.0));
		print ("\n");

		delete [] npp;
	}

	print ("\n");

	return PN;
}

static int sa_smax (const char *what, const char *err, sxml_t root) {
/*
 * Calculates the maximum number of sensors/actuators
 */
	int max, min, last;
	const char *att;
	nparse_t np [1];

	np [0].type = TYPE_LONG;
	max = MININT;
	last = -1;
	min = MAXINT;
	// No consistency checks yet - just the maximum
	for (root = sxml_child (root, what); root != NULL;
						root = sxml_next (root)) {
		
		if ((att = sxml_attr (root, "number")) != NULL ||
		    (att = sxml_attr (root, "index")) != NULL) {
			if (parseNumbers (att, 1, np) != 1 ||
			    np [0].LVal < -256 || np [0].LVal > 255)
				// This is an error
				xeai ("number/index", err, att);
			last = (int) (np [0].LVal);
		} else
			last++;

		if (last > max)
			max = last;
		if (last < min)
			min = last;
	}

	if (max == MININT)
		return 0;

	max = max - min + 1;

	if (max > 256)
		excptn ("Too large span of %s (%1d - %1d)", err, min, max);

	return max;
}

int SensActDesc::bsize (lword bound) {
/*
 * Determines the number size for sensor value bound. Note that the
 * value returned by a sensor is normalized and unsigned, such that
 * Min == 0
 */
	if ((bound & 0xFF000000))
		return 4;
	if ((bound & 0x00FF0000))
		return 3;
	if ((bound & 0x0000FF00))
		return 2;
	return 1;
}

Boolean SensActDesc::expand (address vp) {
/*
 * Expand and store the value
 */
	lword v;

	switch (Length) {
	
		case 1:
			v = (lword) *((byte*) vp);
			break;

		case 2:

			v = (lword) *((word*) vp);
			break;

		case 3:
			// This one assumes little endianness (avoid!!!)
			v = 	((lword)(*((byte*)vp))) |
				(((lword)(*(((byte*)vp) + 1))) <<  8) |
				(((lword)(*(((byte*)vp) + 2))) << 16);
			break;
		case 4:
			v = *((lword*)vp);
			break;

		default:
			excptn ("Illegal length in sensor: %1d", Length);
	}
	return set (v);
}

void SensActDesc::get (address vp) {
/*
 * Retrieve a properly sized value
 */
	switch (Length) {

		case 1:
			*((byte*) vp) = (byte) Value;
			return;
		case 2:
			*((word*) vp) = (word) Value;
			return;
		case 3:
			// This one assumes little endianness (avoid!!!)
			*(((byte*)vp) + 0) = (byte) (Value      );
			*(((byte*)vp) + 1) = (byte) (Value >>  8);
			*(((byte*)vp) + 2) = (byte) (Value >> 16);
			return;
		case 4:
			*((lword*) vp) = Value;
			return;

		default:
			excptn ("Illegal length in sensor: %1d", Length);
	}
}

static SensActDesc *sa_doit (const char *what, sint *off, const char *erc,
							  sxml_t root, int n) {
/*
 * Initialize the sensor/actuator array
 */
	int first, last, i, j, rqs, mns, fill;
	sxml_t sns;
	lword ival;
	const char *att;
	nparse_t np [2];
	SensActDesc *res;
	byte Type;

	Type = strcmp (what, "sensor") ? SEN_TYPE_ACTUATOR : SEN_TYPE_SENSOR;
	
	res = new SensActDesc [n];
	memset (res, 0, sizeof (SensActDesc) * n);
	np [0].type = TYPE_LONG;
	np [1].type = TYPE_LONG;

	mns = MAXINT;
	rqs = MININT;
	last = -1;

	for (sns = sxml_child (root, what); sns != NULL;
						sns = sxml_next (sns)) {
		np [0].type = np [1].type = TYPE_LONG;
		
		if ((att = sxml_attr (sns, "number")) != NULL ||
		    (att = sxml_attr (sns, "index")) != NULL) {
			if (parseNumbers (att, 1, np) != 1 ||
			    np [0].LVal < -256 ||
			    np [0].LVal >  255   )
				xeai ("number/index", erc, att);
			last = (int) (np [0].LVal);
		} else
			last++;
		if (last > rqs)
			rqs = last;
		if (last < mns)
			mns = last;
	}

	*off = 0;
	if (mns <= rqs && (mns < 0))
		*off = (sint) (-mns);

	last = -1;
	fill = 0;

	for (sns = sxml_child (root, what); sns != NULL;
						sns = sxml_next (sns)) {
		np [0].type = np [1].type = TYPE_LONG;
		
		if ((att = sxml_attr (sns, "number")) != NULL ||
		    (att = sxml_attr (sns, "index")) != NULL) {
			parseNumbers (att, 1, np);
			last = (int) (np [0].LVal) + *off;
		} else
			last++;
		
		if (last >= n || res [last] . Length != 0)
			xeai ("number/index", erc, att);

		fill++;

		res [last] . Type = Type;
		res [last] . Id = last;

		// Check for range
		att = sxml_txt (sns);
		i = parseNumbers (att, 1, np);
		if (i == 0) {
			// Assume unsigned full size
			res [last] . Max = 0xffffffff;
		} else if (i < 0) {
			excptn ("Root: illegal numerical value for %s in %s",
				what, erc);
		} else {
			if (np [0] . LVal <= 0)
			   excptn ("Root: max value for %s in %s: is <= 0 (%s)",
			      what, erc, att);
			res [last] . Max = (lword) (np [0] . LVal);
		}

		// Check for initial value
		if ((att = sxml_attr (sns, "init")) != NULL) {
			if (parseNumbers (att, 1, np) != 1 || np [0] . LVal < 0)
				xeai ("init", erc, att);
			ival = (lword)(np [0].LVal);
		} else
			ival = 0;

		// Check for explicit length indication
		if ((att = sxml_attr (sns, "vsize")) != NULL) {
			if (parseNumbers (att, 1, np) != 1 ||
				np [0].LVal < 1 || np [0].LVal > 4)
					excptn ("Root: illegal value size in "
						"%s in %s: %s", what, erc, att);
			rqs = (int) (np [0].LVal);
		} else
			// Requested size, if any
			rqs = 0;

		// Size derived from Max
		mns = SensActDesc::bsize (res [last] . Max);

		if (i == 0) {
			// Derive bound from size
			if (rqs == 0)
				// This is the default
				rqs = 4;	
			else if (rqs < 4)
				// Actually derive the upper bound
				res [last] . Max = (1 << (rqs*8)) - 1;
		} else if (rqs == 0) {
			// Derive size from Max
			rqs = mns;
		} else {
			// Verify
			if (rqs < mns)
				excptn ("Root: requested vsize for %s in %s is "
					"less than minimum required: %s",
						what, erc, att);
		}

		// Timing
		np [0].type = np [1].type = TYPE_double;
		if ((att = sxml_attr (sns, "delay")) != NULL) {
			if ((j = parseNumbers (att, 2, np)) < 1 || j > 2)
				xeai ("delay", erc, att);
			if ((res [last] . MinTime = np [0] . DVal) < 0.0)
				xeai ("delay", erc, att);
			if (j > 1) {
				if ((res [last] . MaxTime = np [1] . DVal) <
							  res [last] . MinTime)
					xeai ("delay", erc, att);
			} else {
				res [last] . MaxTime = res [last] . MinTime;
			}
		}

		res [last] . Length = rqs;
		res [last] . set (ival);
		res [last] . RValue = res [last] . Value;
	}

	print (form ("  %s [total = %1d, used = %1d",
		strcmp (what, "sensor") ? "ACTUATORS" : "SENSORS", n, fill));

	if (*off)
		print (form (", hidden = %1d", *off));

	print ("]:\n");

	for (i = 0; i < n; i++)
		if (res [i] . Length)
			print (form ("    NUMBER %1d, VSize %1d, "
				"Range %1u, Init: %1u, "
					"Delay: [%7.4f,%7.4f]\n",
						i - *off, res [i] . Length,
						res [i] . Max,
						res [i] . Value,
						res [i] . MinTime,
						res [i] . MaxTime));
	return res;
}

data_sa_t *BoardRoot::readSensParams (sxml_t data, const char *esn) {
/*
 * Decodes Sensors/Actuators
 */
	data_sa_t *SA;
	char es [48];

	if ((data = sxml_child (data, "sensors")) == NULL)
		return NULL;

	strcpy (es, "<sensors> for ");
	strcat (es, esn);

	SA = new data_sa_t;

	SA->Sensors = SA->Actuators = NULL;
	SA->absent = NO;

	SA->NS = (byte) sa_smax ("sensor", es, data);
	SA->NA = (byte) sa_smax ("actuator", es, data);

	if (SA->NS == 0 && SA->NA == 0) {
		// That's it
		SA->absent = YES;
		return SA;
	}

	if (SA->NS != 0) {
		SA->Sensors = sa_doit ("sensor", &(SA->SOff), es, data, SA->NS);
		if (SA->NA != 0)
			print ("\n");
	}

	if (SA->NA != 0)
		SA->Actuators = sa_doit ("actuator", &(SA->AOff), es, data,
			SA->NA);

	SA->SMode = get_io_desc (data, es, &(SA->SIDev), &(SA->SODev));
	
	print ("\n");

	return SA;
}

data_le_t *BoardRoot::readLedsParams (sxml_t data, const char *esn) {
/*
 * Decodes LEDs parameters
 */
	nparse_t np [1];
	data_le_t *LE;
	const char *att;
	char es [48];

	strcpy (es, "<leds> for ");
	strcat (es, esn);

	if ((data = sxml_child (data, "leds")) == NULL)
		return NULL;

	LE = new data_le_t;
	LE->LODev = NULL;

	if ((att = sxml_attr (data, "number")) == NULL &&
	    (att = sxml_attr (data, "total")) == NULL    ) {
		LE->absent = YES;
		return LE;
	} else {
		LE->absent = NO;
		np [0].type = TYPE_LONG;
		if (parseNumbers (att, 1, np) != 1 || np [0].LVal < 0 ||
		    np [0].LVal > 64)
			xeai ("number", es, att);
		if (np [0].LVal == 0) {
			// Explicit NO
			print ("  LEDS: none\n");
			LE->absent = YES;
			return LE;
		}
	}

	if ((LE->NLeds = (word) (np [0].LVal)) > MAX_LEDS)
		xeai ("number", es, att);

	print (form ("  LEDS: %1d\n", LE->NLeds));

	// Output mode
	if (get_io_desc (data, es, NULL, &(LE->LODev)) == 0)
		xenf ("<output>", es);

	print ("\n");

	return LE;
}

data_em_t *BoardRoot::readEmulParams (sxml_t data, const char *esn) {
/*
 * Decodes LEDs parameters
 */
	data_em_t *EM;
	const char *att;
	char es [48];
	sxml_t oi;

	strcpy (es, "<emul> for ");
	strcat (es, esn);

	if ((data = sxml_child (data, "emul")) == NULL)
		return NULL;

	EM = new data_em_t;
	EM->EODev = NULL;
	EM->held = EM->absent = NO;

	if ((oi = sxml_child (data, "output")) == NULL) {
		// Indicates "absent"
		EM->absent = YES;
		return EM;
	}

	if ((att = sxml_attr (oi, "type")) != NULL) {
		// Same as for UART
		if (strcmp (att, "held") == 0 ||
		    strcmp (att, "hold") == 0 ||
		    strcmp (att, "wait") == 0 ) {
			EM->held = YES;
		}
	}

	print ("  EMUL\n");

	// Output mode
	get_io_desc (data, es, NULL, &(EM->EODev));
	if (EM->EODev == NULL && EM->held)
		// Socket + HELD flag
		print ("    OUTPUT: HELD\n");

	print ("\n");
	return EM;
}

data_pt_t *BoardRoot::readPwtrParams (sxml_t data, const char *esn) {
/*
 * Decodes power tracker parameters
 */
	sxml_t cur;
	nparse_t np [16];
	double lv [16];
	data_pt_t *PT;
	pwr_mod_t *md;
	int i, nm, cm;
	const char *att;
	char *str;
	char es [48];

	const char *mids [] = { "cpu", "radio", "storage", "sensors", "rf",
				"eeprom", "sd", "sensor" };

	strcpy (es, "<ptracker> for ");
	strcat (es, esn);

	if ((data = sxml_child (data, "ptracker")) == NULL)
		return NULL;

	PT = new data_pt_t;
	PT->absent = YES;

	for (i = 0; i < PWRT_N_MODULES; i++)
		PT->Modules [i] = NULL;

	for (i = 0; i < 16; i++)
		np [i].type = TYPE_double;

	for (cur = sxml_child (data, "module"); cur != NULL;
							cur = sxml_next (cur)) {

		if ((att = sxml_attr (cur, "id")) == NULL)
			xemi ("id", es);

		for (cm = 0; cm < 8; cm++)
			if (strcmp (att, mids [cm]) == 0)
				break;
		if (cm == 8)
			xeai ("id of <module>", es, att);

		switch (cm) {

			case 0:
			case 1:
			case 2:
			case 3:
				break;
			case 4:
				cm = 1;
				break;
			case 5:
			case 6:
				cm = 2;
				break;
			default:
				cm = 3;
		}

		if (PT->Modules [cm] != NULL)
			excptn ("Root: duplicate module id: %s in %s", att, es);
		
		PT->Modules [cm] = md = new pwr_mod_t;

		// Levels
		str = (char*) sxml_txt (cur);
		sanitize_string (str);
		if ((nm = parseNumbers (str, 16, np)) > 16)
			excptn ("Root: too many levels (%1d) in module %s in"
				"%s, max is 16", nm, att, es);
		if (nm < 2)
			excptn ("Root: less than two levels in module %s in %s",
				att, es);

		for (i = 0; i < nm; i++)
			lv [i] = np [i].DVal;

		// trace ("strpool PLEVS");
		md->Levels = (double*) find_strpool ((const byte*)lv, 
			nm * sizeof (double), YES);

		md->NStates = nm;
		PT->absent = NO;
	}

	if (PT->absent) {
		// Explicitly absent
		return PT;
	}

	print ("  PTRACKER:\n");

	PT->PMode = get_io_desc (data, es, &(PT->PIDev), &(PT->PODev));

	for (cm = 0; cm < PWRT_N_MODULES; cm++) {
		if ((md = PT->Modules [cm]) == NULL)
			continue;
		print (form ("    MODULE: %s, Levels:", mids [cm]));
		for (i = 0; i < md->NStates; i++)
			print (form (" %g", md->Levels [i]));
		print ("\n");
	}
	print ("\n");

	return PT;
}

static void append_suppl (const char *src, int len = 0) {
//
// Appends a string at the end of the supplement data
//
	if (len == 0)
		// We have a string
		len = strlen (src);

	if (len == 0)
		// A precaution
		return;

	if ((__pi_XML_Suppl = (char*) realloc (__pi_XML_Suppl,
		__pi_XML_Suppl_Length + len)) == NULL)
			excptn ("Root: out of memory for supplementary data");

	memcpy (__pi_XML_Suppl + __pi_XML_Suppl_Length, src, len);

	__pi_XML_Suppl_Length += len;
}

void BoardRoot::initNodes (sxml_t data, int NT, int NN, const char *BDLB [],
							const char *BDFN [],
								    int NFC) {
	data_no_t *DEF, *BDEF [NFTABLE_SIZE], *NOD, *D;
	const char *bfn, *def_type, *nod_type, *att, *start;
	char *xdata;
	sxml_t cno, lcc, cur, *xnodes;
	Long i, j, last, fill;
	int tq, tl;
	nparse_t np [2];
	data_rf_t *NRF, *DRF;
	data_ep_t *NEP, *DEP;
	location_t *locs;

	print ("\nTiming:\n\n");
	print ((double) etuToItu (1.0),
		   "  ETU (1s) = ", 11, 14);
	print ((double) duToItu (1.0),
		   "  DU  (1m) = ", 11, 14);
	print (getTolerance (&tq),
	           "  TOL      = ", 11, 14);
	print (tq, "  TOL QUAL = ", 11, 14);
	print ("\n");

	TheStation = System;
	locs = NULL;

	if ((data = sxml_child (data, "nodes")) == NULL)
		xenf ("<nodes>", "<network>");

	// Check for the defaults
	cno = sxml_child (data, "defaults");
	start = NULL;
	if (cno != NULL) {
		def_type = sxml_attr (cno, "type");
		start = sxml_attr (cno, "start");
	} else
		def_type = NULL;

	// DEF is never NULL, remember to deallocate it
	DEF = readNodeParams (cno, -1, NULL, start);

	// Read the supplementary data, i.e., board-specific <node> defaults
	if (NFC) {

		append_suppl ("<supplement>\n");

		for (tq = 0; tq < NFC; tq++) {
			xdata = __pi_rdfile (bfn = BDFN [tq], NULL, 0, tl);
			if (tl < 0)
				excptn ("Root: cannot access board defaults "
					"file %s", bfn);
			if (tl == 0)
				excptn ("Root: the board defaults file %s is "
					"empty", bfn);
			// Append to the set
			append_suppl ("<board");
			if (BDLB [tq] != NULL) {
				append_suppl (" type=\"");
				append_suppl (BDLB [tq]);
				append_suppl ("\"");
			}
			append_suppl (">\n");
			append_suppl (xdata, tl);
			append_suppl ("</board>\n");

			// sxml_parse_str1 marks the input string for
			// dealloaction when the sxml structure is deallocated
			cno = sxml_parse_str1 (xdata, tl);
			if (!sxml_ok (cno))
				excptn ("Root: failed to parse the board "
					"defaults file %s, %s", bfn,
					sxml_error (cno));
			if (strcmp (sxml_name (cno), "node") != 0)
				excptn ("Root: the board defaults file %s has "
					"no <node> element", bfn);

			BDEF [tq] = readNodeParams (cno, -2, BDLB [tq],
				sxml_attr (cno, "start"));
			sxml_free (cno);
		}

		append_suppl ("</supplement>\n");
		append_suppl ("\0", 1);
	}

	np [0] . type = TYPE_LONG;

	// Read "locations" if separate from the nodes
	if ((lcc = sxml_child (data, "locations")) != NULL) {

		locs = new location_t [NT];

		for (i = 0; i < NT; i++)
			// Mark as unused
			locs [i] . Movable = 0xff;

		last = -1;
		fill = 0;

		print ("Node locations:\n\n");

		for (cno = sxml_child (lcc, "location"); cno != NULL;
							cno = sxml_next (cno)) {
			if ((att = sxml_attr (cno, "number")) == NULL)
				att = sxml_attr (cno, "index");

			if (att != NULL) {
				if (parseNumbers (att, 1, np) != 1 ||
				    np [0].LVal < 0 || np [0].LVal >= NT)
				      xeai ("number/index", "<location>", att);
				last = (Long) (np [0].LVal);
			} else
				last++;

			if (last >= NT)
				excptn ("Root: implict node index at <location>"
					" reaches the limit of %1d", NT);

			if (locs [last] . Movable != 0xff)
				excptn ("Root: location for node %1d multiply"
					" defined", last);

			locs [last] . Movable = parseLocation (cno, 2, last,
				&(locs [last] . x), &(locs [last] . y)
#if ZZ_R3D
						  , &(locs [last] . z)
#endif
									);

			fill++;
		}

		print ("\n");
	}

	xnodes = new sxml_t [NT];
	for (i = 0; i < NT; i++)
		xnodes [i] = NULL;

	last = -1;
	fill = 0;

	for (cno = sxml_child (data, "node"); cno != NULL;
							cno = sxml_next (cno)) {
		if ((att = sxml_attr (cno, "number")) == NULL)
			att = sxml_attr (cno, "index");
		
		if (att != NULL) {
			if (parseNumbers (att, 1, np) != 1 ||
			    np [0].LVal < 0 || np [0].LVal >= NT)
				xeai ("number/index", "<node>", att);
			last = (Long) (np [0].LVal);
		} else
			last++;

		if (last >= NT)
			excptn ("Root: implict node index reaches the limit"
				" of %1d", NT);

		if (xnodes [last] != NULL)
			excptn ("Root: node %1d multiply defined", last);

		xnodes [last] = cno;

		fill++;
	}

	if (fill < NT) {
		for (i = 0; i < NT; i++)
			if (xnodes [i] == NULL)
				break;
		excptn ("Root: some nodes undefined, first is %1d", i);
	}

	for (tq = i = 0; i < NT; i++) {
		cno = xnodes [i];
		start = sxml_attr (cno, "start");
		NOD = readNodeParams (cno, i, NULL, start);

		if ((nod_type = sxml_attr (cno, "type")) == NULL)
			nod_type = def_type;

		att = sxml_attr (cno, "default");

		// Check if the node type has a board default
		D = NULL;
		for (tl = 0; tl < NFC; tl++) {
			if (nstrcmp (nod_type, BDLB [tl]) == 0) {
				D = BDEF [tl];
				break;
			}
		}

		// Which default to use
		if (att == NULL || *att == 'd') {
			// Generic defaults only
			D = DEF;
		} else if (*att == 'b') {
			// Board only
			if (D == NULL)
				excptn ("Root: node type <%s>, number %1d, "
					"requires board default, but no such "
					"default is available",
					nod_type ? nod_type : "*", i);
		} else {
			// Flexible
			if (D == NULL)
				D = DEF;
		}

		// Substitute defaults as needed; validate later
		if (NOD->Mem == 0)
			NOD->Mem = D->Mem;

		if (NOD->On == WNONE)
			NOD->On = D->On;

		if (NOD->PLimit == WNONE)
			NOD->PLimit = D->PLimit;

		if (NOD->Lcdg == WNONE)
			NOD->Lcdg = D->Lcdg;

		// === radio ==================================================

		NRF = NOD->rf;
		DRF = D->rf;

		if (NRF == NULL) {
			// Inherit the defaults
			if (DRF != NULL && !(DRF->absent)) {
				NRF = NOD->rf = DRF;
			}
		} else if (NRF->absent)  {
			// Explicit absent
			delete NRF;
			NRF = NOD->rf = NULL;
		} else if (DRF != NULL && !(DRF->absent)) {

			// Partial defaults

			if (NRF->Boost == HUGE)
				NRF->Boost = DRF->Boost;

			if (NRF->Rate == WNONE)
				NRF->Rate = DRF->Rate;

			if (NRF->Power == WNONE)
				NRF->Power = DRF->Power;

			if (NRF->Channel == WNONE)
				NRF->Channel = DRF->Channel;

			if (NRF->BCMin == WNONE) {
				NRF->BCMin = DRF->BCMin;
				NRF->BCMax = DRF->BCMax;
			}

			if (NRF->Pre == WNONE)
				NRF->Pre = DRF->Pre;

			if (NRF->LBTDel == WNONE) {
				NRF->LBTDel = DRF->LBTDel;
				NRF->LBTThs = DRF->LBTThs;
				NRF->LBTTries = DRF->LBTTries;
			}
		}

		// === EEPROM =================================================

		NEP = NOD->ep;
		DEP = D->ep;

		if (NEP == NULL) {
			// Inherit the defaults
			if (DEP != NULL && !(DEP->absent)) {
				NEP = NOD->ep = DEP;
			}
		} else if (NEP->absent) {
			// Explicit "no", ignore the default
			delete NEP;
			NEP = NOD->ep = NULL;
		} else if (DEP != NULL && !(DEP->absent)) {
			// Partial defaults?
			if (NEP->EEPRS == LWNONE) {
				// FIXME: provide a function to copy this
				NEP->EEPRS = DEP->EEPRS;
				NEP->EEPPS = DEP->EEPPS;
				NEP->EPIF = DEP->EPIF;
				// Do not deallocate arrays
				NEP->EFLGS = DEP->EFLGS | NVRAM_NOFR_EPINIT;
				NEP->EPINI = DEP->EPINI;
				NEP->EECL = DEP->EECL;
				for (j = 0; j < EP_N_BOUNDS; j++)
					NEP->bounds [j] =
						DEP->bounds [j];
			}
			if (NEP->IFLSS == WNONE) {
				NEP->IFLSS = DEP->IFLSS;
				NEP->IFLPS = DEP->IFLPS;
				NEP->IFIF = DEP->IFIF;
				// Do not deallocate arrays
				NEP->EFLGS |= NVRAM_NOFR_IFINIT;
				NEP->IFINI = DEP->IFINI;
				NEP->IFCL = DEP->IFCL;
			}
		}

		if (NEP) {
			// FIXME: this desperately calls to be cleaned up
			if (NEP->EEPRS == LWNONE)
				NEP->EEPRS = 0;
			if (NEP->IFLSS == WNONE)
				NEP->IFLSS = 0;
		}

		// === UART ===================================================

		if (NOD->ua == NULL) {
			// Inherit the defaults
			if (D->ua != NULL && !(D->ua->absent))
				NOD->ua = D->ua;
		} else if (NOD->ua->absent) {
			// Explicit "no", ignore the default
			delete NOD->ua;
			NOD->ua = NULL;
		}

		// === PINS ===================================================

		if (NOD->pn == NULL) {
			// Inherit the defaults
			if (D->pn != NULL && !(D->pn->absent))
				NOD->pn = D->pn;
		} else if (NOD->pn->absent) {
			// Explicit "no", ignore the default
			delete NOD->pn;
			NOD->pn = NULL;
		}

		// === Sensors/Actuators ======================================

		if (NOD->sa == NULL) {
			// Inherit the defaults
			if (D->sa != NULL && !(D->sa->absent))
				NOD->sa = D->sa;
		} else if (NOD->sa->absent) {
			// Explicit "no", ignore the default
			delete NOD->sa;
			NOD->sa = NULL;
		}

		// === LEDS ===================================================

		if (NOD->le == NULL) {
			// Inherit the defaults
			if (D->le != NULL && !(D->le->absent))
				NOD->le = D->le;
		} else if (NOD->le->absent) {
			// Explicit "no", ignore the default
			delete NOD->le;
			NOD->le = NULL;
		}

		// === EMUL ===================================================

		if (NOD->em == NULL) {
			// Inherit the defaults
			if (D->em != NULL && !(D->em->absent))
				NOD->em = D->em;
		} else if (NOD->em->absent) {
			// Explicit "no", ignore the default
			delete NOD->em;
			NOD->em = NULL;
		}

		// === PTRACKER ===============================================

		if (NOD->pt == NULL) {
			// Inherit the defaults
			if (D->pt != NULL && !(D->pt->absent))
				NOD->pt = D->pt;
		} else if (NOD->pt->absent) {
			// Explicit "no", ignore the default
			delete NOD->pt;
			NOD->pt = NULL;
		}

		// A few checks; some of this stuff is checked (additionally) at
		// the respective constructors

		if (NOD->Mem == 0)
			excptn ("Root: memory for node %1d is undefined", i);

		if (NOD->PLimit == WNONE)
			NOD->PLimit = 0;

		if (NOD->Lcdg == WNONE)
			NOD->Lcdg = 0;

		if (NRF != NULL) {

			// Count those with RF interface
			tq++;
			if (tq > NN)
				excptn ("Root: too many nodes with RF interface"
					", %1d expected", NN);

			if (NRF->Boost == HUGE)
				// The default is no boost (0dB)
				NRF->Boost = 0.0;

			if (NRF->Rate == WNONE)
				// This one has a reasonable default
				NRF->Rate = Ether->Rates->lower ();

			if (NRF->Power == WNONE) {
				// And so does this one
				NRF->Power = __pi_channel_type ==
				    CTYPE_NEUTRINO ? 0 : Ether->PS->lower ();
			}

			if (NRF->Channel == WNONE)
				// And so does this
				NRF->Channel = 0;

			if (NRF->BCMin == WNONE) 
				excptn ("Root: backoff for node %1d is "
					"undefined", i);

			if (NRF->Pre == WNONE)
				excptn ("Root: preamble for node %1d is "
					"undefined", i);

			if (NRF->LBTDel == WNONE)
				excptn ("Root: LBT parameters for node %1d are "
					"undefined", i);
		}

		if (NOD->X < 0.0 || NOD->Y < 0.0
#if ZZ_R3D
				 || NOD->Z < 0.0
#endif
						) {
			// No location
			if (locs && locs [i] . Movable != 0xff) {
				NOD->X = locs [i] . x;
				NOD->Y = locs [i] . y;
#if ZZ_R3D
				NOD->Z = locs [i] . z;
#endif
				NOD->Movable = locs [i] . Movable;
			}
		}

		if (NOD->X < 0.0 || NOD->Y < 0.0
#if ZZ_R3D
				 || NOD->Z < 0.0
#endif
			) {
			if (NRF != NULL)
			  	excptn ("Root: no location for node %1d (which"
					" is equipped with radio)", i);
			// OK, if no radio, but force it to be legal
			NOD->X = NOD->Y =
#if ZZ_R3D
				 NOD->Z =
#endif
					  0.0;
			NOD->Movable = NO;
		} else {
			// Location OK
			if (NOD->Movable != NO && NOD->Movable != YES)
				// Movability unknown, use the default
				NOD->Movable = D->Movable;
		}

		buildNode (nod_type, NOD);

		// Deallocate the data spec. Note that ua and pn may contain
		// arrays, but those arrays need not (must not) be deallocated.
		// They are either strings pointed to in the sxmtl tree, which
		// is deallocated elsewhere, or intentionally copied (constant)
		// strings that have been linked to by the respective objects.

		if (NOD->rf != NULL && NOD->rf != D->rf)
			delete NOD->rf;
		if (NOD->ep != NULL && NOD->ep != D->ep)
			deallocate_ep_def (NOD->ep);
		if (NOD->ua != NULL && NOD->ua != D->ua)
			delete NOD->ua;
		if (NOD->pn != NULL && NOD->pn != D->pn)
			delete NOD->pn;
		if (NOD->sa != NULL && NOD->sa != D->sa)
			delete NOD->sa;
		if (NOD->le != NULL && NOD->le != D->le)
			delete NOD->le;
		if (NOD->em != NULL && NOD->em != D->em)
			delete NOD->em;
		if (NOD->pt != NULL && NOD->pt != D->pt)
			delete NOD->pt;
		delete NOD;
	}

	// Delete the scratch node list
	delete [] xnodes;
	// Delete the scratch location table
	if (locs)
		delete [] locs;

	for (tl = 0; tl <= NFC; tl++) {
		// Delete default data blocks
		D = (tl == NFC) ? DEF : BDEF [tl];

		if (D->rf != NULL)
			delete D->rf;
		if (D->ep != NULL)
			deallocate_ep_def (D->ep);
		if (D->ua != NULL)
			delete D->ua;
		if (D->pn != NULL)
			delete D->pn;
		if (D->sa != NULL)
			delete D->sa;
		if (D->pt != NULL)
			delete D->pt;
		if (D->le != NULL)
			delete D->le;
		if (D->em != NULL)
			delete D->em;

		delete D;
	}

	if (tq < NN)
		excptn ("Root: too few (%1d) nodes with RF interface, %1d"
			" expected", tq, NN);
}

void BoardRoot::initAll () {

	sxml_t xml;
	const char *att;
	nparse_t np [1];
	Boolean nc, sm, ri;
	int NN, NT, NFC;
	double SF;
	Long RI;
	const char *BDFN [NFTABLE_SIZE], *BDLB [NFTABLE_SIZE];

	settraceFlags (	TRACE_OPTION_TIME +
			TRACE_OPTION_ETIME +
			TRACE_OPTION_STATID +
			TRACE_OPTION_PROCESS );

	SF = VUEE_SLOMO_FACTOR;
	RI = VUEE_RESYNC_INTERVAL;

	// Board-specific node defaults
	NFC = 0;

	// VUEE-specific arguments (if present): slomo factor, resync interval

	sm = ri = NO;

	while (PCArgc) {
		PCArgc--;
		att = *PCArgv++;
		if (strcmp (att, "-s") == 0) {
			// Slo-mo factor
			if (sm) {
ADup:
				excptn ("Root: duplicate argument %s", att);
			}
			if (PCArgc == 0) {
AExp:
				excptn ("Root: argument expected after %s",
					att);
			}
			np [0] . type = TYPE_double;
			if (parseNumbers (*PCArgv, 1, np) != 1) {
ANum:
				excptn ("Root: arg of %s is not a number", att);
			}
			SF = np [0] . DVal;
			if (SF <= 0.0) {
				// Treat as unsynced
				SF = 1.0;
				if (ri)
					excptn ("Root: -s 0 forces unsynced, "
						"conflicts with previous -r");
				RI = 0;
				ri = YES;
			}
			sm = YES;
		} else if (strcmp (att, "-r") == 0) {
			// Resync interval
			if (ri)
				excptn ("Root: illegal -r argument");

			if (PCArgc == 0)
				goto AExp;

			np [0] . type = TYPE_LONG;
			if (parseNumbers (*PCArgv, 1, np) != 1)
				goto ANum;
			RI = (Long) (np [0] . LVal);
			if (RI < 0 || RI > 1000)
				excptn ("Root: resync intvl must be >= 0 and "
					"<= 1000");
			ri = YES;
		} else if (strcmp (att, "-n") == 0) {
			// Node defaults file
			if (PCArgc == 0)
				goto AExp;
			if (NFC == NFTABLE_SIZE)
				excptn ("Root: too many board-specific node "
					"defaults, the limit is %1d",
					NFTABLE_SIZE);
			if (PCArgc > 1 && **(PCArgv+1) != '-') {
				// Label + file
				BDLB [NFC] = *PCArgv++;
				PCArgc--;
			} else {
				BDLB [NFC] = NULL;
			}
			BDFN [NFC] = *PCArgv;
			for (NN = 0; NN < NFC; NN++)
				if (nstrcmp (BDLB [NFC], BDLB [NN]) == 0)
					excptn ("Root: duplicate pgm label in "
						"-n, %s", BDLB [NFC] ?
							  BDLB [NFC] :
							  "--null--");
			NFC++;
		} else if (strcmp (att, "-p") == 0) {
			// Agent port
			if (PCArgc == 0)
				goto AExp;
			np [0] . type = TYPE_LONG;
			if (parseNumbers (*PCArgv, 1, np) != 1)
				goto ANum;
			if (np [0] . LVal > 0xFFFF || np [0] . LVal <= 0)
				excptn ("Root: port number must be > 0 && < "
					"64K");
			__pi_Agent_Port = (word) (np [0] . LVal);
			force_port = YES;
		} else
			excptn ("Root: illegal PC argument %s", att);

		PCArgc--;
		PCArgv++;
	}

			
	xml = sxml_parse_input ('\0', &__pi_XML_Data, &__pi_XML_Data_Length);

	if (!sxml_ok (xml))
		excptn ("Root: XML input data error, %s",
			(char*)sxml_error (xml));

	if (strcmp (sxml_name (xml), "network"))
		excptn ("Root: <network> data expected");

	np [0] . type = TYPE_LONG;

	// Decode the number of stations
	if ((att = sxml_attr (xml, "nodes")) == NULL)
		xemi ("nodes", "<network>");

	if (parseNumbers (att, 1, np) != 1)
		xeai ("nodes", "<network>", att);

	NT = (int) (np [0] . LVal);
	if (NT <= 0)
		excptn ("Root: 'nodes' in <network> must be strictly positive, "
			"is %1d", NT);

	// How many interfaced to radio?
	if ((att = sxml_attr (xml, "radio")) == NULL) {
		// All of them by default
		NN = NT;
		nc = YES;
	} else {
		if (parseNumbers (att, 1, np) != 1)
			xeai ("radio", "<network>", att);

		NN = (int) (np [0] . LVal);
		if (NN < 0 || NN > NT)
			excptn ("Root: the 'radio' attribute in <network> must "
				"be >= 0 <= 'nodes', " "is %1d", NN);
		nc = NO;
	}
	// Check for the non-standard port
	if (!force_port && (att = sxml_attr (xml, "port")) != NULL) {
		if (parseNumbers (att, 1, np) != 1 || np [0] . LVal < 1 ||
		    np [0] . LVal > 0x0000ffff)
			xeai ("port", "<network>", att);
		__pi_Agent_Port = (word) (np [0] . LVal);
	}

	initTiming (xml);
	// The third argument tells whether NN is a default copy of NT or not;
	// in the former case, we can force it to zero, if there is no channel
	// definition in the input data set
	NN = initChannel (xml, NN, nc);

	initNodes (xml, NT, NN, BDLB, BDFN, NFC);
	initPanels (xml);
	initRoamers (xml);
	initAgent (xml);
	

	sxml_free (xml);

	print (RI, "Resync interval: ", 11, 20);
	print (SF, "Slow motion factor:", 11, 20);
	
	print ("\n");

	setResync (RI, (RI * 0.001)/SF);
	//setResync (500, 0.5);

	create (System) AgentInterface;

	if (Ether)
		Ether->printTop ("RF CHANNEL exposure\n\n");
}

// ======================================================== //
// Here is the part to run under the fake PicOS environment //
// ======================================================== //

#include "stdattr.h"

// === UART direct ============================================================

void d_uart_inp_p::setup () {

	assert (S->uart->IMode == UART_IMODE_D,
		"d_uart_inp_p->setup: illegal mode");
	f = UART_INTF_D (S->uart);
	assert (f->pcsInserial == NULL,
		"d_uart_inp_p->setup: duplicated process");
	f->pcsInserial = this;
}

void d_uart_inp_p::close () {
	f->pcsInserial = NULL;
	S->tally_out_pcs ();
	terminate ();
	sleep;
}

d_uart_inp_p::perform {

    int quant;

    _pp_enter_ ();

    state IM_INIT:

	if (f->__inpline != NULL)
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
		f->__inpline = tmp;
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
		f->__inpline = tmp;
		close ();
	}
	ptr += quant;
	proceed IM_BIN1;

}

void d_uart_out_p::setup (const char *d) {

	assert (S->uart->IMode == UART_IMODE_D,
		"d_uart_out_p->setup: illegal mode");
	f = UART_INTF_D (S->uart);
	assert (f->pcsOutserial == NULL,
		"d_uart_out_p->setup: duplicated process");
	assert (d != NULL, "d_uart_out_p->setup: string pointer is NULL");
	f->pcsOutserial = this;
	ptr = data = d;
}

void d_uart_out_p::close () {
	f->pcsOutserial = NULL;
	S->tally_out_pcs ();
	terminate ();
	sleep;
}

d_uart_out_p::perform {

    int quant;

    _pp_enter_ ();

    state OM_INIT:

	if (*ptr)
		len = strlen (ptr);
	else
		len = ptr [1] +3; // 3: 0x00, len, 0x04

    transient OM_WRITE:

	quant = io (OM_WRITE, 0, WRITE, (char*)ptr, len);
    	ptr += quant;
	len -= quant;
	if (len == 0) {
		/* This is always a fresh buffer allocated dynamically */
		ufree (data);
		close ();
	}
	proceed OM_WRITE;
}

// ============================================================================

void delay (word msec, int state) {

	TIME delta;

	delta = etuToItu (msec * MILLISECOND);
	ThePPcs->WaitingUntil = Time + delta;
	setFlag (ThePPcs->Flags, _PP_flag_wtimer);
	Timer->wait (delta, state);
}

word dleft (sint pid) {
//
// Left-to 'delay' (we are trying to approximate what you can see in
// PicOS/kernel/kernel.c)
//
	_PP_ *p;
	double d;

	if (pid == 0) {
		// This process
		p = ThePPcs;
	} else {
		p = find_pcs_by_id (pid);
		if (p == NULL || flagCleared (p->Flags, _PP_flag_wtimer))
			return MAX_WORD;
	}
	if (p->WaitingUntil <= Time)
		return 0;

	// To milliseconds
	d = ituToEtu (p->WaitingUntil - Time) * MSCINSECOND;
	return d > (double)(MAX_WORD-1) ? MAX_WORD-1 : (word) d;
}

void hold (int st, lword sec) {
/*
 * Wait until the beginning of next full second
 */
	TIME delta;
	double d, s;

	// Fractional current second
	d = (ituToEtu (Time) - (double)(TheNode->SecondOffset));

	// Target second
	s = (double) sec;

	if (s <= d)
		return;

	delta = etuToItu (s - d);
	if (delta == TIME_0)
		return;

	ThePPcs->WaitingUntil = Time + delta;
	setFlag (ThePPcs->Flags, _PP_flag_wtimer);
	Timer->wait (delta, st);
	sleep;
}

sint __pi_getcpid () {

	return ThePPcs->ID;
}

sint __pi_join (sint pid, word st) {

	Process *p;

	if ((p = find_pcs_by_id (pid)) == NULL)
		return 0;

	p->wait (DEATH, st);
	return pid;
}

void __pi_kill (sint pid) {
//
// PicOS's variant of kill
//
	_PP_ *p;

	if (pid == 0) {
		// This process, equivalent to finish
Self:
		TheNode->tally_out_pcs ();
		// This will trigger the event for joinall, direct events
		// are awaited directly
		terminate (ThePPcs);
		sleep;
		// Unreachable
	}

	// Locate the process
	if ((p = find_pcs_by_id (pid)) != NULL) {
		if (p == ThePPcs)
			goto Self;
		TheNode->tally_out_pcs ();
		p->terminate ();
	}
}

// ============================================================================
// These are inefficient; may redo them some day; would require another pair of
// links in _PP_.
// ============================================================================

sint __pi_running (code_t tid) {
//
// Return the ID of ANY running process of the given type
//
	Process *P [1];

	if (zz_getproclist (TheNode, tid, P, 1) ? __cpint (P [0]) : 0)
		// There is at least one (all we ask for)
		return ((_PP_*)(P [0])) -> ID;

	return 0;
}

void __pi_joinall (code_t pt, word st) {
//
// Wait for any process of the set
//
	TheNode->TB . wait ((IPointer) pt, st);
}

void __pi_killall (code_t tid) {

	Process *P [16];
	Long np, i;
	bool self;

	self = NO;

	while (1) {
		np = zz_getproclist (TheNode, tid, P, 16);
		for (i = 0; i < np; i++) {
			if (P [i] == ThePPcs)
				self = YES;
			terminate (P [i]);
			TheNode->tally_out_pcs ();
		}
		if (np < 16)
			break;
	}

	if (self)
		sleep;
}

int __pi_crunning (code_t tid) {
//
// Count all running processes of the given type
//
	Process **P;
	int np, sz;

	if (tid == NULL)
		// Return the number of remaining process slots
		return TheNode->tally_left ();

	for (sz = 256; ; sz *= 2) {
		P = new Process* [sz];
		np = zz_getproclist (TheNode, tid, P, sz);
		delete P;
		if (np < sz)
			break;
	}

	return np;
}

// ============================================================================

sint __pi_strlen (const char *s) {

	int i;

	for (i = 0; *(s+i) != '\0'; i++);

	return (sint) i;
}

void __pi_strcpy (char *d, const char *s) {

	while ((Boolean)(*d++ = *s++));
}

void __pi_strncpy (char *d, const char *s, int n) {

	while (n-- && (*s != '\0'))
		*d++ = *s++;
	*d = '\0';
}

void __pi_strcat (char *d, const char *s) {

	while (*d != '\0') d++;
	strcpy (d, s);
}

void __pi_strncat (char *d, const char *s, int n) {

	strcpy (d+n, s);
}

// ============================================================================

void setpowermode (word pl) {

	TheNode->pwrt_change (PWRT_CPU, pl);
}

// ============================================================================

process WatchDog (PicOSNode) {

	states { Start, Alert };

	perform {

		state Start:

			Timer->delay (1.0, Alert);
			this->wait (SIGNAL, Start);

		state Alert:

			// trace ("WATCHDOG RESET");
			reset ();
	};
};

void watchdog_start () {

	if (TheNode->Watchdog != NULL) {
		// Just reset
		TheNode->Watchdog->signal (0);
		return;
	}

	TheNode->Watchdog = create WatchDog;
}

void watchdog_stop () {

	if (TheNode->Watchdog != NULL) {
		TheNode->Watchdog->terminate ();
		TheNode->Watchdog = NULL;
	}
}

// ============================================================================

void rtc_module_t::set (const rtc_time_t *rtd) {

	struct tm lct;
	time_t tim;

	memset (&lct, 0, sizeof (lct));
	// Convert the specified time to seconds
	lct.tm_sec = rtd->second;
	lct.tm_min = rtd->minute;
	lct.tm_hour = rtd->hour;
	lct.tm_mday = rtd->day;
	if ((lct.tm_mon = rtd->month))
		// This is stored as a number sarting from 0 not from 1
		lct.tm_mon--;
	// This is since 1900
	lct.tm_year = (int)(rtd->year) + 100;

	if ((tim = mktime (&lct)) < 0)
		// Just in case: we do not detect/signal errors also in PicOS
		SecOffset = 0;
	else
		SecOffset = time (NULL) - tim;
}
	
void rtc_module_t::get (rtc_time_t *rtd) {

	time_t tim;
	struct tm *lct;

	tim = time (NULL) - SecOffset;
	lct = localtime(&tim);

	rtd->year = (byte) (lct->tm_year - 100);
	// This is from 0 rather than 1
	rtd->month = (byte) (lct->tm_mon) + 1;
	rtd->day = (byte) (lct->tm_mday);
	rtd->hour = (byte) (lct->tm_hour);
	rtd->minute = (byte) (lct->tm_min);
	rtd->second = (byte) (lct->tm_sec);
	rtd->dow = (byte) (lct->tm_wday);
}

void rtc_set (const rtc_time_t *rtct) {

	TheNode->rtc_module.set (rtct);
}

void rtc_get (rtc_time_t *rtct) {

	TheNode->rtc_module.get (rtct);
}

// ============================================================================

void vuee_control (int what, ...) {
//
// This will grow; for now, we need it to clear the power tracker from the
// praxis
//
	va_list ap;
	va_start (ap, what);

	switch (what) {

		case VCTRL_PTRCK_CLEAR:

			TheNode->pwrt_clear ();
			return;
	}
}

// ============================================================================

int _no_module_ (const char *t, const char *f) {

	excptn ("Function %s: no %s module at %s", f, t, TheNode->getSName ());
	// No way
	return 0;
}

// ============================================================================

#if 0
void check_pptable () {

	int n;
	_PP_ *tp, *tq;

	for (n = 0; n < PPHASH_SIZE; n++) {
		if ((tp = pptable [n]) == NULL)
			continue;
		tq = tp;

		do {
			if (tp->getClass () != AIC_process)
				excptn ("NOT A PROCESS: %d", n);
			tp = tp->HNext;
		} while (tp != tq);
	}
}
#endif

identify (VUEE VUEE_VERSION);

#include "lcdg_n6100p.cc"

#include "stdattr_undef.h"

#include "agent.cc"
#include "rfmodule.cc"
#include "uart_phys.cc"

#endif
