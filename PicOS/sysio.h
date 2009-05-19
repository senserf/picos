#ifndef __pg_sysio_h
#define	__pg_sysio_h		1

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
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

#define	SYSVER_X		((word)(SYSVER_U * 256 + SYSVER_L))
#define	SYSVER_S		stringify (SYSVER_U) "." stringify (SYSVER_L)
#define	SYSVER_R		stringify (SYSVER_T)

#ifdef	BOARD_TYPE
#define	SYSVER_B		stringify (BOARD_TYPE)
#endif

#define	__sgfy(a)		#a
#define	stringify(a)		__sgfy(a)

// ============================================================================

/* ================================================= */
/* Options are now settable on per-application basis */
/* ================================================= */

#ifndef SIM_NET
#define	SIM_NET		0
#endif

#if SIM_NET==0

#include "board_options.sys"
#include "options.sys"

#endif

//+++ "main.c"
//+++ "kernel.c"

#include "options.h"
#include "mach.h"

/* ================================== */
/* Some hard configuration parameters */
/* ================================== */
#ifdef	__MSP430__
// The io interface is not very popular in this version; we do not use io
// unless the old-fashioned UART interface is present (i.e., io is only ever
// used for the UART)
#define	MAX_DEVICES		UART_DRIVER
#else
#define	MAX_DEVICES		6
#endif

#define	MAX_MALLOC_WASTE	12

// Only needed if SCHED_PRIO != 0
#define MAX_PRIO                (MAX_TASKS * 10)

/* ======================================================================== */
/* This one is not easy to change because the loop in timer_int is unwound. */
/* So changing this will require you to modify timer_int by hand.           */
/* ======================================================================== */
#define MAX_UTIMERS		4

#if	UART_TCV

#if	UART_DRIVER
#error	"UART_DRIVER and UART_TCV are incompatible"
#endif

#else	/* NO UART_TCV */

#undef	UART_TCV_MODE

#endif	/* UART_TCV */

#if	UART_DRIVER > 2
#error	"UART_DRIVER can be 0, 1, or 2"
#endif

#if 	UART_BITS < 7 || UART_BITS > 8
#error "UART_BITS can be 7 or 8"
#endif

#if	RADIO_USE_LEDS
#undef	LEDS_DRIVER
#undef	LEDS_BLINKING
#define	LEDS_DRIVER	1
#define	LEDS_BLINKING	1
#endif

#if	LEDS_DRIVER == 0
#undef	LEDS_BLINKING
#define	LEDS_BLINKING	0
#endif

// LCD =======================================================================

#ifdef	LCD_PRESENT
#undef	LCD_PRESENT
#endif

#if LCD_ST7036
#define	LCD_PRESENT	1
//+++ lcd_st7036.c
#endif

#ifdef	LCDG_PRESENT
#undef	LCDG_PRESENT
#endif

#if LCDG_N6100P
#define	LCDG_PRESENT
//+++ lcdg_n6100p.c
#include "lcdg_n6100p.h"
#endif

// RTC =======================================================================

#ifdef	RTC_PRESENT
#undef	RTC_PRESENT
#endif

#if RTC_S35390
#define	RTC_PRESENT
//+++ rtc_s35390.c
#include "rtc.h"
#endif

// BLUETOOTH through UART ====================================================

#ifdef	BLUETOOTH_PRESENT
#undef	BLUETOOTH_PRESENT
#endif

#if BLUETOOTH_LM20
#define	BLUETOOTH_PRESENT	BLUETOOTH_LM20
#endif

#ifdef BLUETOOTH_PRESENT
#if BLUETOOTH_PRESENT > UART_TCV
#error "Bluetooth on serial requires UART_TCV >= BLUETOOTH_PRESENT"
#endif
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
#if	UART_TCV_MODE == UART_TCV_MODE_L
// This looks almost like direct UART; we can do diag this way
#define	DIAG_IMPLEMENTATION	0
#else
#if	UART_TCV_MODE == UART_TCV_MODE_N
// Non-persistent packets; we have a mode for that
#define	DIAG_IMPLEMENTATION	1
#endif
#endif
// Room for UART_TCV_MODE_P [later - this UART mode is likely to go]
#endif
#endif
// ===========================================================================

