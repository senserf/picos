#ifndef __pg_sysio_h
#define	__pg_sysio_h		1

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2017                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// These are set by mkmk ======================================================

#ifndef	SYSVER_U
#define	SYSVER_U	0	// Upper (major) version number
#endif

#ifndef	SYSVER_L
#define	SYSVER_L	0	// Lower (minor) version number
#endif

#ifndef	SYSVER_T
#define	SYSVER_T	unknown	// Release tag
#endif

#define	SYSVER_X	((word)(SYSVER_U * 256 + SYSVER_L))
#define	SYSVER_S	stringify (SYSVER_U) "." stringify (SYSVER_L)
#define	SYSVER_R	stringify (SYSVER_T)

// ============================================================================

/* ================================================= */
/* Options are now settable on per-application basis */
/* ================================================= */

#ifndef SIM_NET
#define	SIM_NET		0
#endif

#if SIM_NET==0

#include "board_options.sys"

#ifdef	__OPTIONS_SYS__
#include __OPTIONS_SYS__
#endif

#endif

//+++ "main.c"
//+++ "kernel.c"

#include "modsyms.h"
#include "mach.h"

#if	RADIO_USE_LEDS
#undef	LEDS_DRIVER
#define	LEDS_DRIVER	1
#endif

#define	__NORETURN__		__attribute__ ((noreturn))

/* ======================================================================== */
/* This one is not easy to change because the loop in timer_int is unwound. */
/* So changing this will require you to modify timer_int by hand.           */
/* ======================================================================== */
#define MAX_UTIMERS		4

#if	UART_TCV

#if	UART_DRIVER
#error	"S: UART_DRIVER and UART_TCV are incompatible"
#endif

#else	/* NO UART_TCV */

#undef	UART_TCV_MODE

#endif	/* UART_TCV */

#if	UART_DRIVER > 2
#error	"S: UART_DRIVER can be 0, 1, or 2"
#endif

#if	LEDS_DRIVER == 0
#undef	LEDS_BLINKING
#define	LEDS_BLINKING	0
#endif

// DIAG MESSAGES =============================================================

// If DIAG_MESSAGES is not set, there is no need to worry; otherwise, we have
// to figure out how diag is going to work

#if	DIAG_MESSAGES || (dbg_level != 0)

// We go through a number of options

// UART ======================================================================
#ifndef	DIAG_IMPLEMENTATION

#if UART_DRIVER
#define	DIAG_IMPLEMENTATION	0
#endif

#endif
// ===========================================================================

// UART over TCV =============================================================
#ifndef	DIAG_IMPLEMENTATION

#if	UART_TCV
#define	DIAG_IMPLEMENTATION	1
#endif

#endif
// ===========================================================================
// DIAG_IMPLEMENTATION == 2 must be explicit (requires definitions of
// lcd_diag_start, lcd_diag_wchar(c), lcd_diag_wait, lcd_diag_stop
// ===========================================================================

#ifndef	DIAG_IMPLEMENTATION
// We haven't been able to find an implementation for diag, deactivate it
#undef	DIAG_MESSAGES
#undef	dbg_level
#define	DIAG_MESSAGES		0
#define	dbg_level		0
#endif

#endif	/* DIAG_MESSAGES */

// ===========================================================================

#include "diag_sys.h"
#include "dbgtrc.h"
#include "board_headers.h"

#if SIM_NET==0

#if	CC1000

#ifdef	__pi_RADIO_DRIVER_PRESENT
#error	"S: CC1000 cannot coexist with any other radio driver"
#else
#define	__pi_RADIO_DRIVER_PRESENT	1
#endif

#define	__pi_TCV_REQUIRED		1

//+++ "phys_cc1000.c"

#endif	/* CC1000 */

#if	CC1100

#ifdef	__pi_RADIO_DRIVER_PRESENT
#error	"S: CC1100 cannot coexist with any other radio driver"
#else
#define	__pi_RADIO_DRIVER_PRESENT	1
#endif

#define	__pi_TCV_REQUIRED		1

//+++ "phys_cc1100.c"

#endif	/* CC1100 */

#if	RF24L01

#ifdef	__pi_RADIO_DRIVER_PRESENT
#error	"S: RF24L01 cannot coexist with any other radio driver"
#else
#define	__pi_RADIO_DRIVER_PRESENT	1
#endif

#define	__pi_TCV_REQUIRED		1

//+++ "phys_rf24l01.c"

#endif	/* RF24L01 */


#if	DM2100

