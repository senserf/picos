#include "kernel.h"

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "irq_timer_headers.h"

#ifndef	MCLOCK_FROM_CRYSTAL
#define	MCLOCK_FROM_CRYSTAL	0
#endif

extern 			pcb_t		*zz_curr;
extern 			address	zz_utims [MAX_UTIMERS];
extern	void		_reset_vector__;

word			zz_restart_sp;

#define	STACK_SENTINEL	0xB779

void	zz_malloc_init (void);

/* ========================== */
/* Device driver initializers */
/* ========================== */

#if	UART_DRIVER || UART_TCV

static void preinit_uart (void);

#if	UART_DRIVER

#define	N_UARTS	UART_DRIVER

#if	UART_INPUT_FLOW_CONTROL || UART_OUTPUT_FLOW_CONTROL
#if	UART_DRIVER > 1
#error	"flow control is only available when UART_DRIVER == 1"
#endif

#endif

static void	devinit_uart (int);

#else	/* UART_DRIVER */

#define	N_UARTS	UART_TCV

#endif	/* UART_DRIVER */

// ============================================================================
// This is clumsy. We need a way to make the second UART configurable
// dynamically, as it can be shared with SPI, so we need it as a formal
// device, but we don't want the preinit, freeze auto enable/disable, etc.
// ============================================================================

#if N_UARTS < 2
// This is defined in mach.h, but can be undef'ed, e.g., from options.sys
#ifdef	UART_PREINIT_B
#undef	UART_PREINIT_B
#endif
#endif

// ============================================================================

#endif  /* UART_DRIVER || UART_TCV */

extern void	__bss_end;

#if MAX_DEVICES

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
#endif	/* MAX_DEVICES */
	 };

#endif	/* MAX_DEVICES */

static void ssm_init (void), mem_init (void), ios_init (void);


void clockdown (void) {

#if HIGH_CRYSTAL_RATE
	// Sorry, the watchdog must be stopped in the slow-clock mode if ACLK
	// is driven by a high-speed crystal
	WATCHDOG_HOLD;
#endif
	TBCCR0 = TIMER_B_INIT_LOW;	// 1, 2, or 16 ticks per second
}

void clockup (void) {

	TBCCR0 = TIMER_B_INIT_HIGH;	// 1024 ticks per second

#if HIGH_CRYSTAL_RATE
	// Resume the watchdog (it is never stopped if ACLK is driven by a
	// low-speed crystal
	WATCHDOG_RESUME;
#endif

}

void powerdown (void) {

	zz_systat.pdmode = 1;
	clockdown ();
#if CRYSTAL2_RATE
	// Disable XTL2
	_BIS (BCSCTL1, XT2OFF);
#endif
}

void powerup (void) {

#if CRYSTAL2_RATE
	// Enable XTL2
	_BIC (BCSCTL1, XT2OFF);
#endif
	clockup ();
	zz_systat.pdmode = 0;
}

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

	hard_reset;
}

void halt (void) {

	cli_tim;
	diag ("PicOS halted");
	mdelay (500);
	while (1)
		_BIS_SR (LPM4_bits);
}

#if	ADC_PRESENT
// There is no general ADC interface at the moment. Well, not of the same
// kind as there used to be for eCOG. There is an ADC sampler (see
// ../adc_sampler.[ch]) as well as various board-specific hooks to the ADC.
// Also, the ADC is used internally for Radio RSSI.
#endif

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
	mdelay (1);
	mem_init ();
	mdelay (1);
	ios_init ();

#if	TCV_PRESENT
	tcv_init ();
#endif
	/* The event loop */
	zz_curr = (pcb_t*) fork (root, NULL);
	/* Delay root startup until the drivers have been initialized */
	delay (16, 0);
	// Start the clock
	sti_tim;
	_EINT ();

	// We run at the longest possible interval, which means 1 sec for
	// the standard 32768Hz crystal, but may mean as little as 1/250 sec
	// for the 8MHz crystal.
	WATCHDOG_START;

	// Run the scheduler

#include "scheduler.h"

}

