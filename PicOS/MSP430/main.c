/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "kernel.h"
#include "storage.h"


#include "irq_timer_headers.h"

extern 	__pi_pcb_t	*__pi_curr;
extern 	address		__pi_utims [MAX_UTIMERS];

lword 	__pi_nseconds;

void	__pi_malloc_init (void);

/* ========================== */
/* Device driver initializers */
/* ========================== */

#if	UART_DRIVER || UART_TCV

static void preinit_uart (void);

#if	UART_DRIVER

#define	N_UARTS	UART_DRIVER

static void	devinit_uart (int);

#else	/* UART_DRIVER */

#define	N_UARTS	UART_TCV

#endif	/* UART_DRIVER */

// ============================================================================
// This is clumsy. We need a way to make the second UART configurable
// dynamically, as it can be shared with SPI, so we need it as a formal
// device, but we don't want the preinit, freeze, auto enable/disable, etc.
// ============================================================================

#if N_UARTS < 2
// This is defined in mach.h, but can be undef'ed, e.g., from options.sys
#ifdef	UART_PREINIT_B
#undef	UART_PREINIT_B
#endif
#endif

// ============================================================================

#endif  /* UART_DRIVER || UART_TCV */

#if MAX_DEVICES

// On MSP430, UART is the only "device" (a good reason to get rid of the
// concept)

const static devinit_t devinit [MAX_DEVICES] = {
/* === */
#if	UART_DRIVER
		{ devinit_uart,	 0 },
#else
		{ NULL, 0 },
#endif
/* === */
#if	MAX_DEVICES > 1
#if	UART_DRIVER > 1
		{ devinit_uart,	 1 },
#else
		{ NULL, 0 }
#endif
#endif	/* MAX_DEVICES > 1 */
	 };

#endif	/* MAX_DEVICES */

static void ssm_init (void), mem_init (void), ios_init (void);

void reset (void) {

	cli_tim;

// EEPROM/SD panic ============================================================

#ifdef	EEPROM_PRESENT
	ee_panic ();
#endif
#ifdef	SDCARD_PRESENT
	sd_panic ();
#endif

// ============================================================================

	while (1) 
		hard_reset;
}

void halt (void) {

	cli_tim;
	diag ("PicOS halted");
	mdelay (500);
	while (1)
		_BIS_SR (LPM4_bits);
}

int main (void) {

#if	STACK_GUARD
	{ register word i;
	for (i = 0; i < (STACK_SIZE / sizeof (word)); i++)
		// This leaves out one word at the very bottom, but that's
		// fine
		*((((address)STACK_END) - 1) + i) = STACK_SENTINEL;
	}
#endif
	/* Initialization */
	ssm_init ();
	// mdelay (1);
	mem_init ();
	// mdelay (1);
	ios_init ();

#if	TCV_PRESENT
	tcv_init ();
#endif
	// Assume root process identity
	__pi_curr = (__pi_pcb_t*) fork (root, 0);
	// Delay root startup for 16 msec to make sure that the drivers go
	// first
	delay (16, 0);

	powerup ();

#if TRIPLE_CLOCK == 0
	// Start the clock; no need to do that in TRIPLE_CLOCK mode where the
	// clock is started explicitly by delay requests
	sti_tim;
#endif
	// We are up and running
	sti;

	// Fall through to scheduler

#include "scheduler.h"

}

