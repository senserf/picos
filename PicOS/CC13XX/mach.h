/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_mach_h
#define	__pg_mach_h		1

#include "portnames.h"

// Make sure these files are always compiled in
//+++ startup_gcc.c ccfg.c

#define	LITTLE_ENDIAN	1
#define	BIG_ENDIAN	0

// ============================================================================
// CPU type dependencies ======================================================
// ============================================================================

#ifdef	__cc1350__
#define	RAM_START	0x20000000
#define	RAM_SIZE	0x00005000
// Calibrate to microsecond
#define	__USEC_DELAY	4
#endif

// More CPU types to come?
#ifndef	__AUXIO_MAP
#define	__AUXIO_MAP	7	// 7x7 package, relevant for AUXIO mapping
#endif

#ifndef		RAM_SIZE
#error	"S: unknown (yet) CPU type: check CC13XX/mach.h"
#endif

#include "arch.h"
#include <stdarg.h>

// ============================================================================

#ifndef	USE_FLASH_CACHE
// By default the extra GP RAM is used for flash cache
#define	USE_FLASH_CACHE	1
#endif

// ###here: FLASH

// ###here: WATCHDOG
#define	WATCHDOG_STOP	CNOP
#define	WATCHDOG_START	CNOP
#define	WATCHDOG_CLEAR	CNOP
#define	WATCHDOG_HOLD	CNOP
#define	WATCHDOG_RESUME	CNOP

#define  watchdog_start()       WATCHDOG_START
#define  watchdog_stop()        WATCHDOG_STOP
#define  watchdog_clear()       WATCHDOG_CLEAR

#define	DEFAULT_PD_MODE	2

// This is the power down mode when radio is on; as it turns out, when system
// RAM is used for buffers and parameter, the CPU domain must be powered up
// (see Section 23.2, Doorbell). Radio RAM can be used, but there are tradeoffs
// complicating the power budget (Section 23.3.2.1), so perhaps the simplest
// solution is to keep it in RAM, also keeping the CPU on while the radio is
// active.

#ifndef	RADIO_WOR_MODE
#define	RADIO_WOR_MODE	0
#endif

#if RADIO_WOR_MODE
// Set to 1, seems to be the maximum that works, reset to 0 if it causes
// problems.
#define	RFCORE_PD_MODE	1
#else
// Without WOR, it makes sense to keep it at 0, as the current drain from the
// radio makes any savings on the CPU power irrelevant.
#define	RFCORE_PD_MODE	0
#endif


// ============================================================================

#define	RAM_END		(RAM_START + RAM_SIZE)
#define	STACK_START	((byte*)(RAM_END-0))	// FWA + 1 of stack
#define	STACK_END	(STACK_START - STACK_SIZE)

#define	STACK_SENTINEL	0xA778B779

#if STACK_GUARD
#define	check_stack_overflow \
			do { \
				if (*(((lword*)STACK_END)-1) != STACK_SENTINEL)\
					syserror (ESTACK, "st"); \
			} while (0)
#else
#define	check_stack_overflow	CNOP
#endif

// ###here: PORTMAPPER?

#define	cli			__disable_irq ()
#define	sti			__enable_irq ()

// ============================================================================

// Weird. I wasted two days before discovering that one NOP at the end of
// the loop is not enough. Are two?
#define	__SLEEP		do { \
				while (1) { \
					cli; \
					if (__pi_systat.evntpn) \
						break; \
					__do_wfi_as_needed (); \
					sti; \
					__NOP (); \
					__NOP (); \
				} \
				__pi_systat.evntpn = 0; \
				sti; \
			} while (0)

#define	RISE_N_SHINE	do { __pi_systat.evntpn = 1; } while (0)
#define	RTNI		return

// ============================================================================

// This isn't an option, and good riddance, too!
#define	TRIPLE_CLOCK	1

// ============================================================================
// The maximum "hold" second delay to carry out in a single go (converted to
// msec wait) + the maximum hold delay to split into fine 1 msec delays
// ============================================================================