#if DIAG_MESSAGES > 1
void zzz_syserror (int ec, const char *m) {

	WATCHDOG_HOLD;

#if	DUMP_MEMORY
	dmp_mem ();
#endif
	diag ("SYSTEM ERROR: %x, %s", ec, m);
#else
void zzz_syserror (int ec) {

	WATCHDOG_HOLD;

	dbg_0 (ec); // SYSTEM ERROR
	diag ("SYSTEM ERROR: %x", ec);
#endif
	cli;

#if RESET_ON_SYSERR

#if LEDS_DRIVER
	for (zz_lostk = 0; zz_lostk < 32; zz_lostk++) {
		leds (0, 1); leds (1, 1); leds (2, 1); leds (3, 1);
		mdelay (100);
		leds (0, 0); leds (1, 0); leds (2, 0); leds (3, 0);
		mdelay (100);
	}
#endif
	while (1) 
		reset ();

#else	/* RESET_ON_SYSERR */

// ========== DISABLED ========================================================
#if 0
#ifdef	EEPROM_PRESENT
	ee_sync (WNONE);
#endif
#ifdef	SDRAM_PRESENT
	sd_sync ();
#endif
#endif
// ============================================================================

	while (1) {
#if LEDS_DRIVER
		leds (0, 1); leds (1, 1); leds (2, 1); leds (3, 1);
		mdelay (100);
		leds (0, 0); leds (1, 0); leds (2, 0); leds (3, 0);
		mdelay (100);
#else
		_BIS_SR (LPM4_bits);
#endif
	}
#endif	/* RESET_ON_SYSERR */
}

static void ssm_init () {

	// Disable watchdog timer
	WATCHDOG_STOP;

#ifndef	PIN_DEFAULT_P1SEL
#define	PIN_DEFAULT_P1SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P1DIR
#define	PIN_DEFAULT_P1DIR	0xFF	// For power savings, the default is OUT
#endif
#ifndef	PIN_DEFAULT_P1OUT
#define	PIN_DEFAULT_P1OUT	0x00
#endif

#ifndef	PIN_DEFAULT_P2SEL
#define	PIN_DEFAULT_P2SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P2DIR
#define	PIN_DEFAULT_P2DIR	0xFF
#endif
#ifndef	PIN_DEFAULT_P2OUT
#define	PIN_DEFAULT_P2OUT	0x00
#endif

#ifndef	PIN_DEFAULT_P3SEL
#define	PIN_DEFAULT_P3SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P3DIR
#define	PIN_DEFAULT_P3DIR	0xFF
#endif
#ifndef	PIN_DEFAULT_P3OUT
#define	PIN_DEFAULT_P3OUT	0x00
#endif

#ifndef	PIN_DEFAULT_P4SEL
#define	PIN_DEFAULT_P4SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P4DIR
#define	PIN_DEFAULT_P4DIR	0xFF
#endif
#ifndef	PIN_DEFAULT_P4OUT
#define	PIN_DEFAULT_P4OUT	0x00
#endif

#ifndef	PIN_DEFAULT_P5SEL
#define	PIN_DEFAULT_P5SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P5DIR
#define	PIN_DEFAULT_P5DIR	0xFF
#endif
#ifndef	PIN_DEFAULT_P5OUT
#define	PIN_DEFAULT_P5OUT	0x00
#endif

#ifndef	PIN_DEFAULT_P6SEL
#define	PIN_DEFAULT_P6SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P6DIR
#define	PIN_DEFAULT_P6DIR	0xFF
#endif
#ifndef	PIN_DEFAULT_P6OUT
#define	PIN_DEFAULT_P6OUT	0x00
#endif

#ifdef	__MSP430_HAS_PORT7__

#ifndef	PIN_DEFAULT_P7SEL
#define	PIN_DEFAULT_P7SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P7DIR
#define	PIN_DEFAULT_P7DIR	0xFF
#endif
#ifndef	PIN_DEFAULT_P7OUT
#define	PIN_DEFAULT_P7OUT	0x00
#endif

#endif

#ifdef	__MSP430_HAS_PORT8__

#ifndef	PIN_DEFAULT_P8SEL
#define	PIN_DEFAULT_P8SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P8DIR
#define	PIN_DEFAULT_P8DIR	0xFF
#endif
#ifndef	PIN_DEFAULT_P8OUT
#define	PIN_DEFAULT_P8OUT	0x00
#endif

#endif

#ifdef	__MSP430_HAS_PORT9__

#ifndef	PIN_DEFAULT_P9SEL
#define	PIN_DEFAULT_P9SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P9DIR
#define	PIN_DEFAULT_P9DIR	0xFF
#endif
#ifndef	PIN_DEFAULT_P9OUT
#define	PIN_DEFAULT_P9OUT	0x00
#endif

#endif

#ifdef	__MSP430_HAS_PORT10__

#ifndef	PIN_DEFAULT_P10SEL
#define	PIN_DEFAULT_P10SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P10DIR
#define	PIN_DEFAULT_P10DIR	0xFF
#endif
#ifndef	PIN_DEFAULT_P10OUT
#define	PIN_DEFAULT_P10OUT	0x00
#endif

#endif

	// Pre-initialize pins
	P1IE = P2IE = 0x00;
	P1IES = P2IES = 0x00;
	P1IFG = P2IFG = 0x00;

	P1SEL = PIN_DEFAULT_P1SEL;
	P1OUT = PIN_DEFAULT_P1OUT;
	P1DIR = PIN_DEFAULT_P1DIR;

	P2SEL = PIN_DEFAULT_P2SEL;
	P2OUT = PIN_DEFAULT_P2OUT;
	P2DIR = PIN_DEFAULT_P2DIR;

	P3SEL = PIN_DEFAULT_P3SEL;
	P3OUT = PIN_DEFAULT_P3OUT;
	P3DIR = PIN_DEFAULT_P3DIR;

	P4SEL = PIN_DEFAULT_P4SEL;
	P4OUT = PIN_DEFAULT_P4OUT;
	P4DIR = PIN_DEFAULT_P4DIR;

	P5SEL = PIN_DEFAULT_P5SEL;
	P5OUT = PIN_DEFAULT_P5OUT;
	P5DIR = PIN_DEFAULT_P5DIR;

	P6SEL = PIN_DEFAULT_P6SEL;
	P6OUT = PIN_DEFAULT_P6OUT;
	P6DIR = PIN_DEFAULT_P6DIR;


#ifdef	__MSP430_HAS_PORT7__
	P7SEL = PIN_DEFAULT_P7SEL;
	P7OUT = PIN_DEFAULT_P7OUT;
	P7DIR = PIN_DEFAULT_P7DIR;
#endif
#ifdef	__MSP430_HAS_PORT8__
	P8SEL = PIN_DEFAULT_P8SEL;
	P8OUT = PIN_DEFAULT_P8OUT;
	P8DIR = PIN_DEFAULT_P8DIR;
#endif
#ifdef	__MSP430_HAS_PORT9__
	P9SEL = PIN_DEFAULT_P9SEL;
	P9OUT = PIN_DEFAULT_P9OUT;
	P9DIR = PIN_DEFAULT_P9DIR;
#endif
#ifdef	__MSP430_HAS_PORT10__
	P10SEL = PIN_DEFAULT_P10SEL;
	P10OUT = PIN_DEFAULT_P10OUT;
	P10DIR = PIN_DEFAULT_P10DIR;
#endif

// ======================
// Monitor pins overrides
// ======================
#ifdef	MONITOR_PIN_CPU
	_PDS (MONITOR_PIN_CPU, 1);
#endif
#ifdef	MONITOR_PIN_CLOCK
	_PDS (MONITOR_PIN_CLOCK, 1);
#endif
#ifdef	MONITOR_PIN_SCHED
	_PDS (MONITOR_PIN_SCHED, 1);
#endif
// ======================

// ===========================================================================
// Clock setup ===============================================================
// ===========================================================================

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
#error "CRYSTAL_RATE must be >= 4000000 for MCLOCK_FROM_CRYSTAL == 1"
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
#error "Need XT2 for MCLOCK_FROM_CRYSTAL == 2"
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
#error "CRYSTAL_RATE must be >= 4000000 for MCLOCK_FROM_CRYSTAL == 1"
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
#error "Need XT2 for MCLOCK_FROM_CRYSTAL == 2"
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
// ===========================================================================

#endif	/* 4xx */

	// System timer
	TBCTL = TBSSEL0 | TBCLR; 	// ACLK source
	_BIS (TBCTL, ID0 | ID1);	// divided by 8 = 4096 ticks/sec

	// Select power up and high clock rate
	powerup ();

#if HIGH_CRYSTAL_RATE
	// Was set by powerup, we have to stop it until we actually start
	// the clock
	WATCHDOG_HOLD;
#endif
	// Start it in up mode, interrupts still disabled
	_BIS (TBCTL, MC0);
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
		" dec %[n]\n"
		" jne 1b\n"
			: [n] "+r"(n));
}

