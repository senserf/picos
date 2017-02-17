#ifndef __pg_mach_h
#define	__pg_mach_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2017                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "portnames.h"
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

#ifndef		RAM_SIZE
#error	"S: unknown (yet) CPU type: check CC13XX/mach.h"
#endif

#include "arch.h"
#include <stdarg.h>

// ============================================================================

// ###here: FLASH

// ###here: WATCHDOG
#define	WATCHDOG_STOP	CNOP
#define	WATCHDOG_START	CNOP
#define	WATCHDOG_CLEAR	CNOP
#define	WATCHDOG_HOLD	CNOP
#define	WATCHDOG_RESUME	CNOP

#define  watchdog_start()        WATCHDOG_START
#define  watchdog_stop()         WATCHDOG_STOP
#define  watchdog_clear()        WATCHDOG_CLEAR

// ============================================================================

#define	RAM_END		(RAM_START + RAM_SIZE)
#define	STACK_START	((byte*)RAM_END)	// FWA + 1 of stack
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

// PD mode is entered elsewhere and need not be sustained on every sleep
// Weird. I wasted two days before discovering that one NOP at the end of
// the loop is not enough. Are two?
#define	__SLEEP		do { \
				CPU_MARK_IDLE; \
				while (1) { \
					cli; \
					if (__pi_systat.evntpn) \
						break; \
					__WFI (); \
					sti; \
					__NOP (); \
					__NOP (); \
				} \
				__pi_systat.evntpn = 0; \
				CPU_MARK_BUSY; \
				sti; \
			} while (0)
// ###here: check_stack_overflow (no seconds clock)

#define	RISE_N_SHINE	do { __pi_systat.evntpn = 1; } while (0)
#define	RTNI		return

// ============================================================================

#define	TRIPLE_CLOCK	1

#define	TCI_MARK_AUXILIARY_TIMER_ACTIVE	aux_timer_inactive = 0
#define	TCI_RUN_DELAY_TIMER		tci_run_delay_timer ()
#define	TCI_RUN_AUXILIARY_TIMER		tci_run_auxiliary_timer ()
#define	TCI_UPDATE_DELAY_TICKS(f)	tci_update_delay_ticks (f)
// One half of the range
#define	TCI_MAXDEL			(((word)(65535)) >> 1)
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

#define	cli_tim		HWREGBITW (AON_RTC_BASE + AON_RTC_O_CHCTL, \
				AON_RTC_CHCTL_CH0_EN_BITN) = 0
#define	sti_tim		HWREGBITW (AON_RTC_BASE + AON_RTC_O_CHCTL, \
				AON_RTC_CHCTL_CH0_EN_BITN) = 1
#define	cli_aux		HWREGBITW (AON_RTC_BASE + AON_RTC_O_CHCTL, \
				AON_RTC_CHCTL_CH2_EN_BITN) = 0;
#define	sti_aux		HWREGBITW (AON_RTC_BASE + AON_RTC_O_CHCTL, \
				AON_RTC_CHCTL_CH2_EN_BITN) = 1;

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

// ###here: ADC?

#ifdef	MONITOR_PIN_CPU

#define	CPU_MARK_IDLE	_PVS (MONITOR_PIN_CPU, 0)
#define	CPU_MARK_BUSY	_PVS (MONITOR_PIN_CPU, 1)

#else

#define	CPU_MARK_IDLE	CNOP
#define	CPU_MARK_BUSY	CNOP

#endif

#if LEDS_DRIVER
#include "leds.h"
#endif

// The seconds clock
// ###here: need sync to make sure the value is correctly read after sleep?
// ###here: generally, when do we need to sync? driverlib doesn't seem to
#define seconds()	AONRTCSecGet ()
#define	setseconds(a)	do { \
			    HWREG (AON_RTC_BASE + AON_RTC_O_SEC) = (lword)(a); \
			} while (0)

#endif