#define	HOLD_BREAK_DELAY_SEC	63
#define	HOLD_STEP_DELAY_SEC	1

#define	TCI_MARK_AUXILIARY_TIMER_ACTIVE	aux_timer_inactive = 0
#define	TCI_RUN_DELAY_TIMER		tci_run_delay_timer ()
#define	TCI_RUN_AUXILIARY_TIMER		tci_run_auxiliary_timer ()
#define	TCI_UPDATE_DELAY_TICKS(f)	tci_update_delay_ticks (f)
#define	TCI_MAXDEL			((word)(65535))
// We use direct milliseconds
#define	TCI_DELTOTICKS(d)		(d)
// Convert milliseconds to timer increments; bit 16 (from left) is 1 sec, so
// bit 6 corresponds to 1/2^(-10) = 1 PicOS millisecond
#define	TCI_TINCR(d)			(((lword)(d)) << 6)
// Convert timer increments to milliseconds
#define	TCI_INCRT(d)			((word)((d) >> 6))

void tci_run_delay_timer ();
void tci_run_auxiliary_timer ();
word tci_update_delay_ticks (Boolean);

// Not 100% sure if we need to sync after clearing the RTC int in AON; I am
// trying to play it safe, especially that the STANDBY mode has been causing
// me serious problems, and I am far from convinced that it is OK now
#define	cli_tim		do { HWREGBITW (AON_RTC_BASE + AON_RTC_O_CHCTL, \
				AON_RTC_CHCTL_CH0_EN_BITN) = 0; \
				SysCtrlAonSync (); \
			} while (0)


#define	sti_tim		do { HWREGBITW (AON_RTC_BASE + AON_RTC_O_CHCTL, \
				AON_RTC_CHCTL_CH0_EN_BITN) = 1; \
			} while (0)

#define	cli_aux		do { HWREGBITW (AON_RTC_BASE + AON_RTC_O_CHCTL, \
				AON_RTC_CHCTL_CH2_EN_BITN) = 0; \
				SysCtrlAonSync (); \
			} while (0)

#define	sti_aux		do { HWREGBITW (AON_RTC_BASE + AON_RTC_O_CHCTL, \
				AON_RTC_CHCTL_CH2_EN_BITN) = 1; \
			} while (0)

#define	cli_utims	cli_aux
#define	sti_utims	sti_aux

// ============================================================================
// UART(s) ====================================================================
// ============================================================================

#include "uart_def.h"

// No other devices
#define	MAX_DEVICES	UART_DRIVER

#if	UART_DRIVER

typedef struct	{
	uint32_t	base;
	word		rate;
	volatile byte	flags;
} uart_t;

#if UART_DRIVER > 1
#error	"S: only one UART available on CC1350"
#endif

extern uart_t __pi_uart [];

#define	UART_BASE		UART_A

#define	UART_FLAGS_IN		0x80
#define	UART_FLAGS_OUT		0x40
#define	UART_FLAGS_NOTRANS	0x20

word __pi_uart_getrate (const uart_t*);

Boolean __pi_uart_setrate (word, uart_t*);

#endif	/* UART_DRIVER */

// MAP AUXIO to DIO
#if __AUXIO_MAP == 7
#define	DIO_TO_AUX(a)	(30-(a))
#define	AUX_TO_DIO(a)	(30-(a))
#endif

#if LEDS_DRIVER
#include "leds.h"
#endif

// The seconds clock
#define seconds()	AONRTCSecGet ()
#define	setseconds(a)	do { \
			    HWREG (AON_RTC_BASE + AON_RTC_O_SEC) = (lword)(a); \
			} while (0)

#define	powermode()	(__pi_systat . pdflags & 0x3)

extern void __pi_ondomain (lword), __pi_offdomain (lword);

// A function to go directly and unconditionally to SHUTDOWN, as an alternative
// to setpowermode (3), which delays the action to the nearest WFI
void hibernate () __attribute__((noreturn));

extern lword system_event_count;

#endif