#undef mkmk_eval

void mdelay (word n) {
/* ============ */
/* milliseconds */
/* ============ */
	while (n--)
		udelay (995);
}
/* =============== */
/* Timer interrupt */
/* =============== */
interrupt (TIMERB0_VECTOR) timer_int () {

#ifdef	MONITOR_PIN_CLOCK
	_PVS (MONITOR_PIN_CLOCK, 1);
#endif

#if	STACK_GUARD
	if (*(((word*)STACK_END) - 1) != STACK_SENTINEL)
		syserror (ESTACK, "timer_int");
#endif

	// Clear hardware watchdog. Its sole purpose is to guard clock
	// interrupts, while clock interrupts implement a software watchdog.
	WATCHDOG_CLEAR;

	if (TBCCR0 == TIMER_B_INIT_HIGH) {
		// Power up: one tick == 1 JIFFIE
		/* This code assumes that MAX_UTIMERS is 4 */
#define	UTIMS_CASCADE(x) 	if (zz_utims [x]) {\
					 if (*(zz_utims [x]))\
						 (*(zz_utims [x]))--
		UTIMS_CASCADE(0);
		UTIMS_CASCADE(1);
		UTIMS_CASCADE(2);
		UTIMS_CASCADE(3);
		}}}}
#undef UTIMS_CASCADE
		zz_lostk++;