#if DIAG_MESSAGES > 1
void __pi_syserror (word ec, const char *m) {

	WATCHDOG_STOP;

#if	DUMP_MEMORY
	dmp_mem ();
#endif
	diag ("SYSERR: %x, %s", ec, m);
#else
void __pi_syserror (word ec) {

	WATCHDOG_STOP;

	dbg_0 (ec); // SYSTEM ERROR
	diag ("SYSERR: %x", ec);
#endif
	cli;

#if RESET_ON_SYSERR

#if LEDS_DRIVER
	for (__pi_mintk = 0; __pi_mintk < 32; __pi_mintk++)
		all_leds_blink;
#endif
	reset ();

#else	/* RESET_ON_SYSERR */

// ============================================================================

	while (1) {
#if LEDS_DRIVER
		all_leds_blink;
#else
		_BIS_SR (LPM4_bits);
#endif
	}
#endif	/* RESET_ON_SYSERR */
}

// ============================================================================
// ssm_init ===================================================================
// ============================================================================

#ifdef	PIN_PORTMAP

static const portmap_t portmap [] = PIN_PORTMAP;

#define	n_portmap (sizeof (portmap) / sizeof (portmap_t))

#endif

// ============================================================================

static void ssm_init () {

	// Disable watchdog timer
	WATCHDOG_STOP;

#include "portinit.h"

// ===========================================================================
// Clock setup ===============================================================
// ===========================================================================

#ifndef	MCLOCK_FROM_CRYSTAL
#define	MCLOCK_FROM_CRYSTAL	0
#endif

#ifdef	__MSP430_1xx__

// ===========================================================================

#if MCLOCK_FROM_CRYSTAL == 0

	// DCO: select maximum frequency
	DCOCTL = DCO2 + DCO1 + DCO0;
	BCSCTL1 = RSEL2 + RSEL1 + RSEL0 + XT2OFF

#if	CRYSTAL_RATE != 32768
	// We are using a high-speed crystal for XT1
		+ XTS
#endif
	;
	// Measured MCLK is ca. 4.5 MHz

// This is in kHz
#define	CPU_CLOCK_RATE	4500

#if 	CRYSTAL2_RATE
	// Assign SMCLK to XTL2
	BCSCTL2 = SELM_DCOCLK | SELS;
#endif

#endif	/* MCLOCK_FROM_CRYSTAL == 0 */

// ===========================================================================

#if MCLOCK_FROM_CRYSTAL	== 1

#if CRYSTAL_RATE < 4000000
#error "S: CRYSTAL_RATE must be >= 4000000 for MCLOCK_FROM_CRYSTAL == 1"
#endif

// Clock from crystal 1

	BCSCTL1 |= XTS;
	do {
		_BIC (IFG1, OFIFG);
		udelay (100);
	} while ((IFG1 & OFIFG) != 0);

  	BCSCTL2 = SELM1 + SELM0

#if CRYSTAL2_RATE > CRYSTAL_RATE
	// Assign SMCLK to XTL2
		+ SELS
#endif
	; 

#define	CPU_CLOCK_RATE	(CRYSTAL_RATE / 1000)

#endif	/* MCLOCK_FROM_CRYSTAL == 1 */

// ===========================================================================

#if MCLOCK_FROM_CRYSTAL == 2

#if CRYSTAL2_RATE == 0
#error "S: Need XT2 for MCLOCK_FROM_CRYSTAL == 2"
#endif
	// Clock from crystal 2: make sure the second oscillator is running
	_BIC_SR (LPM4_bits);

	// Make sure XT2 is enabled
	BCSCTL1 = 0;

	do {
		_BIC (IFG1, OFIFG);
		udelay (100);
	} while ((IFG1 & OFIFG) != 0);

	// SELS is not needed, I think
	BCSCTL2 = SELM1 | SELS; 

#define	CPU_CLOCK_RATE	(CRYSTAL2_RATE / 1000)

#endif	/* MCLOCK_FROM_CRYSTAL == 2 */

#endif	/* 1xx */

// ===========================================================================
// ===========================================================================
// ===========================================================================

#ifdef	__MSP430_4xx__

// The FLL variant

#ifndef	XCAPACITY
#define	XCAPACITY	XCAP10PF
#endif

#if CRYSTAL_RATE != 32768
#define	__XTS	XTS_FLL
#else
#define	__XTS	0
#endif

#if MCLOCK_FROM_CRYSTAL == 0

// MCLOCK from DCO driven by crystal 1

#define	__scfqctl_set (8000000/CRYSTAL_RATE)

#if __scfqctl_set > 128
	FLL_CTL0 = DCOPLUS + XCAPACITY + __XTS;
   	SCFQCTL = (__scfqctl_set/2) - 1;
#define	CPU_CLOCK_RATE	(((__scfqctl_set/2) * 2 * CRYSTAL_RATE) / 1000)
#else
	FLL_CTL0 = XCAPACITY + __XTS;
   	SCFQCTL = __scfqctl_set - 1;
#define	CPU_CLOCK_RATE	(((__scfqctl_set) * 2 * CRYSTAL_RATE) / 1000)
#endif

// MCLK from DCO
#define	__SELM	SELM0

#endif	/* MCLOCK_FROM_CRYSTAL == 0 */

// ===========================================================================

#if MCLOCK_FROM_CRYSTAL == 1

// MCLOCK driven directly by crystal 1

#if CRYSTAL_RATE < 4000000
#error "S: CRYSTAL_RATE must be >= 4000000 for MCLOCK_FROM_CRYSTAL == 1"
#endif
	FLL_CTL0 = XCAP0PF + __XTS;

#define	CPU_CLOCK_RATE	(CRYSTAL_RATE / 1000)

// This is the default

// MCLK from crystal 1
#define	__SELM	SELM3

#endif /* MCLOCK_FROM_CRYSTAL == 1 */

// ===========================================================================

#if MCLOCK_FROM_CRYSTAL == 2

// MCLOCK driven directly by crystal 2

#if CRYSTAL2_RATE == 0
#error "S: Need XT2 for MCLOCK_FROM_CRYSTAL == 2"
#endif
	FLL_CTL0 = XCAP0PF + __XTS;

	// Disable DCO
	_BIS_SR(SCG1);

	_BIC (FLL_CTL1, XT2OFF);

	do {
		_BIC (IFG1, OFIFG);	// Clear fault flag
		udelay (100);
	} while ((IFG1 & OFIFG) != 0);

#define	CPU_CLOCK_RATE	(CRYSTAL2_RATE / 1000)

// MCLK from crystal 2
#define	__SELM	(SELM2 + SELS)

#endif /* MCLOCK_FROM_CRYSTAL == 2 */

// ===========================================================================

	FLL_CTL1 = __SELM

#if CRYSTAL2_RATE == 0
		+ XT2OFF
#endif
	;

// ===========================================================================
#if 0
	//
	// This is the best I could get through rather tedious experiments
	//
	SCFQCTL = 63;			// ACLK * 64 = 32768 * 64 = 2097152 Hz
	SCFI0 =	FLLD_2 | FN_2;		// ... * 2 = 4,194,304
	// SCFI0 =	FLLD_4 | FN_3;	// ... * 4 = 8,388,608 Hz, (2 -- 17.9)
	_BIS (FLL_CTL0, DCOPLUS);	// Mutiplier active
	// This yields 4MHz + for the CPU clock
#endif	/* 0 -- was 449 */

#endif	/* 4xx */

// ===========================================================================
// ===========================================================================
// ===========================================================================

#if defined(__MSP430_6xx__) || defined(__MSP430_5xx__)

#ifndef	PMM_LEVEL
#define	PMM_LEVEL	3
#endif

#if PMM_LEVEL < 0 || PMM_LEVEL > 3
#error "S: PMM_LEVEL must be between 0 and 3!!"
#endif

#if	PMM_LEVEL
// Note: we may need a general function for this (later)

	{
		word hun, one;

		hun = one = 0;

		do {

			hun += SVSHRVL0;	// 0x0100
			one += PMMCOREV0;	// 0x0001

			// Unlock the module
			PMMCTL0_H = PMMPW_H;
			// SVS/M high side to new level
			SVSMHCTL = SVSHE + hun + SVMHE + one;
			// SVM new level
  			SVSMLCTL = SVSLE + SVMLE + one;
			// Delay until settled
  			while ((PMMIFG & SVSMLDLYIFG) == 0);
			// Clear already set flags
  			PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);
			// VCore level
  			PMMCTL0_L = (byte) one;
  			if ((PMMIFG & SVMLIFG))
				// Wait until level reached
    				while ((PMMIFG & SVMLVLRIFG) == 0);
			// Set SVS/M low side to new level
  			SVSMLCTL = SVSLE + hun + SVMLE + one;
			// Done for this step
			PMMCTL0_H = 0x00;

		} while (one < PMM_LEVEL);
	}

#endif	/* PMM_LEVEL */

	// High power request from module enable
	PMMCTL0_H  = PMMPW_H;
	// Needed (among others) by on-chip CC1101
	PMMCTL0_L |= PMMHPMRE;
	PMMCTL0_H  = 0x00;	

#if CRYSTAL_RATE != 32768
#error "S: the LF crystal must be 32768Hz!!"
#endif

#ifndef	LF_CRYSTAL_ABSENT
#define	LF_CRYSTAL_ABSENT	0
#endif

#if LF_CRYSTAL_ABSENT

	// No LF crystal, use internal 32KHz oscillator (as an acting crystal)

#define	_SELREF	SELREF__REFOCLK
#define	_SELA	SELA__REFOCLK

#else

	// LF crystal present

#define	_SELREF	SELREF__XT1CLK
#define	_SELA	SELA__XT1CLK

#ifndef	UCS_XT1DRIVE
#define	UCS_XT1DRIVE	0
#endif
	// Enable 32kHz crystal ACLK; note: GP pins may have to be selected
	// properly for that; we assume that the appropriate PIN_DEFAULT_PxSEL
	// is set as required in board_pins.h

	UCSCTL6 &= ~XT1OFF;

#if UCS_XT1DRIVE < 0 || UCS_XT1DRIVE > 3
#error "S: UCS_XT1DRIVE must be between 0 and 3!!"
#endif

	UCSCTL6 |= (XCAP_3 | (UCS_XT1DRIVE << 6));

#endif	/* Crystal absent or present */

#define	CPU_CLOCK_RATE	12000

	_BIS_SR(SCG0);             // Disable the FLL control loop

	// The CPU clock rate is determined by FLL_DIVIDER and DCO_MULTIPLIER
	// (see mach.h) as ((DCO_MULTIPLIER + 1) * 32768 * D) / 2, where D
	// is determined from FLL_DIVIDER bits: 0-1, 1-2, 2-4, 3-8, ..., e.g.,
	// FLLD_1 and 366 yield (2 * 367 * 32768)/2 = 12025856

	UCSCTL3 = _SELREF;	   // FLL reference
	UCSCTL4 = _SELA | SELS__DCOCLKDIV | SELM__DCOCLKDIV;      
	
	UCSCTL0 = 0x0000;          // Set lowest possible DCOx, MODx
	UCSCTL1 = DCORSEL_5;       // Select suitable range (depends on freq)
	UCSCTL2 = FLL_DIVIDER + DCO_MULTIPLIER;  // 12,025,856 Hz

	_BIC_SR(SCG0);             // Enable the FLL control loop

    	// Worst-case settling time for the DCO when the DCO range bits have
	// been changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS
	// chapter in 5xx UG for optimization.
    	// 32 x 32 x 8 MHz / 32,768 Hz = 250000 = MCLK cycles for DCO to settle
	udelay (250);
	// Loop until XT1 & DCO stabilizes, make sure to execute at least once
	do {
        	UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + XT1HFOFFG + DCOFFG);
		SFRIFG1 &= ~OFIFG; // Clear fault flags

	} while ((SFRIFG1 & OFIFG));	