#ifdef	__pi_RADIO_DRIVER_PRESENT
#error	"S: DM2100 cannot coexist with any other radio driver"
#else
#define	__pi_RADIO_DRIVER_PRESENT	1
#endif

#define	__pi_TCV_REQUIRED		1

//+++ "phys_dm2100.c"

#endif	/* DM2100 */

#if	DM2200

#ifdef	__pi_RADIO_DRIVER_PRESENT
#error	"S: DM2200 cannot coexist with any other radio driver"
#else
#define	__pi_RADIO_DRIVER_PRESENT	1
#endif

#define	__pi_TCV_REQUIRED		1

//+++ "phys_dm2200.c"

#endif	/* DM2200 */

#if	RF24G

#ifdef	__pi_RADIO_DRIVER_PRESENT
#error	"S: RF24G cannot coexist with any other radio driver"
#else
#define	__pi_RADIO_DRIVER_PRESENT	1
#endif

#define	__pi_TCV_REQUIRED		1

//+++ "phys_rf24g.c"

#endif	/* RF24G */

#ifdef	__pi_RADIO_DRIVER_PRESENT

#if	RANDOM_NUMBER_GENERATOR == 0
#undef	RANDOM_NUMBER_GENERATOR
#define	RANDOM_NUMBER_GENERATOR	1
#endif

#endif	/* __pi_RADIO_DRIVER_PRESENT */

#if	CC3000
#ifndef	__pi_TCV_REQUIRED
#define	__pi_TCV_REQUIRED		1
#endif
#endif

#ifdef	__pi_TCV_REQUIRED
#if	TCV_PRESENT == 0
#error	"S: TCV is required but has been explicitly removed from configuration"
#endif
#endif

// External storage ==========================================================

#ifdef	EEPROM_PRESENT
#undef	EEPROM_PRESENT
#endif

#ifdef	SDCARD_PRESENT
#undef	SDCARD_PRESENT
#endif

#if STORAGE_M95XXX
#define	EEPROM_PRESENT	1
//+++ "storage_m95xxx.c"
#endif

#if STORAGE_AT45XXX
#define	EEPROM_PRESENT	1
//+++ "storage_at45xxx.c"
#endif

#if STORAGE_MT29XXX
#define	EEPROM_PRESENT	1
//+++ "storage_mt29xxx.c"
#endif

#if STORAGE_SDCARD
#define	SDCARD_PRESENT	1
//+++ "sdcard.c"
#endif

#if	INFO_FLASH
//+++ "iflash.c"
#endif	/* INFO_FLASH */

#if	ADC_SAMPLER

//+++ "adc_sampler.c" "adc_sampler_sys.c"

word	adcs_start (word);
void	adcs_stop ();
Boolean	adcs_get_sample (word, address);
word	adcs_overflow ();

#endif	/* ADC_SAMPLER */

/* ============================ */
/* Device identifiers (numbers) */
/* ============================ */
#define	UART_A			0
#define	UART_B			1
#define	UART			UART_A

// The ones below are anachronisms
#define	LCD			2
#define	LEDS			3
#define	ETHERNET		4
#define	RADIO			5

/* ========================================= */
/* The number of clock interrupts per second */
/* ========================================= */
#define	JIFFIES			1024	/* Clock ticks in a second           */
#define	SECONDS_IN_MINUTE	64


#define	_BIS(a,b)	(a) |= (b)
#define	_BIC(a,b)	(a) &= ~(b)
#define	_BIX(a,b)	(a) ^= (b)

/* ============ */
/* Tricky casts */
/* ============ */
#define	ptrtoint(a)	((int)(a))	// Pointer to integer

/* ============= */
/* Byte ordering */
/* ============= */
#if	LITTLE_ENDIAN

#define	ntohs(w)	((((w)&0xff)<<8)|(((w)>>8)&0xff))
#define ntohl(w)	((((w)&0xff)<<24)|(((w)&0xff00)<<8)|\
				(((w)>>8)&0xff00)|(((w)>>24)&0xff))
#define	ntowl(w)	((((w) & 0xffff) << 16) | (((w) >> 16) & 0xffff))

// Access to word fragments by pointers: byte in word, word in long,
// byte in long
#define	bytepw(n,w)	(((byte*)(&(w)))+(n))
#define	wordpl(n,w)	(((word*)(&(w)))+(n))
#define	bytepl(n,w)	(((byte*)(&(w)))+(n))

#else

#define	ntohs(w)	(w)
#define ntohl(w)	(w)
#define ntowl(w)	(w)