#if	ADC_PRESENT
		// Stub
#endif

		// Extras
#include "irq_timer.h"

		// Run the scheduler at least once every second - to
		// keep the second clock up to date
		if ((zz_lostk >= JIFFIES) ||
		    (zz_mintk && zz_mintk <= zz_lostk)) {

#if WATCHDOG_ENABLED
			if (zz_lostk >= 16 * JIFFIES ) {
				// Software watchdog reset: 16 seconds
#ifdef	WATCHDOG_SAVER
				WATCHDOG_CLEAR;
				WATCHDOG_SAVER ();
#endif
				reset ();
			}
			
#endif	/* WATCHDOG_ENABLED */

			RISE_N_SHINE;
		}

#ifdef	MONITOR_PIN_CLOCK
		_PVS (MONITOR_PIN_CLOCK, 0);
#endif
		RTNI;
	}

	// Here we are running in the SLOW mode: one tick ==
	// JIFFIES/TIMER_B_LOW_PER_SEC

#define	UTIMS_CASCADE(x) \
		if (zz_utims [x]) { \
		  if (*(zz_utims [x])) \
		    *(zz_utims [x]) = *(zz_utims [x]) > \
		      (JIFFIES/TIMER_B_LOW_PER_SEC) ? \
			*(zz_utims [x]) - (JIFFIES/TIMER_B_LOW_PER_SEC) : 0

	UTIMS_CASCADE(0);
	UTIMS_CASCADE(1);
	UTIMS_CASCADE(2);
	UTIMS_CASCADE(3);
	}}}}
#undef UTIMS_CASCADE

	zz_lostk += JIFFIES/TIMER_B_LOW_PER_SEC;

#if TIMER_B_LOW_PER_SEC > 1
	if ((zz_lostk >= JIFFIES) || (zz_mintk && zz_mintk <= zz_lostk))
#endif
	{

#if WATCHDOG_ENABLED
		if (zz_lostk >= 16 * JIFFIES) {
			// Software watchdog reset (16 sec)
#ifdef	WATCHDOG_SAVER
			WATCHDOG_CLEAR;
			WATCHDOG_SAVER ();
#endif
			reset ();
		}
#endif	/* WATCHDOG_ENABLED */

		RISE_N_SHINE;
	}

#ifdef	MONITOR_PIN_CLOCK
	_PVS (MONITOR_PIN_CLOCK, 0);
#endif
	RTNI;
}