// LCD =======================================================================
#ifndef	DIAG_IMPLEMENTATION

#ifdef	LCD_PRESENT
#define	DIAG_IMPLEMENTATION	2
#endif

#endif
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

#ifdef	ZZ_RADIO_DRIVER_PRESENT
#error	"CC1000 cannot coexist with any other radio driver"
#else
#define	ZZ_RADIO_DRIVER_PRESENT	1
#endif

#define	ZZ_TCV_REQUIRED		1

//+++ "phys_cc1000.c"

#endif	/* CC1000 */

#if	CC1100

#ifdef	ZZ_RADIO_DRIVER_PRESENT
#error	"CC1100 cannot coexist with any other radio driver"
#else
#define	ZZ_RADIO_DRIVER_PRESENT	1
#endif

#define	ZZ_TCV_REQUIRED		1

//+++ "phys_cc1100.c"

#endif	/* CC1100 */

#if	RF24L01

#ifdef	ZZ_RADIO_DRIVER_PRESENT
#error	"RF24L01 cannot coexist with any other radio driver"
#else
#define	ZZ_RADIO_DRIVER_PRESENT	1
#endif

#define	ZZ_TCV_REQUIRED		1

//+++ "phys_rf24l01.c"

#endif	/* RF24L01 */


#if	DM2100

#ifdef	ZZ_RADIO_DRIVER_PRESENT
#error	"DM2100 cannot coexist with any other radio driver"
#else
#define	ZZ_RADIO_DRIVER_PRESENT	1
#endif

#define	ZZ_TCV_REQUIRED		1

//+++ "phys_dm2100.c"

#endif	/* DM2100 */

#if	DM2200

#ifdef	ZZ_RADIO_DRIVER_PRESENT
#error	"DM2200 cannot coexist with any other radio driver"
#else
#define	ZZ_RADIO_DRIVER_PRESENT	1
#endif

#define	ZZ_TCV_REQUIRED		1

//+++ "phys_dm2200.c"

#endif	/* DM2200 */

#if	RF24G

#ifdef	ZZ_RADIO_DRIVER_PRESENT
#error	"RF24G cannot coexist with any other radio driver"
#else
#define	ZZ_RADIO_DRIVER_PRESENT	1
#endif

#define	ZZ_TCV_REQUIRED		1

//+++ "phys_rf24g.c"

#endif	/* RF24G */


#if	RADIO_DRIVER

#ifdef	ZZ_RADIO_DRIVER_PRESENT
#error	"RADIO_DRIVER cannot coexist with any TCV-dependent radio device"
#else
#define	ZZ_RADIO_DRIVER_PRESENT	1
#endif

#endif	/* RADIO_DRIVER */

#ifdef	ZZ_RADIO_DRIVER_PRESENT

#if	RANDOM_NUMBER_GENERATOR == 0
#undef	RANDOM_NUMBER_GENERATOR
#define	RANDOM_NUMBER_GENERATOR	1
#endif

#endif	/* ZZ_RADIO_DRIVER_PRESENT */

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

#ifdef	ZZ_TCV_REQUIRED
#if	TCV_PRESENT == 0
#error	"TCV is required but has been explicitly removed from configuration"
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

// LCD =======================================================================

#ifdef	LCD_PRESENT

#define	LCD_CURSOR_ON		0x0001

void	lcd_on (word);
void	lcd_off ();
void	lcd_clear (word, word);
void	lcd_write (word, const char*);
void	lcd_putchar (char);
void	lcd_setp (word);

#define	LCD_N_CHARS	(LCD_LINE_LENGTH * LCD_N_LINES)

#else	/* LCD_PRESENT */

#define	lcd_on(a)	CNOP
#define	lcd_off()	CNOP
#define	lcd_clear(a,b)	CNOP
#define	lcd_write(a,b)	CNOP
#define	lcd_putchar(a)	CNOP

#endif	/* LCD_PRESENT */

// ===========================================================================

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

#if	BUTTONS_DRIVER

//+++ "buttons.c"

void	buttons_action (void (*action)(word));

#endif	/* BUTTONS_DRIVER */

