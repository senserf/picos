#ifndef __pg_sysio_h
#define	__pg_sysio_h		1

/* ============================================================================ */
/*                       PicOS                                                  */
/*                                                                              */
/* The file is to be included by applications                                   */
/*                                                                              */
/*                                                                              */
/* Copyright (C) Olsonet Communications Corporation, 2002--2005                 */
/*                                                                              */
/* Permission is hereby granted, free of charge, to any person obtaining a copy */
/* of this software and associated documentation files (the "Software"), to     */
/* deal in the Software without restriction, including without limitation the   */
/* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or  */
/* sell copies of the Software, and to permit persons to whom the Software is   */
/* furnished to do so, subject to the following conditions:                     */
/*                                                                              */
/* The above copyright notice and this permission notice shall be included in   */
/* all copies or substantial portions of the Software.                          */
/*                                                                              */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  */
/* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER       */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS */
/* IN THE SOFTWARE.                                                             */
/*                                                                              */
/* ============================================================================ */

#define	SYSVERSION		"1.15"

/* ================================================= */
/* Options are now settable on per-application basis */
/* ================================================= */

#ifndef SIM_NET
#define	SIM_NET		0
#endif

#if SIM_NET==0

#include "options.sys"

#endif

//+++ "main.c"
//+++ "kernel.c"

/* ======================================================================== */
/*              C O N F I G U R A T I O N    O P T I O N S                  */
/*              ==========================================                  */
/*                                                                          */
/* We make sure that the configuration symbols receive default values, even */
/* if they are not mentioned in options.sys.                                */
/* ======================================================================== */

// This one indicates whether we are running under the simulator (0/1)
#ifndef ECOG_SIM
#define	ECOG_SIM		0
#endif

#ifndef	SCHED_PRIO
#define	SCHED_PRIO		0
#endif

// The maximum number of tasks in the system. The overall performance may
// depend on it (somewhat).
#ifndef	MAX_TASKS
#define	MAX_TASKS		16
#endif

#ifndef	MAX_PRIO
#define MAX_PRIO                (MAX_TASKS *10)
#endif

// Detect stack overrun
#ifndef	STACK_GUARD
#define	STACK_GUARD		0
#endif

// The maximum number of simultaneously awaited events (timer excluded)
// per task (see above).
#ifndef	MAX_EVENTS_PER_TASK
#define	MAX_EVENTS_PER_TASK	4
#endif

// This one makes system messages go to UART_A. Also 'diag' becomes enabled
// and useful for debugging (0/1). If DIAG_MESSAGES > 1, system error
// messages are verbose.
#ifndef	DIAG_MESSAGES
#define	DIAG_MESSAGES	2
#endif

// Use switches for PIO input
#ifndef	SWITCHES
#define	SWITCHES		0
#endif

// Indicates whether the UART driver is present at all (0/1/2, 2 == both UARTs,
// i.e., A and B)
#ifndef	UART_DRIVER
#define	UART_DRIVER		0
#endif

// LEDs driver present (0/1)
#ifndef	LEDS_DRIVER
#define	LEDS_DRIVER		0
#endif

// LCD driver present (0/1)
#ifndef	LCD_DRIVER
#define	LCD_DRIVER		0
#endif

// RADIO driver present (0/1)
#ifndef	RADIO_DRIVER
#define	RADIO_DRIVER		0
#endif
// radio options
#define	RADIO_NONE              0
#define	RADIO_RFMI		2
#define	RADIO_XEMICS		3
// Select the radio type in options.sys if present
#ifndef RADIO_TYPE
#define RADIO_TYPE              RADIO_NONE
#endif
// If this is nonzero, the radio driver is assisted by a helper simulating
// interrupts (triggering events) when an activity is sensed
#define	RADIO_INTERRUPTS	4

// Indicates whether the Ethernet chip should be configured into the system
#ifndef	ETHERNET_DRIVER
#define	ETHERNET_DRIVER		1
#endif

// Indicates whether TCV is present
#ifndef	TCV_PRESENT
#define	TCV_PRESENT		1
#endif

#if TCV_PRESENT
// These select TCV configuration options
#ifndef	TCV_TIMERS
#define	TCV_TIMERS		1	// 0/1
#endif
#ifndef	TCV_HOOKS
#define	TCV_HOOKS		1	// 0/1
#endif
#ifndef	TCV_MAX_DESC
#define	TCV_MAX_DESC		16	// Maximum number of sessions
#endif
#ifndef	TCV_MAX_PHYS
#define	TCV_MAX_PHYS		3	// Maximum number of physical interfaces
#endif
#ifndef	TCV_MAX_PLUGS
#define	TCV_MAX_PLUGS		3	// Maximul number of plugins

