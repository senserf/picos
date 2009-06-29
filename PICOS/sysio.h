#ifndef	__sysio_h__
#define	__sysio_h__

#include "picos.h"

#include "options.sys"

#ifdef	TCV_PRESENT
#undef	TCV_PRESENT
#endif

#ifndef	CODE_LONG_INTS
#define	CODE_LONG_INTS	1
#endif

#define	CRC_ISO3309	1

// This must be overriden because TCV is always compiled in
#define	TCV_PRESENT	1

// ----------------------------------------------------------------------- //
// NOTE: this will be replaced in PicOS as an isolated fragment of sysio.h //
// ----------------------------------------------------------------------- //

#define	TCV_MAX_DESC	4
#define	TCV_MAX_PHYS	2
#define	TCV_MAX_PLUGS	3
#define	TCV_LIMIT_RCV	0
#define	TCV_LIMIT_XMT	0

#define	PHYSOPT_PLUGINFO	(-1)	/* These two are kind of special */
#define	PHYSOPT_PHYSINFO	(-2)

#define	PHYSOPT_STATUS		0	/* Get device status */
#define	PHYSOPT_TXON		1	/* Transmitter on */
#define	PHYSOPT_ON		1	/* General on */
#define	PHYSOPT_TXOFF		2	/* Transmitter off */
#define	PHYSOPT_OFF		2	/* General off */
#define	PHYSOPT_TXHOLD		3	/* OFF + queue */
#define	PHYSOPT_HOLD		3	/* General hold */
#define	PHYSOPT_RXON		4	/* Receiver on */
#define	PHYSOPT_RXOFF		5	/* Receiver off */
#define	PHYSOPT_CAV		6	/* Set collision avoidance 'vector' */
#define	PHYSOPT_SETPOWER 	7	/* Transmission power */
#define	PHYSOPT_GETPOWER	8	/* Last reception power */
#define	PHYSOPT_ERROR		9	/* Return/clear error code */
#define	PHYSOPT_SETSID		10	/* Set station (network) ID */
#define	PHYSOPT_GETSID		11	/* Return station Id */
#define	PHYSOPT_SENSE		12	/* Return channel status */
#define	PHYSOPT_SETPARAM	13	/* Set channel parameter */

#define	PHYSOPT_SETPAYLEN	14	/* Set payload length */
#define	PHYSOPT_GETPAYLEN	15

#define	PHYSOPT_SETGROUP	16	/* Set station group */
#define	PHYSOPT_GETGROUP	17

#define	PHYSOPT_SETCHANNEL	18	/* Set RF channel */
#define	PHYSOPT_GETCHANNEL	19

#define	PHYSOPT_SETMODE		20
#define	PHYSOPT_GETMODE		21

#define	PHYSOPT_SETRATE		22
#define	PHYSOPT_GETRATE		23

#define	PHYSOPT_GETMAXPL	24	/* Get the maximum packet length */

typedef	struct {
/*
 * Application data pointers. These two numbers represent the offset to the
 * starting byte in the application portion of a received packet, and the
 * offset of the last application byte from the end, or, in simple words,
 * the header and trailer length, respectively.
 */
	word	head,
		tail;
} tcvadp_t;

/*
 * Plugin functions
 */
typedef struct {
	int (*tcv_ope) (int, int, va_list);
	int (*tcv_clo) (int, int);
	int (*tcv_rcv) (int, address, int, int*, tcvadp_t*);
	int (*tcv_frm) (address, int, tcvadp_t*);
	int (*tcv_out) (address);
	int (*tcv_xmt) (address);
	int (*tcv_tmt) (address);
	int tcv_info;
} tcvplug_t;

#if	TCV_PRESENT

/* Functions, we declare them only if the device is present */
void	tcv_plug (int, const tcvplug_t*);
int	tcv_open (word, int, int, ...);
int	tcv_close (word, int);
address	tcv_rnp (word, int);
address tcv_wnp (word, int, int);
address tcv_wnpu (word, int, int);
int	tcv_qsize (int, int);
int	tcv_erase (int, int);
int	tcv_read (address, char*, int);
int	tcv_write (address, const char*, int);
void	tcv_endp (address);
void	tcv_drop (address);
int	tcv_left (address);	/* Also plays the role of old tcv_plen */
void	tcv_urgent (address);
Boolean	tcv_isurgent (address);
int	tcv_control (int, int, address);

typedef	int (*ctrlfun_t) (int option, address);

/* Disposition codes */
#define	TCV_DSP_PASS	0
#define	TCV_DSP_DROP	1
#define	TCV_DSP_RCV	2
#define	TCV_DSP_RCVU	3
#define	TCV_DSP_XMT	4
#define	TCV_DSP_XMTU	5