#define	MAX_INT			((int)0x7fff)
#define	MAX_UINT		((word)0xffff)
#define	MAX_WORD		MAX_UINT
#define	MIN_INT			((int)0x8000)
#define	MIN_UINT		((word)0)
#define	MIN_WORD		MIN_UINT
#define	MAX_LONG		((long)0x7fffffffL)
#define	MIN_LONG		((long)0x80000000L)
#define MAX_ULONG		((lword)0xfffffffL)
#define	MAX_LWORD		MAX_ULONG
#define	MIN_ULONG		((lword)0)
#define	MIN_LWORD		MIN_ULONG

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
#define	SECONDS_IN_MINUTE	64


#define	NULL			0
#define	NONE			((word)(-1))
#define	LNONE			((lword)(0xffffffff))
#define	LWNONE			LNONE
#define	SNONE			((int)(-1))
#define	WNONE			NONE
#define	BNONE			0xff
#define	ERROR			NONE
#define	BLOCKED			((word)(-2))

#define	NO	((Boolean)0)
#define	YES	((Boolean)1)

#define	CNOP	do { } while (0)

#define	_BIS(a,b)	(a) |= (b)
#define	_BIC(a,b)	(a) &= ~(b)

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

// Access to word fragments by pointers byte in word, word in long, byte in long
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

#define	wsizeof(a)	((sizeof (a) + sizeof (word) - 1) / sizeof (word))

/* ============================================================ */
/* The main program (process) to be provided by the application */
/* ============================================================ */
int	root (word state, address data);

typedef	int (*code_t)(word, address);

void		zzz_uwait (word, word);
int		zzz_utrigger (word), zzz_ptrigger (int, word);
int		zzz_fork (code_t func, address data);
void		reset (void);
void		halt (void);

void		savedata (void*);

int		zzz_strlen (const char*);
void		zzz_strcpy (char*, const char*);
void		zzz_strncpy (char*, const char*, int);
void		zzz_strcat (char*, const char*);
void		zzz_strncat (char*, const char*, int);
void		zzz_memcpy (char *dest, const char *src, int);
void		zzz_memset (char *dest, char c, int);

extern 	const char	zz_hex_enc_table [];
#define	HEX_TO_ASCII(p)		(zz_hex_enc_table [(p) & 0xf])

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

#define	staticsize()	STATIC_LENGTH

#if	DIAG_MESSAGES > 1
void		zzz_syserror (int, const char*)
#ifdef	__MSP430__
		__attribute__ ((noreturn))
#endif
;

#define		syserror(a,b)	zzz_syserror (a, b)
#define		sysassert(a,b)	do { \
					if (!(a)) syserror (EASSERT, b); \
				} while (0)
#else
void		zzz_syserror (int)
#ifdef	__MSP430__
		__attribute__ ((noreturn))
#endif
;
#define		syserror(a,b)	zzz_syserror (a)
#define		sysassert(a,b)
#endif

#if	SDRAM_PRESENT
void		ramget (address, lword, int);
void		ramput (lword, address, int);
#endif

#if	DUMP_MEMORY
void		dmp_mem (void);
#endif

#if	LEDS_DRIVER

#define	LED_OFF		0
#define	LED_ON		1
#define	LED_BLINK	2

#define	leds(a,b)	do { \
				if ((b) == 0) { \
					if ((a) == 0) { \
						zz_systat.ledsts &= 0xe; \
						LED0_OFF; \
					} else if ((a) == 1) { \
						zz_systat.ledsts &= 0xd; \
						LED1_OFF; \
					} else if ((a) == 2) { \
						zz_systat.ledsts &= 0xb; \
						LED2_OFF; \
					} else if ((a) == 3) { \
						zz_systat.ledsts &= 0x7; \
						LED3_OFF; \
					} \
				} else if ((b) == 1) { \
					if ((a) == 0) { \
						zz_systat.ledsts &= 0xe; \
						LED0_ON; \
					} else if ((a) == 1) { \
						zz_systat.ledsts &= 0xd; \
						LED1_ON; \
					} else if ((a) == 2) { \
						zz_systat.ledsts &= 0xb; \
						LED2_ON; \
					} else if ((a) == 3) { \
						zz_systat.ledsts &= 0x7; \
						LED3_ON; \
					} \
				} else { \
					if ((a) == 0) { \
						zz_systat.ledsts |= 0x1; \
						LED0_ON; \
					} else if ((a) == 1) { \
						zz_systat.ledsts |= 0x2; \
						LED1_ON; \
					} else if ((a) == 2) { \
						zz_systat.ledsts |= 0x4; \
						LED2_ON; \
					} else if ((a) == 3) { \
						zz_systat.ledsts |= 0x8; \
						LED3_ON; \
					} \
				} \
			} while (0)