// Access to word fragments by pointers (for constant n, it will compile out)
#define	bytepw(n,w)	(((byte*)(&(w)))+(1-(n)))
#define	wordpl(n,w)	(((word*)(&(w)))+(1-(n)))
#define	bytepl(n,w)	(((byte*)(&(w)))+(3-(n)))

#endif	/* LITTLE_ENDIAN */

#define	htons(w)	ntohs (w)
#define	htonl(w)	ntohl (w)
#define	wtonl(w)	ntowl (w)

/* ============================================================ */
/* The main program (process) to be provided by the application */
/* ============================================================ */
void	root (word state);

typedef	void (*fsmcode)(word);

void		__pi_wait (aword, word);
void		__pi_trigger (aword), __pi_ptrigger (aword, aword);
aword		__pi_fork (fsmcode func, aword data);
void		__pi_fork_join_release (fsmcode func, aword data, word st);
void		reset (void) __NORETURN__ ;
void		halt (void) __NORETURN__ ;

int		__pi_strlen (const char*);
void		__pi_strcpy (char*, const char*);
void		__pi_strncpy (char*, const char*, int);
void		__pi_strcat (char*, const char*);
void		__pi_strncat (char*, const char*, int);
void		__pi_memcpy (char *dest, const char *src, int);
void		__pi_memset (char *dest, char c, int);

extern 	const char		 __pi_hex_enc_table [];
#define	HEX_TO_ASCII(p)		(__pi_hex_enc_table [(p) & 0xf])

extern	const lword	host_id;

#if	MALLOC_NPOOLS < 2

aword			*__pi_malloc (word);	// size in bytes up to 64K-1
void			__pi_free (aword*);
void			__pi_waitmem (word);	// state

#if	MALLOC_STATS
word			__pi_memfree (address);	// word total free
word			__pi_maxfree (address);	// max free chunk size
#define	memfree(p,s)	__pi_memfree (s)
#define	maxfree(p,s)	__pi_maxfree (s)
#endif

#define	free(p,s)	__pi_free ((aword*)(s))
#define	malloc(p,s)	((address)__pi_malloc (s))
#define	waitmem(p,t)	__pi_waitmem (t)

#else	// Multiple pools

aword			*__pi_malloc (sint, word);	// pool, size
void			__pi_free (sint, aword*);	// pool, pointer
void			__pi_waitmem (sint, word);
#if	MALLOC_STATS
word			__pi_memfree (sint, address);
word			__pi_maxfree (sint, address);
#define	memfree(p,s)	__pi_memfree (p, s)
#define	maxfree(p,s)	__pi_maxfree (p, s)
#endif
#define	free(p,s)	__pi_free (p, (aword*)(s))
#define	malloc(p,s)	((address)__pi_malloc (p,s))
#define	waitmem(p,t)	__pi_waitmem (p, t)

#endif	// MALLOC_NPOOLS

#if	STACK_GUARD
word			__pi_stackfree (void);
#define	stackfree()	__pi_stackfree ()
#endif

#define	staticsize()	STATIC_LENGTH

#if	DIAG_MESSAGES > 1

void		__pi_syserror (word, const char*) __NORETURN__ ;
#define		syserror(a,b)	__pi_syserror (a, b)
#define		sysassert(a,b)	do { \
					if (!(a)) syserror (EASSERT, b); \
				} while (0)
#else

void		__pi_syserror (word) __NORETURN__ ;
#define		syserror(a,b)	__pi_syserror (a)
#define		sysassert(a,b)	CNOP
#endif

#if	DUMP_MEMORY
void		dmp_mem (void);
#endif

// ============================================================================

#if	LEDS_DRIVER

#define	LED_OFF		0
#define	LED_ON		1
#define	LED_BLINK	2

// ============================================================================

#ifndef	leds

// Board files can override the operation

#define	leds(a,b)	do { \
				if ((b) == 0) { \
					if ((a) == 0) { \
						led0_off; \
					} else if ((a) == 1) { \
						led1_off; \
					} else if ((a) == 2) { \
						led2_off; \
					} else if ((a) == 3) { \
						led3_off; \
					} \
				} else if ((b) == 1) { \
					if ((a) == 0) { \
						led0_on; \
					} else if ((a) == 1) { \
						led1_on; \
					} else if ((a) == 2) { \
						led2_on; \
					} else if ((a) == 3) { \
						led3_on; \
					} \
				} else { \
					if ((a) == 0) { \
						led0_blk; \
					} else if ((a) == 1) { \
						led1_blk; \
					} else if ((a) == 2) { \
						led2_blk; \
					} else if ((a) == 3) { \
						led3_blk; \
					} \
					TCI_RUN_AUXILIARY_TIMER; \
				} \
			} while (0)