#if GLACIER
void freeze (word nsec) {
/*
 * Freezes the system in power-down mode for the specified number of seconds.
 */
	byte saveP1IE, saveP2IE;

#if UART_DRIVER || UART_TCV
	byte saveIE1, saveIE2;
#endif
	byte saveLEDs;
	word saveLostK;

	// In case some of these take too long for the clock guard
	WATCHDOG_HOLD;

	cli;

	// Save I/O pin interrupt configuration
	saveP1IE = P1IE;
	saveP2IE = P2IE;
	// And disable them
	P1IE = P2IE = 0x00;

	// Save leds status and turn them off
	saveLEDs = leds_save ();
	leds_off ();

#if UART_DRIVER || UART_TCV
	// Save UART interrupt configuration
	saveIE1 = IE1;
	saveIE2 = IE2;
	// And disable them
	IE1 = 0;
	IE2 = 0;
	while ((UTCTL_A & TXEPT) == 0);
#ifdef UART_PREINIT_B
	while ((UTCTL_B & TXEPT) == 0);
#endif
#endif
	// We should be OK now
	WATCHDOG_RESUME;

	powerdown ();
	// Save clock state
	saveLostK = zz_lostk;

	while (1) {
		zz_lostk = 0;
		_BIS_SR (LPM3_bits + GIE);
		cli;
		if (nsec == 0)
			break;
		nsec--;
	}

	WATCHDOG_HOLD;

	P1IE = saveP1IE;
	P2IE = saveP2IE;

#if UART_DRIVER || UART_TCV
	// Reset the UART to get it back to normal
	_BIS (UCTL_A, SWRST);
	_BIC (UCTL_A, SWRST);
#ifdef 	UART_PREINIT_B
	_BIS (UCTL_B, SWRST);
	_BIC (UCTL_B, SWRST);
#endif
	IE1 = saveIE1;
	IE2 = saveIE2;
#endif
	powerup ();

	// Trigger the interrupts if they are pending
	P1IFG = (P1IE & (P1IES ^ P1IN));
	P2IFG = (P2IE & (P2IES ^ P2IN));

	zz_lostk = saveLostK;

	leds_restore (saveLEDs);

	WATCHDOG_START;
	sti;
}

#endif	/* GLACIER */

static void mem_init () {

	zz_malloc_init ();
}

#if	STACK_GUARD
word zzz_stackfree (void) {

	word sc;
	for (sc = 0; sc < STACK_SIZE / sizeof (word); sc++)
		if (*(((word*)STACK_END) + sc) != STACK_SENTINEL)
			break;
	return sc;
}

#endif