#define	fastblink(a)	(zz_systat.fstblk = ((a) != 0))
#define is_fastblink    (zz_systat.fstblk != 0)

#else

#define	leds(a,b)	do { } while (0)
#define	fastblink(a)	do { } while (0)
#define is_fastblink	0
	
#endif 	/* LEDS_DRIVER */

void	diag (const char *, ...);

#if	SWITCHES
word	switches (void);
#else
#define	switches()	0
#endif

#if MAX_DEVICES
/* ======================================= */
/* This is the common i/o request function */
/* ======================================= */
int	io (int, int, int, char*, int);
#define	ion(a,b,c,d)	io (NONE, a, b, c, d)
#endif	/* MAX_DEVICES */

/* User wait */
#define	wait(a,b)	zzz_uwait ((word)(a),b)
/* A prefered alias */
#define	when(a,b)	wait (a,b)

/* Timer wait */
void	delay (word, word);
word	dleft (int);

/* Minute wait */
void	ldelay (word, word);
word	ldleft (int, address);

/* Continue timer wait */
void	snooze (word);
/* Signal trigger: returns the number of awakened processes */
#define	trigger(a)	zzz_utrigger ((word)(a))
#define	ptrigger(a,b)	zzz_ptrigger (a, (word)(b))
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
int	running (code_t), crunning (code_t);
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
int	utimer (address, Boolean);

/* Second clock */
#ifdef	__ECOG1__
lword	seconds (void);
word 	sectomin (void);
#else
#define	seconds()	zz_nseconds
#define	sectomin()	((word)zz_mincd)
extern	byte		zz_mincd;
#endif

extern	lword		zz_nseconds;

#define	setseconds(a)	(zz_nseconds = (lword) (a));

/* Spin delay */
void	udelay (word);
void	mdelay (word);

#if	GLACIER
void	freeze (word);
#endif

#if	RADIO_INTERRUPTS
Boolean	rcvwait (word);
void	rcvcancel (void);
int	rcvlast (void);
#endif

#define	call(p,d,s)	do { join (fork (p, d), s); release; } while (0)

#define	finish		kill (0)
#define	hang		kill (-1)
#define	fork(p,d)	zzz_fork (p, (address)(d))
#define	find(p,d)	zzz_find (p, (address)(d))
#define	iszombie(p)	(status (p) == -1)
#define	getcpid()	running (NULL)
#define	ptleft()	crunning (NULL)

#define	heapmem		const word zzz_heap [] =

#define	strlen(s)	zzz_strlen (s)
#define	strcpy(a,b)	zzz_strcpy (a, b)
#define	strncpy(a,b,c)	zzz_strncpy (a, b, c)
#define	strcat(a,b) 	zzz_strcat (a, b)
#define	strncat(a,b,c)	zzz_strncat (a, b, c)
#define	memcpy(a,b,c)	zzz_memcpy ((char*)(a), (const char*)(b), c)
#define	memset(a,b,c)	zzz_memset ((char*)(a), (char)(b), c)
#define	bzero(a,c)	memset ((char*)(a), 0, c)

/* User malloc shortcut */
#define	umalloc(s)	malloc (0, s)
#define	ufree(p)	free (0, p)
#define umwait(s)	waitmem (0, s)
/* Availability of a free process table entry */
#define	npwait(s)	waitmem (0, s)
/* Actual size of an malloc'ed piece */
#define	actsize(p)	(*(((word*)(p))-1) << 1)

void zz_badstate (void);