#ifdef	PIN_PORTMAP
	{
		word i, j;

		PMAPPWD = PMAPPW;
		// To allow future reconfigurations
		PMAPCTL = PMAPRECFG;

		for (i = 0; i < n_portmap; i++)
			for (j = 0; j < 8; j++)
				*((portmap [i] . pm_base) + j) =
					portmap [i] . map [j];
		PMAPPWD = 0;
	}
#endif	/* PIN_PORTMAP */

#endif	/* __MSP430_6xx__ */

// ===========================================================================
// ===========================================================================
// ===========================================================================

	// System timer (assumes TASSEL0/TACLR is same as TBSSEL0/TBCLR)
	// Source is ACLK, divided by 8 (4096 ticks/sec)

#if TRIPLE_CLOCK
	// Continuous mode
	TCI_CTL = TASSEL0 | TACLR | ID0 | ID1 | MC1;
	TCI_CCS = (word) TCI_SEC_DIV;
	sti_sec;
	sti_aux;
#else
	// Up mode
	TCI_CTL = TASSEL0 | TACLR | ID0 | ID1 | MC0;
#endif

}

void setpowermode (word m) {
//
// Only one level of PD
//
	if (m) {

		__pi_systat.pdmode = 1;
		clockdown ();
#if CRYSTAL2_RATE
		// Disable XTL2
		_BIS (BCSCTL1, XT2OFF);
#endif
	} else {

#if CRYSTAL2_RATE
		// Enable XTL2
		_BIC (BCSCTL1, XT2OFF);
#endif
		clockup ();
		__pi_systat.pdmode = 0;
	}
}