/* ========================= */
/* End of TCV specific stuff */
/* ========================= */
#endif

/* LEDs operations */
#define	LED_OFF		0
#define	LED_ON		1
#define	LED_BLINK	2

/* malloc shortcut */
#define	tmalloc(s)	(TheNode->memAlloc (s, (word)(s)))
#define	tfree(s)	(TheNode->memFree ((address)(s)))
#define tmwait(s)	(TheNode->waitMem((int)(s)))

#define	stackfree()	256

#ifndef	NULL
#define	NULL		0

#define	YES		1
#define	NO		0
#define	NONE		(-1)
#define	ERROR		NONE
#endif

#define	JIFFIES		1024	/* Clock ticks in a second */

#define	umalloc(s)	tmalloc (s)
#define	ufree(s)	tfree (s)
#define	umwait(s)	tmwait (s)
#define	npwait(s)	tmwait (s)

#define	SNONE		((int)NONE)
#define	WNONE		((word)NONE)
#define	LWNONE		((lword)NONE)
#define	BNONE		0xff
#define	BLOCKED		(-2)

#define	trigger(a)	(((PicOSNode*)TheStation)->TB.signal (__cpint(a)))
#define	ptrigger(a,b)	trigger(b)

#define	hexcode(a)	(isdigit(a) ? ((a) - '0') : ( ((a)>='a'&&(a)<='f') ?\
	    ((a) - 'a' + 10) : (((a)>='A'&&(a)<='F') ? ((a) - 'A' + 10) : 0) ) )

/* Errors */
#define	ENODEVICE	1	/* Illegal device */
#define	ENOOPER		2	/* Illegal operation */
#define	EREQPAR		3	/* Illegal request parameters */
#define	ERESOURCE	4	/* Out of resources */
#define	ENEVENTS	5	/* Too many wait requests */
#define	EMALLOC		6	/* Memory corruption */
#define	ESTATE		7	/* Illegal process state */
#define	EHARDWARE	8	/* Hardware error */
#define	ETOOMANY	9	/* Too many times (like more than once) */
#define	EASSERT		10	/* Consistency check failed */
#define	ESTACK		11	/* Stack overrun */
#define	EEEPROM		12	/* EEPROM reference out of range */
#define	EFLASH		13	/* FLASH reference out of range */
#define	EWATCH		14	/* Watchdog condition */

#if	BYTE_ORDER == LITTLE_ENDIAN

#define	ntowl(w)	((((w) & 0xffff) << 16) | (((w) >> 16) & 0xffff))
#define	re_endian_w(w)	CNOP
#define	re_endian_lw(w)	CNOP

#else

#define ntowl(w)	(w)
#define	re_endian_w(w)	do { (w) = (((word)(w)) >> 8) | (((word)(w)) << 8); }\
				while (0)
#define	re_endian_lw(w)	do { (w) = ((((lword)(w)) >> 24)          ) | \
				   ((((lword)(w)) >>  8) &  0xff00) | \
				   ((((lword)(w)) <<  8) & 0xff000) | \
				   ((((lword)(w)) << 24)          ) \
			} while (0)
				   
#endif	/* LITTLE_ENDIAN */

#define	wtonl(w)	ntowl (w)

#define	add_entropy(w)	(TheNode->entropy = (TheNode->entropy << 4) ^ (w))

/* ========================================== */
/* Registered identifiers of plugins and phys */
/* ========================================== */
#define	INFO_PLUG_NULL		0x0001	/* NULL plugin */
#define INFO_PLUG_TARP          0x0002  /* TARP plugin */

#define	INFO_PHYS_UART    	0x0100	/* Persistent UART */
#define	INFO_PHYS_ETHER    	0x0200	/* Raw Ethernet */
#define	INFO_PHYS_RADIO		0x0300	/* Radio */
#define INFO_PHYS_CC1000        0x0400  /* CC1000 radio */
#define	INFO_PHYS_DM2100	0x0500	/* DM2100 */
#define	INFO_PHYS_CC1100	0x0600	/* CC1100 */
#define	INFO_PHYS_DM2200	0x0700  /* VERSA 2 */
#define	INFO_PHYS_RF24L01	0x0800

/* ======================================================================== */

#define	ptrtoint(a)			__cpint (a)

#define	_da(a)				_na_ ## a
#define	_dac(a,b)			(((a *)TheStation)-> _na_ ## b)
#define	_dan(a,b)			(((a *)TheStation)-> b)
#define	_dad(t,a)			t::_na_ ## a
#define	_dap(b)				_dan (PicOSNode, b)