static void ios_init () {

	pcb_t *p;
	int i;

#ifdef	RESET_ON_KEY_PRESSED

	if (RESET_ON_KEY_PRESSED) {
		for (i = 0; i < 4; i++) {
			leds (0,1); leds (1,1); leds (2,1); leds (3,1);
			mdelay (256);
			leds (0,0); leds (1,0); leds (2,0); leds (3,0);
			mdelay (256);
		}
#ifdef	board_key_erase_action
		board_key_erase_action;
#endif
		while (RESET_ON_KEY_PRESSED);
	}
#endif

#ifdef SENSOR_LIST
	zz_init_sensors ();
#endif

#ifdef ACTUATOR_LIST
	zz_init_actuators ();
#endif

#ifdef LCDG_PRESENT
	zz_lcdg_init ();
#endif

#if	UART_DRIVER || UART_TCV
	// A UART is configured, initialize it beforehand without enabling
	// anything, which is up to the driver plugin. We just want to be able
	// to use diag.
	preinit_uart ();
#endif
	diag ("");

#ifdef	BANNER
	diag (BANNER);
#else
	diag ("PicOS v" SYSVER_S "/" SYSVER_R
#ifdef	SYSVER_B
		"-" SYSVER_B
#endif
        	", (C) Olsonet Communications, 2002-2009");
	diag ("Leftover RAM: %d bytes", (word)STACK_END - (word)(&__bss_end));
#endif
	dbg_1 (0x1000 | SYSVER_X);
	dbg_1 ((word)STACK_END - (word)(&__bss_end)); // RAM in bytes

	for_all_tasks (p)
		/* Mark all task table entries as available */
		p->code = NULL;

	/* Processes can be created past this point */

#if MAX_DEVICES
	/* Initialize devices */
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

uart_t	zz_uart [N_UARTS];

static void preinit_uart () {

#ifdef	UART_PREINIT_A
	// UART_A
	UART_PREINIT_A;
#if UART_INPUT_FLOW_CONTROL || UART_OUTPUT_FLOW_CONTROL
	UART_PREINIT_A_FC;
#endif
	_BIS (UCTL_A, SWRST);
	UTCTL_A = UART_UTCTL;
	UBR0_A = UART_UBR0;
	UBR1_A = UART_UBR1;
	UMCTL_A = UART_UMCTL;
	_BIS (IFG_A, UTXIFG_A);
	_BIS (ME_A, UTXE_A + URXE_A);
	_BIS (UCTL_A, UART_UCTL_CHAR | UART_UCTL_PENA | UART_UCTL_PEV);
	_BIC (UCTL_A, SWRST);
#if UART_RATE_SETTABLE
	zz_uart [0] . flags = UART_RATE_INDEX;
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
	_BIS (UCTL_B, SWRST);
	UTCTL_B = UART_UTCTL;
	UBR0_B = UART_UBR0;
	UBR1_B = UART_UBR1;
	UMCTL_B = UART_UMCTL;
	_BIS (IFG_B, UTXIFG_B);
	_BIS (ME_B, UTXE_B + URXE_B);
	_BIS (UCTL_B, UART_UCTL_CHAR | UART_UCTL_PENA | UART_UCTL_PEV);
	_BIC (UCTL_B, SWRST);
#if UART_RATE_SETTABLE
	zz_uart [1] . flags = UART_RATE_INDEX;
#endif

#endif	/* UART_PREINIT_B */
}

#if UART_RATE_SETTABLE

typedef struct	{

	word rate;
	byte A, B;
} uart_rate_t;

static const uart_rate_t urates [] = {

#if UART_CLOCK_RATE == 32768

    { 12, 0x1B, 0x03 },
    { 24, 0x0D, 0x6B },
    { 48, 0x06, 0x6F },
    { 96, 0x03, 0x4A }
#else
    {	12, (UART_CLOCK_RATE/  1200) % 256, (UART_CLOCK_RATE/  1200) / 256 },
    {	24, (UART_CLOCK_RATE/  2400) % 256, (UART_CLOCK_RATE/  2400) / 256 },
    {	48, (UART_CLOCK_RATE/  4800) % 256, (UART_CLOCK_RATE/  4800) / 256 },
    {	96, (UART_CLOCK_RATE/  9600) % 256, (UART_CLOCK_RATE/  9600) / 256 },
    {  144, (UART_CLOCK_RATE/ 14400) % 256, (UART_CLOCK_RATE/ 14400) / 256 },
    {  192, (UART_CLOCK_RATE/ 19200) % 256, (UART_CLOCK_RATE/ 19200) / 256 },
    {  288, (UART_CLOCK_RATE/ 28800) % 256, (UART_CLOCK_RATE/ 28800) / 256 },
    {  384, (UART_CLOCK_RATE/ 38400) % 256, (UART_CLOCK_RATE/ 38400) / 256 },
    {  768, (UART_CLOCK_RATE/ 76800) % 256, (UART_CLOCK_RATE/ 76800) / 256 },
    { 1152, (UART_CLOCK_RATE/115200) % 256, (UART_CLOCK_RATE/115200) / 256 },
    { 2560, (UART_CLOCK_RATE/256000) % 256, (UART_CLOCK_RATE/256000) / 256 }
#endif

};

#define	N_RATES		(sizeof(urates) / sizeof(uart_rate_t))

Boolean zz_uart_setrate (word rate, uart_t *ua) {

	byte j, saveIE;

	for (j = 0; j < N_RATES; j++) {
		if (rate == urates [j] . rate) {
			// Found
#if N_UARTS > 1
			if (ua != zz_uart) {
				// UART_B
				UBR0_B = urates [j].A;
#if UART_CLOCK_RATE == 32768
				UBR1_B = 0;
				UMCTL_B = urates [j].B;
#else
				UBR1_B = urates [j].B;
				UMCTL_B = 0;
#endif
			} else {
#endif	/* N_UARTS */
				UBR0_A = urates [j].A;
#if UART_CLOCK_RATE == 32768
				UBR1_A = 0;
				UMCTL_A = urates [j].B;
#else
				UBR1_A = urates [j].B;
				UMCTL_A = 0;
#endif

#if N_UARTS > 1
			}
#endif
			ua->flags = (ua->flags & ~UART_RATE_MASK) | j;
			return YES;
		}
	}
	return NO;
}

word zz_uart_getrate (uart_t *ua) {

	return urates [ua->flags & UART_RATE_MASK] . rate;
}

#endif 	/* UART_RATE_SETTABLE */

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

#endif	/* UART_DRIVER || UART_TCV */

#if	UART_DRIVER
/* ======== */
/* The UART */
/* ======== */

#define uart_xpending(u)	__ualt(u, IFG_B&UTXIFG_B, IFG_A&UTXIFG_A)

#define	uart_write8(u,w)	__usel (u, TXBUF_B = (w), TXBUF_A = (w))

#define uart_enable_write_int(u) __usel (u, uart_b_enable_write_int, \
						uart_a_enable_write_int)

#define uart_disable_int(u)	__usel (u, uart_b_disable_int, \
						uart_a_disable_int)