#define	mkmk_eval
void udelay (register word n) {
/* =================================== */
/* n should be roughly in microseconds */
/* =================================== */
	__asm__ __volatile__ (
		"1:\n"
#if CPU_CLOCK_RATE > 2000
		" nop\n"
#endif
#if CPU_CLOCK_RATE > 4000
		" nop\n"
#endif
#if CPU_CLOCK_RATE > 6000
		" nop\n"
#endif
#if CPU_CLOCK_RATE > 7000
		" nop\n"
#endif
#if CPU_CLOCK_RATE > 8200
		" nop\n"
#endif
#if CPU_CLOCK_RATE > 9400
		"nop\n"
#endif
#if CPU_CLOCK_RATE > 10600
		"nop\n"
#endif
#if CPU_CLOCK_RATE > 11800
		"nop\n"
#endif
		" dec %[n]\n"
		" jne 1b\n"
			: [n] "+r"(n));
}
#undef	mkmk_eval

void mdelay (word n) {
/* ============ */
/* milliseconds */
/* ============ */
	while (n--)
		udelay (995);
}

// ============================================================================

#if TRIPLE_CLOCK

// ============================================================================
// Version with two clocks and circular timer counter; note that
// HIGH_CRYSTAL_RATE == 0, as the two cannot be set together
// ============================================================================

static word setdel = 0;		// Interval for which the delay timer was set

static word gettav () {
//
// Majority vote read for the timer value (which we are not supposed to read)
//
	word del;

	while (1) {
		del = TCI_VAL;
		if (TCI_VAL == del && TCI_VAL == del && TCI_VAL == del)
			return del;
	}
}

void tci_run_delay_timer () {
//
// Set the delay timer according to __pi_mintk
//
	word d;

	// cli_tim;	// This should be redundant

	// Time to elapse in msecs
	d = __pi_mintk - __pi_old;

	// Don't exceed the maximum
	setdel = (d > TCI_MAXDEL) ? TCI_MAXDEL : d;

	TCI_CCR = gettav () + TCI_DELTOTICKS (setdel);
	sti_tim;
}

void tci_run_auxiliary_timer () {
//
// Start the auxiliary timer
//
	cli_aux;	// In case we are only resetting
	TCI_CCA = gettav () + TCI_HIGH_DIV;
	sti_aux;
}

word tci_update_delay_ticks (Boolean force) {
//
// Called to stop the timer, if running, and tally up the ticks
//
	cli_tim;
	if (setdel) {
		// The timer has been running, otherwise we don't have to
		// bother; force tells whether we have to stop the timer (e.g.,
		// catering to a new delay request), or whether we are just
		// checking; if !force, and the timer is running, just let
		// it go and return YES, to tell update_n_wake to exit;
		// when we return NO, it means that we actually have to
		// examine the delay queue; in such a case, the timer will
		// be restarted later
		if (force) {
			// the role of the 3 is to round it up, such that the
			// estimate will tend to err on the low side,
			// which will make sure that the measured delay is
			// never less than asked for
			__pi_new +=
			    setdel - TCI_TICKSTODEL (TCI_CCR - gettav () + 3);
			// Use it only once
			setdel = 0;
			goto EX;
		}
		sti_tim;
		return YES;
	}
EX:
	return NO;
}
		
// ============================================================================
// Timer interrupts for the triple-clock version ==============================
// ============================================================================

interrupt (TCI_VECTOR_S) timer_auxiliary () {
//
// Two in one: one of them runs exactly every second and is used to maintain
// a precise seconds clock; the other is used for utimers, debouncing,
// blinking leds, and other stuff registered in irq_timer.h.
//
	word aux_timer_inactive;

	// This test also removes the interrupt status
	if (TCI_AUXILIARY_TIMER_INTERRUPT) {

		// Set for the next tick - as quickly as possible
		TCI_CCA += TCI_HIGH_DIV;
		// Flag == should we continue?
		aux_timer_inactive = 1;
		// Take care of utimers
		if (__pi_utims [0] == 0)
			goto EUT;
		if (*(__pi_utims [0])) {
			(*(__pi_utims [0]))--;
			aux_timer_inactive = 0;
		}
		if (__pi_utims [1] == 0)
			goto EUT;
		if (*(__pi_utims [1])) {
			(*(__pi_utims [1]))--;
			aux_timer_inactive = 0;
		}
		if (__pi_utims [2] == 0)
			goto EUT;
		if (*(__pi_utims [2])) {
			(*(__pi_utims [2]))--;
			aux_timer_inactive = 0;
		}
		if (__pi_utims [3] != 0) {
			if (*(__pi_utims [3])) {
				(*(__pi_utims [3]))--;
				aux_timer_inactive = 0;
			}
		}
EUT:
// ============================================================================
#include "irq_timer.h"
// ============================================================================
		if (aux_timer_inactive)
			// Nobody wants us
			cli_aux;

		RTNI;
	}

	// This is the seconds clock. By default it ticks at 1 sec, which is
	// mostly OK. If we are really obsessive about power savings, the rate
	// of the clock can be reduced by setting SECOND_CLOCK_RATE to a value
	// higher than 1, which must be a power of two not exceeding 16, so the
	// valid settings are 1 (the default), 2, 4, 8, 16. At 16, the clock
	// ticks once every 16 seconds which means that __pi_nseconds is going
	// to be incremented by 16 every 16 seconds of real time.
	// Note that any code included in "second.h"/"board_second.h" (like
	// emergency resets and stuff) will be run at the update rate of the
	// clock. [231028]

	// TCI_SEC_DIV can be legitimately 64K (full wrap around) which should
	// quietly translate to zero; hence the cast
	TCI_CCS += (word) TCI_SEC_DIV;

	__pi_nseconds += SECOND_CLOCK_RATE;

	check_stack_overflow;

// ============================================================================
#include "second.h"
// ============================================================================

	RTNI;
}