#define	leds_all(b)	do { \
				if ((b) == 0) { \
					leds_off; \
				} else if ((b) == 1) { \
					leds_on; \
				} else { \
					leds_blk; \
					TCI_RUN_AUXILIARY_TIMER; \
				} \
			} while (0)
						
#define	fastblink(a)	(__pi_systat.fstblk = ((a) != 0))
#define fastblinking()  (__pi_systat.fstblk != 0)

#define	all_leds_blink	do { leds_on; mdelay (200); leds_off; mdelay (200); } \
				while (0)

#endif	/* leds [overriden from BOARD files] */

// ============================================================================

#else	/* No LEDS_DRIVER */

#ifdef	leds
#undef	leds
#endif

#ifdef	leds_all
#undef	leds_all
#endif

#ifdef	fastblink
#undef	fastblink
#endif

#ifdef	fastblinking
#undef	fastblinking
#endif

#define	leds(a,b)	CNOP
#define leds_all(b)	CNOP
#define	fastblink(a)	CNOP
#define is_fastblink	0

#define	all_leds_blink	CNOP
	
#endif 	/* LEDS_DRIVER */

// ============================================================================

#if DIAG_MESSAGES || defined(__ECOG1__)
void	diag (const char *, ...);
#else
#define	diag(...)	CNOP
#endif

#if RANDOM_NUMBER_GENERATOR > 1
// ###here: will have to provide a hardware option
// High-quality rnd
lword lrnd (void);
#define	rnd()	((word)(lrnd () >> 16))
#endif

#if RANDOM_NUMBER_GENERATOR == 1
// Low-quality rnd
word rnd (void);
#define lrnd()	(((lword)rnd ()) << 16 | rnd ())
#endif

#if	SWITCHES
word	switches (void);
#else
#define	switches()	0
#endif

#if MAX_DEVICES
/* ======================================= */
/* This is the common i/o request function */
/* ======================================= */
sint	io (word, word, word, char*, word);
#define	ion(a,b,c,d)	io (NONE, a, b, c, d)
#endif	/* MAX_DEVICES */

/* User wait */
#define	wait(a,b)	__pi_wait ((aword)(a),b)
/* A prefered alias */
#define	when(a,b)	wait (a,b)

void unwait (void);

/* Timer wait */
void	delay (word, word);
word	dleft (aword);

/* Signal trigger: returns the number of awakened processes */
#define	trigger(a)	__pi_trigger ((aword)(a))
#define	ptrigger(a,b)	__pi_ptrigger (a, (aword)(b))
/* Kill the indicated process */
void	kill (aword);
/* Kill all processes running this code */
void	killall (fsmcode);

/* Wait for a process */
#define	join(p,s)	when (p, s)
/* Wait for any process running this code */
#define	joinall(f,s)	when (f, s)
/* Locate a process by code */
aword	running (fsmcode);
word	crunning (fsmcode);
/* Transform pid into code pointer */
fsmcode getcode (aword);
/* Proceed to another state */
void	proceed (word);
/* Power up/down functions */
void	setpowermode (word);
#define	powerup() 	setpowermode (0)
#define	powerdown()	setpowermode (DEFAULT_PD_MODE)
/* User timers */
void	utimer_add (address), utimer_delete (address);

#if TRIPLE_CLOCK
void __pi_utimer_set (address, word);
#define	utimer_set(a,v)	__pi_utimer_set (&(a), v)
#else
#define	utimer_set(a,v)	((a) = v)
#endif

/* Spin delay */
void	udelay (word);
void	mdelay (word);

#if	GLACIER
void	freeze (word);
#endif

// FIXME: to be generated by picomp
#define	call(p,d,s)	do { join (fork (p, d), s); release; } while (0)

#define	finish		kill (0)
#define	fork(p,d)	__pi_fork (p, (aword)(d))
#define	forkjr(p,d,s)	__pi_fork_join_release (p, (aword)(d), s)
#define	getcpid()	running (NULL)

#define	release		__pi_release ()

// Pool percentages
#define	heapmem		const byte __pi_heap [] =

#define	strlen(s)	__pi_strlen (s)
#define	strcpy(a,b)	__pi_strcpy (a, b)
#define	strncpy(a,b,c)	__pi_strncpy (a, b, c)
#define	strcat(a,b) 	__pi_strcat (a, b)
#define	strncat(a,b,c)	__pi_strncat (a, b, c)
#define	memcpy(a,b,c)	__pi_memcpy ((char*)(a), (const char*)(b), c)
#define	memset(a,b,c)	__pi_memset ((char*)(a), (char)(b), c)
#define	bzero(a,c)	memset ((char*)(a), 0, c)