/* Process operations */
#define	process(p,d)	int p (word zz_st, address zz_da) { \
				d *data = (d*) zz_da; \
				switch (zz_st) {

#define	strand(a,b)	process (a, b)

#define	thread(p)	int p (word zz_st, address zz_dummy) { \
				switch (zz_st) {

#define	endprocess(n)			break; \
				    default: \
					if (zz_st == 0xffff) return (n); \
					zz_badstate (); \
				} return 1; }

#define	endthread	endprocess (1)
#define	endstrand	endprocess (0)

#define	entry(s)	case s:

#define	procname(p)	extern int p (word, address)
#define	sprocname(p)	static int p (word, address)

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

#define	UART_CNTRL_LCK		1	/* UART lock/unlock */
#define	UART_CNTRL_SETRATE	2
#define	UART_CNTRL_GETRATE	3
#define	UART_CNTRL_CALIBRATE	4	/* For UARTs driven by flimsy clocks */

#define	LCD_CNTRL_POS		1	/* Position (SEEK) */
#define	LCD_CNTRL_ERASE		2	/* Clear */

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

#define	PHYSOPT_RESET		25	/* Reset the radio */

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
int	tcv_plug (int, const tcvplug_t*);
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


#if	RANDOM_NUMBER_GENERATOR

#if	RANDOM_NUMBER_GENERATOR > 1
/* High quality */
extern	lword zz_seed;

#define	rnd()	(zz_seed = (1103515245 * zz_seed + 12345 + entropy) & 0x7fffff,\
			(word) zz_seed)

#else	/* RANDOM_NUMBER_GENERATOR == 1 */
/* Low quality */
extern	word zz_seed;

#define	rnd()	(zz_seed = (zz_seed + (word) entropy + 1) * 12345, zz_seed)

#endif	/* RANDOM_NUMBER_GENERATOR == 1 */

#endif	/* RANDOM_NUMBER_GENERATOR */

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
#define	ENOTNOW		15	/* Operation illegal at this time */

#if	ADC_PRESENT
#if     CC1000
#error  "CC1000 and ADC_PRESENT are incompatible"
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

#define	INFO_PHYS_UART    	0x0100	/* Non-persistent UART */
#define	INFO_PHYS_UARTB    	0x1100	/* Non-persistent UART + BlueTooth */
#define	INFO_PHYS_UARTP		0x4100	/* Persistent UART */
#define	INFO_PHYS_UARTL		0x8100	/* Line-mode UART over TCV */
#define	INFO_PHYS_UARTLB	0x9100	/* Line-mode UART + BlueTooth */
#define	INFO_PHYS_ETHER    	0x0200	/* Raw Ethernet */
#define	INFO_PHYS_RADIO		0x0300	/* Radio */
#define INFO_PHYS_CC1000        0x0400  /* CC1000 radio */
#define	INFO_PHYS_DM2100	0x0500	/* DM2100 */
#define	INFO_PHYS_CC1100	0x0600	/* CC1100 */
#define	INFO_PHYS_DM2200	0x0700  /* VERSA 2 */
#define	INFO_PHYS_RF24L01	0x0800

#endif //if SIM_NET==0

/* ======================================== */
/* Tools for isolating SMURPH-specific code */
/* ======================================== */

#define	_da(a)		a
#define	_dac(a,b)	b
#define	_dad(t,a)	a

// Should be something redundant and empty; cannot be literaly empty as the
// stupid CYAN compiler won't take it
#define	praxis_starter(a)	int kill (int)

#define	__STATIC	static
#define	__CONST		const
#define	__EXTERN	extern
#define	__VIRTUAL
#define	__ABSTRACT

#define	__PRIVF(ot,tp,nam)	static tp nam
#define	__PUBLF(ot,tp,nam)	tp nam
#define	__PUBLS(ot,tp,nam)	__PRIVF (ot, tp, nam)

#define	__PROCESS(a,b)	process (a, b)
#define	__ENDPROCESS(a)	endprocess (a)
#define	__NA(a,b)	(b)

// A few symbolic state ordinals
#define	__S0		0
#define	__S1		1
#define	__S2		2
#define	__S3		3
#define	__S4		4

#endif 