#define uart_disable_read_int(u) __usel (u, uart_b_disable_read_int, \
						uart_a_disable_read_int)

#define uart_disable_write_int(u) __usel (u, uart_b_disable_write_int, \
						uart_a_disable_write_int)

#define uart_rpending(u)       __ualt (u, IFG_B&URXIFG_B, IFG_A&URXIFG_A)

#define	uart_read(u)		__ualt (u, RXBUF_B, RXBUF_A)

#define uart_enable_read_int(u)	__usel (u, uart_b_enable_read_int, \
						uart_a_enable_read_int)

static void uart_lock (uart_t*), uart_unlock (uart_t*);

/* ======================= */
/* Common request function */
/* ======================= */
static INLINE int ioreq_uart (uart_t *u, int operation, char *buf, int len) {

	switch (operation) {

		case READ:

			if ((u->flags & UART_FLAGS_LOCK)) {
				// Raw operation, interrupts disabled
				if (len && uart_rpending (u)) {
					*buf = uart_read (u);
					return 1;
				}
				return 0;
			} else {

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
#if UART_INPUT_FLOW_CONTROL
					if (u->ib_count <=
						UART_INPUT_BUFFER_LENGTH/2)
							UART_SET_RTR;
#endif
					*buf++ = u->in [u->ib_out];
					if (++(u->ib_out) ==
						UART_INPUT_BUFFER_LENGTH)
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
#if UART_INPUT_FLOW_CONTROL
					UART_SET_RTR;
#endif
					return 1;
				}
				uart_disable_read_int (u);
				if ((u->flags & UART_FLAGS_IN))
					// Account for the race
					goto R_redo;
				// The buffer is empty
				return -2;
#endif
			}
				
		case WRITE:

			if ((u->flags & UART_FLAGS_LOCK)) {
				if (len && uart_xpending (u)) {
					uart_write8 (u, *buf);
					return 1;
				}
				return 0;
			} else {
#if UART_OUTPUT_FLOW_CONTROL
				if (!UART_TST_RTR) {
					return - 16 - 2
				}
#endif
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
			}
					
		case NONE:

			// Cleanup
#if UART_INPUT_BUFFER_LENGTH < 2
			if ((u->flags & UART_FLAGS_IN) == 0) {
				uart_enable_read_int (u);
#if UART_INPUT_FLOW_CONTROL
				UART_SET_RTR;
#endif
			}
#else
				uart_enable_read_int (u);
#endif
			if ((u->flags & UART_FLAGS_OUT))
				uart_enable_write_int (u);

			return 0;

		case CONTROL:

			if (len == UART_CNTRL_LCK) {
				if (*buf)
					uart_lock (u);
				else
					uart_unlock (u);
				return 1;
			}
#if UART_RATE_SETTABLE
			if (len == UART_CNTRL_SETRATE) {
				if (zz_uart_setrate (*((word*)buf), u))
						return 1;
				syserror (EREQPAR, "uart rate");
			}
			if (len == UART_CNTRL_GETRATE) {
				*((word*)buf) = zz_uart_getrate (u);
				return 1;
			}
#endif
			/* Fall through */
		default:
			syserror (ENOOPER, "ioreq_uart");
			/* No return */
			return 0;
	}
}

static void uart_unlock (uart_t *u) {
/* ============================================ */
/* Start up normal (interrupt-driven) operation */
/* ============================================ */
	u->flags &= UART_RATE_MASK;
	uart_enable_read_int (u);
	// uart_enable_write_int (u);
}

static void uart_lock (uart_t *u) {
/* ================================= */
/* Direct (interrupt-less) operation */
/* ================================= */
	if ((u->flags & UART_FLAGS_LOCK) == 0) {
		uart_disable_int (u);
		u->flags = UART_FLAGS_LOCK;
	}
}

/* ========================== */
/* Interrupt service routines */
/* ========================== */
interrupt (UART_A_TX_VECTOR) uart_a_tx_int (void) {

	// Disable until a character arrival
	_BIC (IE_A, UTXIE_A);

#if NESTED_INTERRUPTS
	// Enable other interrupts. Note: we do this only for UART0.
	sti;
#endif
	RISE_N_SHINE;

	if ((zz_uart [0] . flags & UART_FLAGS_OUT) == 0) {
		// Keep the interrupt pending
		_BIS (IFG_A, UTXIFG_A);
	} else {
		_BIC (zz_uart [0] . flags, UART_FLAGS_OUT);
		TXBUF_A = zz_uart [0] . out;
	}
	i_trigger (ETYPE_IO, devevent (UART_A, WRITE));
	RTNI;
}