/* User malloc shortcut */
#define	umalloc(s)	malloc (0, s)
#define	ufree(p)	free (0, p)
#define umwait(s)	waitmem (0, s)
/* Availability of a free process table entry */
#define	npwait(s)	waitmem (0, s)

/* Actual size of an malloc'ed piece */
#if SIZE_OF_AWORD > 2
#define	actsize(p)	(*(((aword*)(p))-1) << 2)
#else
#define	actsize(p)	(*(((aword*)(p))-1) << 1)
#endif

void __pi_badstate (void);

// Note: data in a picomp-issue thread will be a macro
#define	process(p,d)	void p (word __pi_st) { \
				d data = (d) __pi_curr->data; \
				switch (__pi_st) {

#define	savedata(a)	(__pi_curr->data = (aword)(a))

#define	strand(a,b)	process (a, b)

#define	thread(p)	void p (word __pi_st) { switch (__pi_st) {

#define	endprocess	break; default: __pi_badstate (); } }

#define	endthread	endprocess
#define	endstrand	endprocess

#define	entry(s)	case s:

#define	procname(p)	void p (word)
#define	sprocname(p)	void p (word)

#define	runthread(a)	fork (a, NULL)
#define	runstrand(a,b)	fork (a, b)

#ifdef	DEBUG_BUFFER
void	dbb (word);
#else
#define	dbb(a)
#endif

/* I/O operations */
#define	READ		0
#define	WRITE		1
#define	CONTROL		2

// None of these can be zero
#define	UART_CNTRL_SETRATE	1
#define	UART_CNTRL_GETRATE	2
#define	UART_CNTRL_TRANSPARENT	3

/* ============================================= */
/* TCV specific stuff to be visible by everybody */
/* ============================================= */

#if	TCV_PRESENT

#include "tcv_defs.h"

/* TCV malloc shortcut */
#define	tmalloc(s)	malloc (1, s)
#define	tfree(s)	free (1, s)
#define tmwait(s)	waitmem (1, s)

/* ========================= */
/* End of TCV specific stuff */
/* ========================= */
#endif

#if	ENTROPY_COLLECTION
extern	lword entropy;
#define	add_entropy(w)	do { entropy = (entropy << 4) ^ (w); } while (0)
#else
#define	add_entropy(w)	do { } while (0)
#define	entropy		0
#endif

#endif // if SIM_NET==0

// Tools for isolating SMURPH-specific code from VUEE

#define	_da(a)		a
#define	_dac(a,b)	b
#define	_dad(t,a)	a

// Should be something redundant and empty
#define	praxis_starter(a)

#define	__STATIC	static
#define	__CONST		const
#define	__EXTERN	extern
#define	__VIRTUAL
#define	__ABSTRACT

#define	trueconst	const
#define	idiosyncratic
#define nonidiosyncratic

#define	PREINIT(a,b)	(a)

#define	vuee_control(a,...)		CNOP
#define	highlight_set(a,b,c,...)	CNOP
#define	highlight_clear()		CNOP
#define	__sinit(...)		= { __VA_ARGS__ }

#define	__PRIVF(ot,tp,nam)	static tp nam
#define	__PUBLF(ot,tp,nam)	tp nam
#define	__PUBLS(ot,tp,nam)	__PRIVF (ot, tp, nam)

// ============================================================================
// PCB layout =================================================================
// ============================================================================

typedef struct	{
/* =================================== */
/* A single event awaited by a process */
/* =================================== */
	word	State;
	aword	Event;
} __pi_event_t;

struct __pi_pcb_s {
	/* ============================================================== */
	/* This is the PCB. Status consists of two parts. The three least */
	/* significant bits store the number of awaited events except for */
	/* the Timer delay, and the fourth bit is set if a Timer event is */
	/* being awaited.  The remaining (upper 12) bits encode the state */
	/* to be assumed when the Timer goes off. Also, if the process is */
	/* ready to go, those bits encode the process's current state.    */
	/* ============================================================== */
	word		Status;
	word		Timer;		/* Timer wakeup tick */
	fsmcode		code;		/* Code function pointer */
	aword		data;		/* Data pointer */
	__pi_event_t	Events [MAX_EVENTS_PER_TASK];
	struct __pi_pcb_s	*Next;
};

typedef struct __pi_pcb_s __pi_pcb_t;

extern __pi_pcb_t *__pi_curr;

#endif 