#if SECOND_CLOCK_RATE != 1

// Otherwise, seconds is a macro simply returning __pi_nseconds

// When returning the number of seconds (with the second clock being coarse),
// we adjust __pi_nseconds by the number of ticks elapsed since last timer
// setting divided by the number of ticks per second. This is cruder than my
// original plans, but OK, I think. The worst that can happen when we call
// seconds is that the timer is exactly on the target (so we shall spin). The
// tick lasts for up to 256 us, so we may occasionally spin for that much.
// [231028]

lword seconds () {

	word t;
	lword res;

	do {
		cli_sec;
		t = gettav () - TCI_CCS;
		res =  __pi_nseconds;
		sti_sec;

	} while (t == 0);

	return res + (t / TIMER_TICKS_PER_SECOND);
}

#endif

interrupt (TCI_VECTOR) timer_int () {

	// This one serves only delays; utimers are handled by the debounce
	// timer

	cli_tim;
	__pi_new += setdel;
	setdel = 0;	// Mark that the counting has been accomplished
	RISE_N_SHINE;

	RTNI;
}

#else	/* TRIPLE_CLOCK */

// ============================================================================
// Timer interrupt and clock modes for the single clock case ==================
// ============================================================================

// clockdown/clockup are macros defined in mach.h

interrupt (TCI_VECTOR) timer_int () {

	word d;

	// Clear hardware watchdog. Its sole purpose is to guard clock
	// interrupts, with clock interrupts acting as software watchdog.

#if HIGH_CRYSTAL_RATE == 0
	// We have two distinct modes for the clock
	if (TCI_CCR == TCI_INIT_HIGH) {
#endif
		// Power up: one tick == 1 JIFFIE
		/* This code assumes that MAX_UTIMERS is 4 */
#define	UTIMS_CASCADE(x) 	if (__pi_utims [x]) {\
					 if (*(__pi_utims [x]))\
						 (*(__pi_utims [x]))--
		UTIMS_CASCADE(0);
		UTIMS_CASCADE(1);
		UTIMS_CASCADE(2);
		UTIMS_CASCADE(3);
		}}}}
#undef UTIMS_CASCADE

		// Extras
#include "irq_timer.h"

		__pi_new++;

		d = __pi_new - __pi_old;

		if (d >= JIFFIES || twakecnd (__pi_old, __pi_new, __pi_mintk))
			RISE_N_SHINE;

		RTNI;

#if HIGH_CRYSTAL_RATE == 0
	// We actually do have two different modes
	}

	// Here we are running in the SLOW mode: one tick ==
	// JIFFIES/TCI_LOW_PER_SEC

#define	UTIMS_CASCADE(x) \
		if (__pi_utims [x]) { \
		  if (*(__pi_utims [x])) \
		    *(__pi_utims [x]) = *(__pi_utims [x]) > \
		      (JIFFIES/TCI_LOW_PER_SEC) ? \
			*(__pi_utims [x]) - (JIFFIES/TCI_LOW_PER_SEC) : 0

	UTIMS_CASCADE(0);
	UTIMS_CASCADE(1);
	UTIMS_CASCADE(2);
	UTIMS_CASCADE(3);
	}}}}
#undef UTIMS_CASCADE

	__pi_new += JIFFIES/TCI_LOW_PER_SEC;

	d = __pi_new - __pi_old;

#if TCI_LOW_PER_SEC > 1
	if (d >= JIFFIES || twakecnd (__pi_old, __pi_new, __pi_mintk))
#endif
		RISE_N_SHINE;

	RTNI;
#endif	/* HIGH_CRYSTAL_RATE == 0 */
}

#endif	/* TRIPLE_CLOCK */

#if GLACIER

// ============================================================================

#if TRIPLE_CLOCK
#error "S: GLACIER is not compatible (and not needed) with TRIPLE_CLOCK"
#endif

#if HIGH_CRYSTAL_RATE
#error "S: GLACIER is not compatible with HIGH_CRYSTAL_RATE!!"
#endif

// ============================================================================