//+++ "tcv.c"

#endif
// TCV_PRESENT
#endif

// If this is 1, SDRAM is configured in. Dynamic memory is allocated within
// the first 32K page of SDRAM. THe remaining SDRAM is available through
// ramget / ramput. (0/1)
#ifndef	SDRAM_PRESENT
#define	SDRAM_PRESENT		0
#endif

// Makes long encoding/decoding formats (%ld, %lu, %lx) available in form
// and scan (library functions) (0/1)
#ifndef	CODE_LONG_INTS
#define	CODE_LONG_INTS		1
#endif

// Configures the ADC interface: 0/1
#ifndef	ADC_PRESENT
#define	ADC_PRESENT		0
#endif

// Use a single memory pool for malloc (may make better sense for tight
// memory boards)
#ifndef	MALLOC_SINGLEPOOL
#define	MALLOC_SINGLEPOOL	0
#endif

// Calculate malloc statistics
#ifndef	MALLOC_STATS
#define	MALLOC_STATS		0
#endif

// Safe malloc (safer anyway)
#ifndef	MALLOC_SAFE
#define	MALLOC_SAFE		0
#endif

#ifndef CHIPCON
#define CHIPCON                 0
#endif

#ifndef	DM2100
#define	DM2100			0
#endif

#if	CHIPCON
#ifndef	CHIPCON_FREQ
// Default Chipcon frequency. 868 is another option.
#define	CHIPCON_FREQ		433
#endif
#endif

#ifndef	ENTROPY_COLLECTION
#define	ENTROPY_COLLECTION	0
#endif

/* ======================================================================== */
/*        E N D    O F    C O N F I G U R A T I O N     O P T I O N S       */
/* ======================================================================== */

/* ================================== */
/* Some hard configuration parameters */
/* ================================== */
#define	MAX_DEVICES		6
#define	MAX_MALLOC_WASTE	12

/* ======================================================================== */
/* This one is not easy to change because the loop in timer_int is unwound. */
/* So changing this will require you to modify timer_int by hand.           */
/* ======================================================================== */
#define MAX_UTIMERS		4

#if	UART_DRIVER == 0
#undef	DIAG_MESSAGES
#define	DIAG_MESSAGES		0
#endif

#include "mach.h"


#if SIM_NET==0

#if     CHIPCON
// Chipcon has a separate module integrating phys + driver
#if     RADIO_DRIVER
#error  "CHIPCON and RADIO_DRIVER are incompatible"
#endif
#if	DM2100
#error  "CHIPCON and DM2100 are incompatible"
#endif
#if     TCV_PRESENT == 0
#error  "CHIPCON requires TCV_PRESENT"
#endif
//+++ "phys_chipcon.c"
#endif	/* CHIPCON */

#if     DM2100
// Chipcon has a separate module integrating phys + driver
#if     RADIO_DRIVER
#error  "DM2100 and RADIO_DRIVER are incompatible"
#endif
#if     TCV_PRESENT == 0
#error  "DM2100 requires TCV_PRESENT"
#endif
//+++ "phys_dm2100.c"
#endif	/* DM2100 */

#if	RADIO_DRIVER == 0
#undef	RADIO_INTERRUPTS
#undef	RADIO_TYPE
#define	RADIO_INTERRUPTS	0
#define	RADIO_TYPE		0
#endif

#if	RADIO_TYPE == RADIO_XEMICS
#undef	RADIO_INTERRUPTS
#define	RADIO_INTERRUPTS	1
#if	LEDS_DRIVER
#error	LEDS and XEMICS radio cannot be configured together
#endif
//+++ "radio.c"
#endif /* XEMICS */

#define	MAX_INT			((int)0x7fff)
#define	MAX_UINT		((word)0xffff)
#define	MIN_INT			((int)0x8000)
#define	MIN_UINT		((word)0)
#define	MAX_LONG		((long)0x7fffffffL)
#define	MIN_LONG		((long)0x80000000L)
#define MAX_ULONG		((lword)0xfffffffL)
#define	MIN_ULONG		((lword)0)

/* ============================ */
/* Device identifiers (numbers) */
/* ============================ */
#define	UART_A			0
#define	UART_B			1
#define	UART			UART_A
#define	LCD			2
#define	LEDS			3
#define	ETHERNET		4
#define	RADIO			5