interrupt (UART_A_RX_VECTOR) uart_a_rx_int (void) {

#define	ua	(zz_uart + 0)

	_BIC (IE_A, URXIE_A);

#if NESTED_INTERRUPTS
	// Enable other interrupts (UART0 only)
	sti;
#endif

#if UART_INPUT_BUFFER_LENGTH > 1

	if (ua->ib_count <= UART_INPUT_BUFFER_LENGTH) {
		// Not full
		ua->in [ua->ib_in] = RXBUF_A;
		if (++(ua->ib_in) == UART_INPUT_BUFFER_LENGTH)
			ua->ib_in = 0;
		ua->ib_count++;
#if UART_INPUT_FLOW_CONTROL
		if (ua->ib_count > UART_INPUT_BUFFER_LENGTH/2)
			UART_CLR_RTR;
#endif
	}

	if ((ua -> flags & UART_FLAGS_IN)) {
		RISE_N_SHINE;
		i_trigger (ETYPE_IO, devevent (UART_A, READ));
	}

#if NESTED_INTERRUPTS
	cli;
#endif
	_BIS (IE_A, URXIE_A);

#else	/* UART_INPUT_BUFFER_LENGTH */

#if UART_INPUT_FLOW_CONTROL
	UART_CLR_RTR;
#endif
	RISE_N_SHINE;

	if ((ua -> flags & UART_FLAGS_IN)) {
		// Keep the interrupt pending
		_BIS (IFG_A, URXIFG_A);
	} else {
		_BIS (ua -> flags, UART_FLAGS_IN);
		ua -> in = RXBUF_A;
	}
	i_trigger (ETYPE_IO, devevent (UART_A, READ));
#endif
	RTNI;

}

static int ioreq_uart_a (int op, char *b, int len) {
	return ioreq_uart (ua, op, b, len);
}

#undef ua

#if UART_DRIVER > 1

interrupt (UART_B_TX_VECTOR) uart_b_tx_int (void) {
	RISE_N_SHINE;

	if ((zz_uart [1] . flags & UART_FLAGS_OUT) == 0) {
		// Keep the interrupt pending
		_BIS (IFG_B, UTXIFG_B);
	} else {
		_BIC (zz_uart [1] . flags, UART_FLAGS_OUT);
		TXBUF_B = zz_uart [1] . out;
	}
	// Disable until a character arrival
	_BIC (IE_B, UTXIE_B);
	i_trigger (ETYPE_IO, devevent (UART_B, WRITE));
	RTNI;
}

interrupt (UART_B_RX_VECTOR) uart_b_rx_int (void) {

#define	ua	(zz_uart + 1)

#if UART_INPUT_BUFFER_LENGTH > 1

	if (ua->ib_count <= UART_INPUT_BUFFER_LENGTH) {
		// Not full
		ua->in [ua->ib_in] = RXBUF_B;
		if (++(ua->ib_in) == UART_INPUT_BUFFER_LENGTH)
			ua->ib_in = 0;
		ua->ib_count++;
	}
	if ((ua -> flags & UART_FLAGS_IN)) {
		RISE_N_SHINE;
		i_trigger (ETYPE_IO, devevent (UART_B, READ));
	}
#else
	RISE_N_SHINE;

	if ((ua -> flags & UART_FLAGS_IN)) {
		// Keep the interrupt pending
		_BIS (IFG_B, URXIFG_B);
	} else {
		_BIS (ua -> flags, UART_FLAGS_IN);
		ua -> in = RXBUF_B;
	}
	_BIC (IE_B, URXIE_B);
	i_trigger (ETYPE_IO, devevent (UART_B, READ));
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
	zz_uart [devnum] . ib_count = zz_uart [devnum] . ib_in =
		zz_uart [devnum] . ib_out = 0;
#endif

#if UART_INPUT_FLOW_CONTROL
	UART_SET_RTR;
#endif

#if UART_DRIVER > 1
	zz_uart [devnum] . selector = devnum;
#endif
	_BIS (zz_uart [devnum] . flags, UART_FLAGS_LOCK);
	uart_unlock (zz_uart + devnum);
}
/* =========== */
/* UART DRIVER */
/* =========== */
#endif

/* ============== */
/* End of drivers */
/* ============== */