void freeze (word nsec) {
/*
 * Freezes the system in power-down mode for the specified number of seconds.
 */
	byte saveP1IE, saveP2IE;

#if UART_DRIVER || UART_TCV
	word saveUIE;
#endif
	byte saveLEDsV, saveLEDsB;

	// In case some of these take too long for the clock guard
	cli;

	// Save I/O pin interrupt configuration
	saveP1IE = P1IE;
	saveP2IE = P2IE;
	// And disable them
	P1IE = P2IE = 0x00;

#if LEDS_DRIVER
	// Save leds status and turn them off
	leds_save (saveLEDsV, saveLEDsB);
#endif

#if UART_DRIVER || UART_TCV
	
	// Save and disable UART interrupt configuration
	uart_save_ie_flags (saveUIE);
	uart_a_wait_tx;
#ifdef UART_PREINIT_B
	uart_b_wait_tx;
#endif
#endif
	powerdown ();
	// Save clock state

	while (1) {
		_BIS_SR (LPM3_bits + GIE);
		cli;
		if (nsec == 0)
			break;
		nsec--;
	}

	P1IE = saveP1IE;
	P2IE = saveP2IE;

#if UART_DRIVER || UART_TCV
	// Reset the UART to get it back to normal
	uart_a_reset_on;
	uart_a_reset_off;
#ifdef 	UART_PREINIT_B
	uart_b_reset_on;
	uart_b_reset_off;
#endif
	uart_restore_ie_flags (saveUIE);
#endif
	powerup ();

	// Trigger the interrupts if they are pending
	P1IFG = (P1IE & (P1IES ^ P1IN));
	P2IFG = (P2IE & (P2IES ^ P2IN));

#if LEDS_DRIVER
	leds_restore (saveLEDsV, saveLEDsB);
#endif
	sti;
}

#endif	/* GLACIER */

static void mem_init () {

	__pi_malloc_init ();
}

#if	STACK_GUARD
word __pi_stackfree (void) {

	word sc;
	for (sc = 0; sc < STACK_SIZE / sizeof (word); sc++)
		if (*(((word*)STACK_END) + sc) != STACK_SENTINEL)
			break;
	return sc;
}

#endif

static void ios_init () {

#ifdef	EMERGENCY_STARTUP_CONDITION

	if (EMERGENCY_STARTUP_CONDITION) {
		EMERGENCY_STARTUP_ACTION;
	}
#endif

// ============================================================================
// Praxis-dependent extra initialization ======================================
// ============================================================================

#ifdef	SENSOR_INITIALIZERS
	__pi_init_sensors ();
#endif

#ifdef	ACTUATOR_INITIALIZERS
	__pi_init_actuators ();
#endif

#ifdef	EXTRA_INITIALIZERS
	// Extra initialization
	EXTRA_INITIALIZERS;
#endif

#if	UART_DRIVER || UART_TCV
	// A UART is configured, initialize it beforehand without enabling
	// anything, which is up to the driver plugin. We just want to be able
	// to use diag.
	preinit_uart ();
#endif

#if	DIAG_MESSAGES
	diag ("");

#ifdef	BANNER
	diag (BANNER);
#else
	diag ("PicOS v" SYSVER_S "/" SYSVER_R
#ifdef	SYSVER_B
		"-" SYSVER_B
#endif
        	", (C) Olsonet Communications, 2002-2021");
	diag ("Leftover RAM: %d bytes", (word)STACK_END - (word)(&__BSS_END));
#endif

#endif	/* DIAG_MESSAGES */

	dbg_1 (0x1000 | SYSVER_X);
	dbg_1 ((word)STACK_END - (word)(&__BSS_END)); // RAM in bytes

#if MAX_DEVICES
	/* Initialize devices */
	int i;
	for (i = UART; i < MAX_DEVICES; i++)
		if (devinit [i] . init != NULL)
			devinit [i] . init (devinit [i] . param);
#endif
	/* Make SMCLK/MCLK available on P5.5, P5.4 */
	//_BIS (P5OUT, 0x30);
	//_BIS (P5SEL, 0x30);
}

/* ------------------------------------------------------------------------ */
/* ============================ DEVICE DRIVERS ============================ */
/* ------------------------------------------------------------------------ */

#if	UART_DRIVER || UART_TCV

uart_t	__pi_uart [N_UARTS];

static void preinit_uart () {

#ifdef	UART_PREINIT_A
	// UART_A
	UART_PREINIT_A;
	uart_a_reset_on;
	uart_a_set_clock;
	uart_a_set_rate_def;
	uart_a_set_write_int;
	uart_a_enable;
	uart_a_reset_off;
#if UART_RATE_SETTABLE
	__pi_uart [0] . flags = UART_RATE_INDEX;
#endif

#endif	/* UART_PREINIT_A */

/*
 * Note: the second UART doesn't want to work at 9600 with the standard slow
 * crystal. The xmitter is OK, but the receiver gets garbage. 4800 is fine. 
 * Also, things seem to work fine (up to 115200) with a high speed crystal.
 */
#ifdef	UART_PREINIT_B
	// UART_B
	UART_PREINIT_B;
	uart_b_reset_on;
	uart_b_set_clock;
	uart_b_set_rate_def;
	uart_b_set_write_int;
	uart_b_enable;
	uart_b_reset_off;
#if UART_RATE_SETTABLE
	__pi_uart [1] . flags = UART_RATE_INDEX;
#endif

#endif	/* UART_PREINIT_B */
}

#if UART_RATE_SETTABLE

static const uart_rate_table_entry_t urates [] = UART_RATE_TABLE;