/* ========================================= */
/* The number of clock interrupts per second */
/* ========================================= */
#define	JIFFIES			1024	/* Clock ticks in a second           */
					/* FIXME: check how accurate this is */
#define	NULL			0
#define	NONE			((word)(-1))
#define	ERROR			NONE
#define	BLOCKED			((word)(-2))

#define	NO	((bool)0)
#define	YES	((bool)1)

/* ============= */
/* Byte ordering */
/* ============= */
#if	LITTLE_ENDIAN

#define	ntohs(w)	((((w)&0xff)<<8)|(((w)>>8)&0xff))
#define ntohl(w)	((((w)&0xff)<<24)|(((w)&0xff00)<<8)|\
				(((w)>>8)&0xff00)|(((w)>>24)&0xff))
#define	ntowl(w)	((((w) & 0xffff) << 16) | (((w) >> 16) & 0xffff))

#else

#define	ntohs(w)	(w)
#define ntohl(w)	(w)
#define ntowl(w)	(w)

#endif	/* LITTLE_ENDIAN */

#define	htons(w)	ntohs (w)
#define	htonl(w)	ntohl (w)
#define	wtonl(w)	ntowl (w)

#define	wsizeof(a)	((sizeof (a) + sizeof (word) - 1) / sizeof (word))

/* ============================================================ */
/* The main program (process) to be provided by the application */
/* ============================================================ */
int	root (word state, address data);

typedef	int (*code_t)(word, address);

int		zzz_fork (code_t func, address data);
void		reset (void);
void		halt (void);
int		zzz_strlen (const char*);
void		zzz_strcpy (char*, const char*);
void		zzz_strncpy (char*, const char*, int);
void		zzz_bcopy (const char *src, char *dest, int);
void		zzz_strcat (char*, const char*);
void		zzz_strncat (char*, const char*, int);
void		zzz_memcpy (char *dest, const char *src, int);
void		zzz_memset (char *dest, char c, int);

#if	MALLOC_SINGLEPOOL

address			zzz_malloc (word);
void			zzz_free (address);
void			zzz_waitmem (word);

#if	MALLOC_STATS
word			zzz_memfree (address);
word			zzz_maxfree (address);
#define	memfree(p,s)	zzz_memfree (s)
#define	maxfree(p,s)	zzz_maxfree (s)
#endif

#define	free(p,s)	zzz_free ((address)(s))
#define	malloc(p,s)	zzz_malloc (s)
#define	waitmem(p,t)	zzz_waitmem (t)

#else	/* MALLOC_SINGLEPOOL */

address			zzz_malloc (int, word);
void			zzz_free (int, address);
void			zzz_waitmem (int, word);
#if	MALLOC_STATS
word			zzz_memfree (int, address);
word			zzz_maxfree (int, address);
#define	memfree(p,s)	zzz_memfree (p, s)
#define	maxfree(p,s)	zzz_maxfree (p, s)
#endif
#define	free(p,s)	zzz_free (p, (address)(s))
#define	malloc(p,s)	zzz_malloc (p,s)
#define	waitmem(p,t)	zzz_waitmem (p, t)

#endif	/* MALLOC_SINGLEPOOL */

#if	STACK_GUARD
word			zzz_stackfree (void);
#define	stackfree()	zzz_stackfree ()
#endif

#if	DIAG_MESSAGES > 1
void		zzz_syserror (int, const char*);
#define		syserror(a,b)	zzz_syserror (a, b)
#define		sysassert(a,b)	do { \
					if (!(a)) syserror (EASSERT, b); \
				} while (0)
#else
void		zzz_syserror (int);
#define		syserror(a,b)	zzz_syserror (a)
#define		sysassert(a,b)
#endif

#if	SDRAM_PRESENT
void		ramget (address, lword, int);
void		ramput (lword, address, int);
#endif

void	diag (const char *, ...);
void	leds (word);
word	switches (void);

/* ======================================= */
/* This is the common i/o request function */
/* ======================================= */
int	io (int, int, int, char*, int);
#define	ion(a,b,c,d)	io (NONE, a, b, c, d)

/* User wait */
void	wait (word, word);
/* Timer wait */
void	delay (word, word);
/* Continue timer wait */
void	snooze (word);
/* Signal trigger: returns the number of awakened processes */
int	trigger (word signal);
/* Kill the indicated process */
int	kill (int);
/* Kill all processes running this code */
int	killall (code_t);

#if SCHED_PRIO
/* Prioritize for scheduling */
int     prioritizeall (code_t, int);
int     prioritize (int, int);
#else
#define	prioritizeall(a,b)	0
#define	prioritizel(a,b)	0
#endif