#define	__PRIVF(ot,tp,nam)		tp ot::nam
#define	__PUBLS(ot,tp,nam)		__PUBLF (ot, tp, nam)
#define	__PUBLF(ot,tp,nam)		tp ot::_na_ ## nam

#define	__STATIC
#define	__EXTERN
#define	__CONST
#define	__VIRTUAL	virtual
#define	__ABSTRACT	= 0
#define	__sinit(a)	

#ifndef	THREADNAME
#define	THREADNAME(a)	a
#endif

#define	threadhdr(a,b)	process THREADNAME(a) (b)
#define	strandhdr(a,b)	threadhdr (a, b)

#define	thread(a)	THREADNAME(a)::perform {
#define	runthread(a)	(((PicOSNode*)TheStation)->tally_in_pcs () ? \
				(int) create THREADNAME(a) : 0)
#define	runstrand(a,b)	(((PicOSNode*)TheStation)->tally_in_pcs () ? \
				(int) create THREADNAME(a) (b) : 0)

#define	strand(a,b)	thread(a)
#define	endthread	}
#define	endstrand	endthread

#define	praxis_starter(nt) \
		void nt::appStart () { create THREADNAME(root); }

// FIXME: cleanup this

#define	__PROCESS(a,b)	process a (PicOSNode) { \
		states { __S0, __S1, __S2, __S3, __S4 }; perform {
#define	__ENDPROCESS(a)	}};

#define	__NA(a,b)		(((a*)TheNode)->b)

/* running + killall */

#ifdef getpid
#undef getpid
#endif

#define	running(pt)	zz_running (&zz_!!THREADNAME(pt)!!_prcs)
#define	getcpid()	zz_running (NULL)
#define	crunning(pt)	zz_crunning (&zz_!!THREADNAME(pt)!!_prcs)
#define	ptleft()	zz_crunning (NULL)
#define	killall(pt)	zz_killall (&zz_!!THREADNAME(pt)!!_prcs)

#define	ee_open()	0
#define	ee_close()	CNOP
#define	ee_panic()	CNOP

#define	sd_open()	0
#define	sd_close()	CNOP
#define	sd_panic()	CNOP

#define	sd_read(a,b,c)	ee_read (a, b, c)
#define sd_write(a,b,c)	ee_write (WNONE, a, b, c)
#define	sd_erase(a,b)	ee_erase (WNONE, a, b)
#define	sd_sync()	ee_sync (WNONE)
#define	sd_size()	ee_size (NULL, NULL)
#define	sd_idle()	CNOP

void zz_dbg (int, word);

#ifndef	dbg_0
#define	dbg_0(a)	zz_dbg (0, (word)(a));
#endif
#ifndef	dbg_1
#define	dbg_1(a)	zz_dbg (1, (word)(a));
#endif
#ifndef	dbg_2
#define	dbg_2(a)	zz_dbg (2, (word)(a));
#endif
#ifndef	dbg_3
#define	dbg_3(a)	zz_dbg (3, (word)(a));
#endif
#ifndef	dbg_4
#define	dbg_4(a)	zz_dbg (4, (word)(a));
#endif
#ifndef	dbg_5
#define	dbg_5(a)	zz_dbg (5, (word)(a));
#endif
#ifndef	dbg_6
#define	dbg_6(a)	zz_dbg (6, (word)(a));
#endif
#ifndef	dbg_7
#define	dbg_7(a)	zz_dbg (7, (word)(a));
#endif
#ifndef	dbg_8
#define	dbg_8(a)	zz_dbg (8, (word)(a));
#endif
#ifndef	dbg_9
#define	dbg_9(a)	zz_dbg (9, (word)(a));
#endif
#ifndef	dbg_a
#define	dbg_a(a)	zz_dbg (10, (word)(a));
#endif
#ifndef	dbg_b
#define	dbg_b(a)	zz_dbg (11, (word)(a));
#endif
#ifndef	dbg_c
#define	dbg_c(a)	zz_dbg (12, (word)(a));
#endif
#ifndef	dbg_d
#define	dbg_d(a)	zz_dbg (13, (word)(a));
#endif
#ifndef	dbg_e
#define	dbg_e(a)	zz_dbg (14, (word)(a));
#endif
#ifndef	dbg_f
#define	dbg_f(a)	zz_dbg (15, (word)(a));
#endif

#define	AB_MODE_OFF	0
#define	AB_MODE_PASSIVE	1
#define	AB_MODE_ACTIVE	2

#endif