#define	N_RATES		(sizeof(urates) / sizeof(uart_rate_table_entry_t))

Boolean __pi_uart_setrate (word rate, uart_t *ua) {

	byte j;

	for (j = 0; j < N_RATES; j++) {
		if (rate == urates [j] . rate) {
			// Found
#if N_UARTS > 1
			if (ua != __pi_uart)
				uart_b_set_rate (urates [j]);
			else
#endif	/* N_UARTS */
				uart_a_set_rate (urates [j]);
			ua->flags = (ua->flags & ~UART_RATE_MASK) | j;
			return YES;
		}
	}
	return NO;
}

word __pi_uart_getrate (uart_t *ua) {

	return urates [ua->flags & UART_RATE_MASK] . rate;
}

#endif 	/* UART_RATE_SETTABLE */

#endif	/* UART_DRIVER || UART_TCV */

// ============================================================================

#if	UART_DRIVER
/* ======== */
/* The UART */
/* ======== */

#define	uart_write8(u,w) \
	__usel (u, uart_b_write (w), uart_a_write (w))

#define uart_enable_write_int(u) __usel (u, uart_b_enable_write_int, \
						uart_a_enable_write_int)

#define uart_disable_int(u)	__usel (u, uart_b_disable_int, \
						uart_a_disable_int)

#define uart_disable_read_int(u) __usel (u, uart_b_disable_read_int, \
						uart_a_disable_read_int)

#define uart_disable_write_int(u) __usel (u, uart_b_disable_write_int, \
						uart_a_disable_write_int)

#define	uart_read(u)		__ualt (u, uart_b_read, uart_a_read)

#define uart_enable_read_int(u)	__usel (u, uart_b_enable_read_int, \
						uart_a_enable_read_int)

/* ======================= */
/* Common request function */
/* ======================= */
static INLINE int ioreq_uart (uart_t *u, int operation, char *buf, int len) {

	switch (operation) {

		case READ:

#if UART_INPUT_BUFFER_LENGTH > 1
			uart_disable_read_int (u);
			if (u->ib_count == 0) {
				// The buffer is empty
				_BIS (u->flags, UART_FLAGS_IN);
				return -2;
			}
			_BIC (u->flags, UART_FLAGS_IN);
			uart_enable_read_int (u);
			operation = len;
			while (len) {
				uart_disable_read_int (u);
				if (u->ib_count == 0) {
					uart_enable_read_int (u);
					break;
				}
				u->ib_count--;
				uart_enable_read_int (u);
				*buf++ = u->in [u->ib_out];
				if (++(u->ib_out) == UART_INPUT_BUFFER_LENGTH)
					u->ib_out = 0;
				len--;
			}
			return operation - len;
#else
			if ((u->flags & UART_FLAGS_IN)) {
				// Receive interrupt is disabled
R_redo:
				*buf = u->in;
				_BIC (u->flags, UART_FLAGS_IN);
				uart_enable_read_int (u);
				return 1;
			}
			uart_disable_read_int (u);
			if ((u->flags & UART_FLAGS_IN))
				// Account for the race
				goto R_redo;
			// The buffer is empty
			return -2;
#endif
		case WRITE:

#if defined(blue_ready) && defined(BLUETOOTH_UART)
			// Bluetooth on UART
			if (
#if N_UARTS > 1
				// There is a choice of UARTs, have to check if
				// this is the one
				u
#if BLUETOOTH_UART == 0
				   // BT on UART 0
				   ==
#else
				   // BT on UART 1
				   !=
#endif	/* UART 0 or 1 */
				      __pi_uart &&
#endif	/* Multiple UARTs */
					(u->flags & UART_FLAGS_NOTRANS) == 0 &&
					  // BT must be connected
					  !blue_ready)
								return 1;
#endif	/* Transparent BT */
			if ((u->flags & UART_FLAGS_OUT) == 0) {
X_redo:
				u->out = *buf;
				_BIS (u->flags, UART_FLAGS_OUT);
				uart_enable_write_int (u);
				return 1;
			}
			uart_disable_write_int (u);
			if ((u->flags & UART_FLAGS_OUT) == 0)
				goto X_redo;
			return -2;
					
		case NONE:

			// Cleanup
#if UART_INPUT_BUFFER_LENGTH < 2
			if ((u->flags & UART_FLAGS_IN) == 0) {
				uart_enable_read_int (u);
			}
#else
				uart_enable_read_int (u);
#endif
			if ((u->flags & UART_FLAGS_OUT))
				uart_enable_write_int (u);

			return 0;

		case CONTROL:

#if UART_RATE_SETTABLE
			if (len == UART_CNTRL_SETRATE) {
				if (__pi_uart_setrate (*((word*)buf), u))
						return 1;
				syserror (EREQPAR, "uar");
			}

			if (len == UART_CNTRL_GETRATE) {
				*((word*)buf) = __pi_uart_getrate (u);
				return 1;
			}
#endif

#ifdef blue_ready
			if (len == UART_CNTRL_TRANSPARENT) {
				if (*((word*)buf))
					_BIC (u->flags, UART_FLAGS_NOTRANS);
				else
					_BIS (u->flags, UART_FLAGS_NOTRANS);
				return 1;
			}
#endif
			/* Fall through */
		default:
			syserror (ENOOPER, "uai");
			/* No return */
			return 0;
	}
}

/* ========================== */
/* Interrupt service routines */
/* ========================== */