/* Wait for a process */
int	join (int, word);
/* Wait for any process running this code */
void	joinall (code_t, word);
/* Locate a process by code */
int	running (code_t);
/* Locate a process by code and data */
int	zzz_find (code_t, address);
/* Locate a zombie by code */
int	zombie (code_t);
/* Check for waiting or being a zombie */
int	status (int);
/* Transform pid into code pointer */
code_t	getcode (int);
/* Proceed to another state */
void	proceed (word);
/* Power up/down functions */
void	powerup (void), powerdown (void), clockup (void), clockdown (void);
/* User timers */
int	utimer (address, bool);
/* Second clock */
lword	seconds (void);
/* Spin delay */
void	udelay (word);
void	mdelay (word);

#if	RADIO_INTERRUPTS
bool	rcvwait (word);
void	rcvcancel (void);
int	rcvlast (void);
#endif

#define	call(p,d,s)	do { join (fork (p, d), s); release; } while (0)

#define	finish		kill (0)
#define	hang		kill (-1)
#define	fork(p,d)	zzz_fork (p, (address)(d))
#define	find(p,d)	zzz_find (p, (address)(d))
#define	iszombie(p)	(status (p) == -1)
#define	getpid()	running (NULL)

#define	heapmem		const word zzz_heap [] =

#define strlen(s)	zzz_strlen (s)
#define	strcpy(a,b)	zzz_strcpy (a, b)
#define	strncpy(a,b,c)	zzz_strncpy (a, b, c)
#define	strcat(a,b) 	zzz_strcat (a, b)
#define	strncat(a,b,c)	zzz_strncat (a, b, c)
#define	memcpy(a,b,c)	zzz_memcpy ((char*)(a), (const char*)(b), c)
#define	memset(a,b,c)	zzz_memset ((char*)(a), (char)(b), c)

/* User malloc shortcut */
#define	umalloc(s)	malloc (0, s)
#define	ufree(p)	free (0, p)
#define umwait(s)	waitmem (0, s)
/* Actual size of an malloc'ed piece */
#define	actsize(p)	(*(((word*)(p))-1) << 1)

/* Process operations */
#define	process(p,d)	int p (word zz_st, address zz_da) { \
				d *data = (d*) zz_da; \
				switch (zz_st) {

#define	endprocess(n)			break; \
				    default: \
					if (zz_st == 0xffff) return (n); \
					syserror (ESTATE, "no such state"); \
				} return 1; }

#define	entry(s)	case s:

#ifdef	DEBUG_BUFFER
void	dbb (word);
#else
#define	dbb(a)
#endif

/* I/O operations */
#define	READ		0
#define	WRITE		1
#define	CONTROL		2

#define	UART_CNTRL_LCK		1	/* UART lock/unlock */
#define	UART_CNTRL_RATE		2	/* Set baud rate */
#define	UART_CNTRL_MODE		3	/* Bits + parity */

#define	LCD_CNTRL_POS		1	/* Position (SEEK) */
#define	LCD_CNTRL_ERASE		2	/* Clear */

#define	LEDS_CNTRL_SET		1

#define RADIO_CNTRL_XMTCTRL	1	/* Transmitter enable/disable */
#define RADIO_CNTRL_RCVCTRL	2	/* Receiver enable/disable */
#define	RADIO_CNTRL_READSTAT	3
#define	RADIO_CNTRL_READPOWER	4
#define	RADIO_CNTRL_SETPOWER	5
#define	RADIO_CNTRL_CALIBRATE	6
#define	RADIO_CNTRL_CHECKSUM	7	/* Enable/disable checksum */
#define	RADIO_CNTRL_SETPRLEN	8	/* Cycles in transmitted preamble */
#define	RADIO_CNTRL_SETPRWAIT	9	/* Waiting for preamble high */
#define	RADIO_CNTRL_SETPRTRIES	10	/* Set num of retries for rcvd prmbl */

#define	ETHERNET_CNTRL_PROMISC	1	/* Ethernet promiscuous mode on/off */
#define	ETHERNET_CNTRL_MULTI	2	/* Accept multicast on/off */
#define	ETHERNET_CNTRL_SETID	3	/* Set card ID for cooked mode */
#define	ETHERNET_CNTRL_RMODE	4	/* Set read mode */
#define	ETHERNET_CNTRL_WMODE	5	/* Set write mode */
#define	ETHERNET_CNTRL_GMODE	6	/* Return last read mode */
#define	ETHERNET_CNTRL_ERROR	7	/* Return error status */
#define	ETHERNET_CNTRL_SENSE	8	/* Check for pending rcv packet */