#ifdef	UART_A_TX_RX_VECTOR
//
// This means that there is a single vector for both RX and TX, as in CC430
//
interrupt (UART_A_TX_RX_VECTOR) uart_a_tx_rx_int (void) {

	if (uart_a_tx_interrupt) {

// ============================================================================

#else

interrupt (UART_A_TX_VECTOR) uart_a_tx_int (void) {

// ============================================================================

#endif

// ============================================================================
// TX interrupt service =======================================================
// ============================================================================

	// Disable until a character arrival
	uart_a_disable_write_int;

#if NESTED_INTERRUPTS
	// Enable other interrupts. Note: we do this only for UART0.
	sti;
#endif
	RISE_N_SHINE;

	if ((__pi_uart [0] . flags & UART_FLAGS_OUT) == 0) {
		// Keep the interrupt pending
		uart_a_set_write_int;
	} else {
		_BIC (__pi_uart [0] . flags, UART_FLAGS_OUT);
		uart_a_write (__pi_uart [0] . out);
	}
	i_trigger (devevent (UART_A, WRITE));
	RTNI;
}

#ifdef UART_A_TX_RX_VECTOR

	else {

#else

interrupt (UART_A_RX_VECTOR) uart_a_rx_int (void) {

#endif

// ============================================================================
// RX interrupt service =======================================================
// ============================================================================

#define	ua	(__pi_uart + 0)

	uart_a_disable_read_int;

#if NESTED_INTERRUPTS
	// Enable other interrupts (UART0 only)
	sti;
#endif

#if UART_INPUT_BUFFER_LENGTH > 1

	if (ua->ib_count <= UART_INPUT_BUFFER_LENGTH) {
		// Not full
		ua->in [ua->ib_in] = uart_a_read;
		if (++(ua->ib_in) == UART_INPUT_BUFFER_LENGTH)
			ua->ib_in = 0;
		ua->ib_count++;
	}

	if ((ua -> flags & UART_FLAGS_IN)) {
		RISE_N_SHINE;
		i_trigger (devevent (UART_A, READ));
	}

#if NESTED_INTERRUPTS
	cli;
#endif
	uart_a_enable_read_int;

#else	/* UART_INPUT_BUFFER_LENGTH */

	RISE_N_SHINE;

	if ((ua -> flags & UART_FLAGS_IN)) {
		// Keep the interrupt pending
		uart_a_set_read_int;
	} else {
		_BIS (ua -> flags, UART_FLAGS_IN);
		ua -> in = uart_a_read;
	}
	i_trigger (devevent (UART_A, READ));
#endif
	RTNI;

}

#ifdef UART_A_TX_RX_VECTOR
}
#endif

// ============================================================================

static int ioreq_uart_a (int op, char *b, int len) {
	return ioreq_uart (ua, op, b, len);
}

#undef ua

#if UART_DRIVER > 1

interrupt (UART_B_TX_VECTOR) uart_b_tx_int (void) {
	RISE_N_SHINE;

	if ((__pi_uart [1] . flags & UART_FLAGS_OUT) == 0) {
		// Keep the interrupt pending
		uart_b_set_write_int;
	} else {
		_BIC (__pi_uart [1] . flags, UART_FLAGS_OUT);
		uart_b_write (__pi_uart [1] . out);
	}
	// Disable until a character arrival
	uart_b_disable_write_int;
	i_trigger (devevent (UART_B, WRITE));
	RTNI;
}

interrupt (UART_B_RX_VECTOR) uart_b_rx_int (void) {

#define	ua	(__pi_uart + 1)

#if UART_INPUT_BUFFER_LENGTH > 1

	if (ua->ib_count <= UART_INPUT_BUFFER_LENGTH) {
		// Not full
		ua->in [ua->ib_in] = uart_b_read;
		if (++(ua->ib_in) == UART_INPUT_BUFFER_LENGTH)
			ua->ib_in = 0;
		ua->ib_count++;
	}
	if ((ua -> flags & UART_FLAGS_IN)) {
		RISE_N_SHINE;
		i_trigger (devevent (UART_B, READ));
	}
#else
	RISE_N_SHINE;

	if ((ua -> flags & UART_FLAGS_IN)) {
		// Keep the interrupt pending
		uart_b_set_read_int;
	} else {
		_BIS (ua -> flags, UART_FLAGS_IN);
		ua -> in = uart_b_read;
	}
	uart_b_disable_read_int;
	i_trigger (devevent (UART_B, READ));
#endif
	RTNI;
}

static int ioreq_uart_b (int op, char *b, int len) {
	return ioreq_uart (ua, op, b, len);
}

#undef ua

#endif

/* =========== */
/* Initializer */
/* =========== */
static void devinit_uart (int devnum) {

#if UART_DRIVER > 1
	if (devnum == 0) {
#endif
		adddevfunc (ioreq_uart_a, UART_A);
#if UART_DRIVER > 1
	} else {
		adddevfunc (ioreq_uart_b, UART_B);
	}
#endif

#if UART_INPUT_BUFFER_LENGTH > 1
	__pi_uart [devnum] . ib_count = __pi_uart [devnum] . ib_in =
		__pi_uart [devnum] . ib_out = 0;
#endif

#if UART_DRIVER > 1
	__pi_uart [devnum] . selector = devnum;
#endif
	uart_enable_read_int (__pi_uart + devnum);
}

/* =========== */
/* UART DRIVER */
/* =========== */
#endif

/* ============== */
/* End of drivers */
/* ============== */