#define	ETHERNET_MODE_RAW	0
#define	ETHERNET_MODE_COOKED	1
#define	ETHERNET_MODE_BOTH	2

/* ============================================= */
/* TCV specific stuff to be visible by everybody */
/* ============================================= */

/*
 * General control options for radio interfaces and their models
 */
#define	PHYSOPT_PLUGINFO	(-1)	/* These two are kind of special */
#define	PHYSOPT_PHYSINFO	(-2)

#define	PHYSOPT_STATUS		0	/* Get device status */
#define	PHYSOPT_TXON		1	/* Transmitter on */
#define	PHYSOPT_TXOFF		2	/* Transmitter off */
#define	PHYSOPT_TXHOLD		3	/* OFF + queue */
#define	PHYSOPT_RXON		4	/* Receiver on */
#define	PHYSOPT_RXOFF		5	/* Receiver off */
#define	PHYSOPT_CAV		6	/* Set collision avoidance 'vector' */
#define	PHYSOPT_SETPOWER 	7	/* Transmission power */
#define	PHYSOPT_GETPOWER	8	/* Last reception power */
#define	PHYSOPT_ERROR		9	/* Return/clear error code */
#define	PHYSOPT_SETSID		10	/* Set station (network) ID */
#define	PHYSOPT_SENSE		11	/* Return channel status */
#define	PHYSOPT_SETPARAM	12	/* Set channel parameter */

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
void	tcv_plug (int, tcvplug_t*);
int	tcv_open (word, int, int, ...);
int	tcv_close (word, int);
address	tcv_rnp (word, int);
address tcv_wnp (word, int, int);
int	tcv_read (address, char*, int);
int	tcv_write (address, const char*, int);
void	tcv_endp (address);
int	tcv_left (address);	/* Also plays the role of old tcv_plen */
void	tcv_urgent (address);
int	tcv_control (int, int, address);

/* TCV malloc shortcut */
#define	tmalloc(s)	malloc (1, s)
#define	tfree(s)	free (1, s)
#define tmwait(s)	waitmem (1, s)

/* ========================= */
/* End of TCV specific stuff */
/* ========================= */
#endif

#if	ENTROPY_COLLECTION
extern	lword zzz_ent_acc;
#define	add_entropy(w)	do { zzz_ent_acc = (zzz_ent_acc << 4) ^ (w); } while (0)
#define	entropy		zzz_ent_acc
#else
#define	add_entropy(w)	do { } while (0)
#define	entropy		0
#endif

/* Errors */
#define	ENODEV		1	/* Illegal device */
#define	ENOOPER		2	/* Illegal operation */
#define	EREQPAR		3	/* Illegal request parameters */
#define	ERESOURCE	4	/* Out of resources */
#define	ENEVENTS	5	/* Too many wait requests */
#define	EMALLOC		6	/* Out of heap memory */
#define	ESTATE		7	/* Illegal process state */
#define	EHARDWARE	8	/* Illegal process state */
#define	ETOOMANY	9	/* Too many times (like more than once) */
#define	EASSERT		10	/* Consistency check failed */
#define	ESTACK		11	/* Stack overrun */

#if	ADC_PRESENT
#if     CHIPCON
#error  "CHIPCON and ADC_PRESENT are incompatible"
#endif
/* === */
/* ADC */
/* === */
#define	ADC_MODE_TEMP		0
#define	ADC_MODE_VOLTAGE	1
#define	ADC_MODE_INTREF		2	/* Internal reference */
#define	ADC_MODE_OUTREF		3	/* Reference goes out */
#define	ADC_MODE_EXTREF		4	/* External reference */
#define	ADC_MODE_DIFFER		5	/* Differentials */

void	adc_start (int, int, int);
int	adc_read (int);
void	adc_stop (void);
#endif

/* ========================================== */
/* Registered identifiers of plugins and phys */
/* ========================================== */
#define	INFO_PLUG_NULL		0x0001	/* NULL plugin */
#define INFO_PLUG_TARP          0x0002  /* TARP plugin */

#define	INFO_PHYS_UART    	0x0100	/* Persistent UART */
#define	INFO_PHYS_ETHER    	0x0200	/* Raw Ethernet */
#define	INFO_PHYS_RADIO		0x0300	/* Radio */
#define INFO_PHYS_CHIPCON       0x0400  /* Chipcon radio */
#define	INFO_PHYS_DM2100	0x0500	/* DM2100 */

#endif //if SIM_NET==0

#endif 
