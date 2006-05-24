#include "kernel.h"

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "irq_timer_headers.h"

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

#ifndef	UART_INPUT_FLOW_CONTROL
#define	UART_INPUT_FLOW_CONTROL		0
#endif

#ifndef	UART_OUTPUT_FLOW_CONTROL
#define	UART_OUTPUT_FLOW_CONTROL	0
#endif

#if	UART_INPUT_FLOW_CONTROL || UART_OUTPUT_FLOW_CONTROL
#if	UART_DRIVER > 1
#error	"flow control is only available when UART_DRIVER == 1"
#endif

#define	set_ready_to_receive	_BIS (P3OUT, 0x80)
#define	clear_ready_to_receive	_BIC (P3OUT, 0x80)
#define	peer_ready_to_receive	((P3IN & 0x40) != 0)

#endif

static void	devinit_uart (int);

#else	/* UART_DRIVER */

#define	N_UARTS	UART_TCV

#endif	/* UART_DRIVER */

#endif  /* UART_DRIVER || UART_TCV */

extern void	__bss_end;

const static devinit_t devinit [MAX_DEVICES] = {
/* === */
#if	UART_DRIVER
		{ devinit_uart,	 0 },
#else
		{ NULL, 0 },
#endif
/* === */
#if	UART_DRIVER > 1
		{ devinit_uart,	 1 },
#else
		{ NULL, 0 }
#endif
	 };

static void ssm_init (void), mem_init (void), ios_init (void);


void clockdown (void) {
	TBCCR0 = TIMER_B_INIT_LOW;	// 1 or 16 ticks per second
}

void clockup (void) {
	TBCCR0 = TIMER_B_INIT_HIGH;	// 1024 ticks per second
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
// There is no general ADC interface at the moment.
// ADC is used internally for Radio RSSI.
#endif

void udelay (register word n) {
/* =================================== */
/* n should be roughly in microseconds */
/* =================================== */
	__asm__ __volatile__ (

#ifdef	__MSP430_449__
		"1:\n"
		" nop\n"
		" nop\n"
		" nop\n"
		" nop\n"
		" nop\n"
		" dec %[n]\n"
		" jne 1b\n"
#endif
#ifdef	__MSP430_14x__
		"1:\n"
		" nop\n"
		" nop\n"
		" dec %[n]\n"
		" jne 1b\n"
#endif
			: [n] "+r"(n));
}

void mdelay (word n) {
/* ============ */
/* milliseconds */
/* ============ */
	while (n--)
		udelay (995);
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
	// Run the scheduler
#include "scheduler.h"
}

#if DIAG_MESSAGES > 1
void zzz_syserror (int ec, const char *m) {
#ifdef DUMP_MEM
	dmp_mem ();
#endif
	diag ("SYSTEM ERROR: %x, %s", ec, m);
#else
void zzz_syserror (int ec) {
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
	hard_reset;
#else	/* RESET_ON_SYSERR */

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

	P1IE = P2IE = 0x00;
	P1IES = P2IES = 0x00;
	P1IFG = P2IFG = 0x00;
	// For power saving, the default setting of all registers
	// is non-special + output.
	P1SEL = P2SEL = P3SEL = P4SEL = P5SEL = P6SEL = 0x00;
	// The value should be "don't care". We set it high, essentially for
	// compatibility with the RESET key on Versa, which should be high
	// initially.
	P1OUT = P2OUT = P3OUT = P4OUT = P5OUT = P6OUT = 0xff;
	P1DIR = P2DIR = P3DIR = P4DIR = P5DIR = P6DIR = 0xff;

#ifdef	__MSP430_449__
	// This one needs the capacitance setting
	_BIS (FLL_CTL0, XCAP18PF);
#endif
	// Power status
	// powerup ();

	// Set up the CPU clock

#ifdef	__MSP430_14x__
	// Maximum DCO frequency
	DCOCTL = DCO2 + DCO1 + DCO0;
	BCSCTL1 = RSEL2 + RSEL1 + RSEL0 + XT2OFF

#if	CRYSTAL_RATE != 32768
	// We are using a high-speed crystal for XT1
		+ XTS
#endif

	;
	// Measured MCLK is ca. 4.5 MHz

#if 	CRYSTAL2_RATE
	// Assign SMCLK to XTL2
	BCSCTL2 = SELM_DCOCLK | SELS;
#endif

#endif	/* 14x */

#ifdef	__MSP430_449__
	//
	// This is the best I could get through rather tedious experiments
	//
	SCFQCTL = 63;			// ACLK * 64 = 32768 * 64 = 2097152 Hz
	SCFI0 =	FLLD_2 | FN_2;		// ... * 2 = 4,194,304
	// SCFI0 =	FLLD_4 | FN_3;	// ... * 4 = 8,388,608 Hz, (2 -- 17.9)
	_BIS (FLL_CTL0, DCOPLUS);	// Mutiplier active
	// This yields 4MHz + for the CPU clock
#endif	/* 449 */
	// Note: in all cases, SMLCK is set to run at DCO frequency

	// Add here code for other CPU types

	// System timer
	TBCTL = TBSSEL0 | TBCLR; 	// ACLK source
	_BIS (TBCTL, ID0 | ID1);	// divided by 8 = 4096 ticks/sec

	// Select power up and high clock rate
	powerup ();

	// Start it in up mode, interrupts still disabled
	_BIS (TBCTL, MC0);
}

/* =============== */
/* Timer interrupt */
/* =============== */
interrupt (TIMERB0_VECTOR) timer_int () {

// Make interrupts visible on P1.2
// _BIS (P1OUT, 0x04);

#if	STACK_GUARD
	if (*(((word*)STACK_END) - 1) != STACK_SENTINEL)
		syserror (ESTACK, "timer_int");
#endif

	if (TBCCR0 == TIMER_B_INIT_HIGH) {
		// Power up
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

	// Room for extras
#include "irq_timer.h"

		// Run the scheduler at least once every second - to
		// keep the second clock up to date
		if ((zz_lostk & 1024) || (zz_mintk && zz_mintk <= zz_lostk))
			RISE_N_SHINE;
// -------------------
// _BIC (P1OUT, 0x04);
// -------------------
		return;
	}

	// Here we are running in the slooooow mode
#define	UTIMS_CASCADE(x) if (zz_utims [x]) {\
			     if (*(zz_utims [x]))\
				*(zz_utims [x]) = *(zz_utims [x]) > JIFFIES ?\
				    *(zz_utims [x]) - JIFFIES : 0
	UTIMS_CASCADE(0);
	UTIMS_CASCADE(1);
	UTIMS_CASCADE(2);
	UTIMS_CASCADE(3);
	}}}}

	zz_lostk += JIFFIES/TIMER_B_LOW_PER_SEC;

#if TIMER_B_LOW_PER_SEC > 1
	if ((zz_lostk & 1024) || (zz_mintk && zz_mintk <= zz_lostk))
#endif
		RISE_N_SHINE;
// -------------------
// _BIC (P1OUT, 0x04);
// -------------------
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
#endif
	powerdown ();
	// Save clock state
	saveLostK = zz_lostk;

	while (nsec) {
		zz_lostk = 0;
		_BIS_SR (LPM3_bits + GIE);
		cli;
		nsec--;
	}

	P1IE = saveP1IE;
	P2IE = saveP2IE;

#if UART_DRIVER || UART_TCV
	// Reset the UART to get it back to normal
	_BIS (UCTL0, SWRST);
	_BIC (UCTL0, SWRST);
#if N_UARTS > 1
	_BIS (UCTL1, SWRST);
	_BIC (UCTL1, SWRST);
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

	int i;
	pcb_t *p;

#if EEPROM_DRIVER
	zz_ee_init ();
#endif

#if INFO_FLASH
	zz_if_init ();
#endif

#if	UART_DRIVER || UART_TCV
	// A UART is configured, initialize it beforehand without enabling
	// anyting, which is up to the driver plugin. We just want to be able
	// to use diag.
	preinit_uart ();
#endif
	diag ("");

#ifdef	BANNER
	diag (BANNER);
#else
	diag ("PicOS v" SYSVERSION ", "
        	"Copyright (C) Olsonet Communications, 2002-2006");
	diag ("Leftover RAM: %d bytes", (word)STACK_END - (word)(&__bss_end));
#endif
	dbg_1 (0x1000 | SYSVER_B);
	dbg_1 ((word)STACK_END - (word)(&__bss_end)); // RAM in bytes

	for_all_tasks (p)
		/* Mark all task table entries as available */
		p->code = NULL;

	/* Processes can be created past this point */

	/* Initialize devices */
	for (i = UART; i < MAX_DEVICES; i++)
		if (devinit [i] . init != NULL)
			devinit [i] . init (devinit [i] . param);

	/* Make SMCLK/MCLK available on P5.5, P5.4 */
	_BIS (P5OUT, 0x30);
	_BIS (P5SEL, 0x30);
}

/* ------------------------------------------------------------------------ */
/* ============================ DEVICE DRIVERS ============================ */
/* ------------------------------------------------------------------------ */

#if	UART_DRIVER || UART_TCV

uart_t	zz_uart [N_UARTS];

#if CRYSTAL2_RATE
// SMCLK
#define	utctl		SSEL1
#define	UART_CLOCK_RATE	CRYSTAL2_RATE
#else
// ACLK
#define	utctl		SSEL0
#define	UART_CLOCK_RATE	CRYSTAL_RATE
#endif

#if	UART_CLOCK_RATE == 32768
// This is the standard (or perhaps not any more?)
#define	ubr1		0

#if UART_RATE == 1200
#define	ubr0		0x1B
#define	umctl		0x03
#endif

#if UART_RATE == 2400
#define	ubr0		0x0D
#define	umctl		0x6B
#endif

#if UART_RATE == 4800
#define	ubr0		0x06
#define	umctl		0x6F
#endif

#if UART_RATE == 9600
#define	ubr0		0x03
#define	umctl		0x4A
#endif

#ifndef ubr0
#error "Illegal UART_RATE, can be 1200, 2400, 4800, 9600"
#endif

#else	/* UART_CLOCK_RATE > 32768 */

#if UART_RATE == 1200
#define	UART_RATE_INDEX	0
#endif
#if UART_RATE == 2400
#define	UART_RATE_INDEX	1
#endif
#if UART_RATE == 4800
#define	UART_RATE_INDEX	2
#endif
#if UART_RATE == 9600
#define	UART_RATE_INDEX	3
#endif
#if UART_RATE == 14400
#define	UART_RATE_INDEX	4
#endif
#if UART_RATE == 19200
#define	UART_RATE_INDEX	5
#endif
#if UART_RATE == 28800
#define	UART_RATE_INDEX	6
#endif
#if UART_RATE == 38400
#define	UART_RATE_INDEX	7
#endif

#ifndef	UART_RATE_INDEX
#error "Illegal UART_RATE"
#endif

// No need to use corrections for high-speed crystals
#define	umctl		0
#define	ubr0		((UART_CLOCK_RATE/UART_RATE) % 256)
#define	ubr1		((UART_CLOCK_RATE/UART_RATE) / 256)

#endif	/* UART_CLOCK_RATE */

#if UART_BITS == 8

#define	uctl_char	CHAR
#define	uctl_pena	0
#define	uctl_pev	0

#else	/* UART_BITS == 7 */

#define	uctl_char	0
#define	uctl_pena	PENA

#if UART_PARITY == 0
#define	uctl_pev	PEV
#else
#define	uctl_pev	0
#endif

#endif	/* UART_BITS */

static void preinit_uart () {

	// UART_A
	_BIC (P3DIR, 0x26);
	_BIS (P3SEL, 0x30);	// 4, 5 special function
#if UART_INPUT_FLOW_CONTROL || UART_OUTPUT_FLOW_CONTROL
	_BIC (P3SEL, 0xc0);	// Standard use P3.6, P3.7
	_BIS (P3DIR, 0x80);	// P3.7 == CTS, goes out
#endif
	_BIS (UCTL0, SWRST);
	_BIC (UTCTL0, SSEL1 + SSEL0);
	_BIS (UTCTL0, utctl);
	UBR00 = ubr0;
	UBR10 = ubr1;
	UMCTL0 = umctl;
	_BIS (IFG1, UTXIFG0);
	_BIS (ME1, UTXE0 + URXE0);
	_BIS (UCTL0, uctl_char | uctl_pena | uctl_pev);
	_BIC (UCTL0, SWRST);
#if UART_RATE_SETTABLE
	zz_uart [0] . flags = UART_RATE_INDEX;
#endif

#if N_UARTS > 1
	// UART_B
	_BIC (P3DIR, 0x89);
	_BIS (P3SEL, 0xc0);	// 6, 7 special function
	_BIS (UCTL1, SWRST);
	_BIC (UTCTL1, SSEL1 + SSEL0);
	_BIS (UTCTL1, utctl);
	UBR01 = ubr0;
	UBR11 = ubr1;
	UMCTL1 = umctl;
	_BIS (IFG2, UTXIFG1);
	_BIS (ME2, UTXE1 + URXE1);
	_BIS (UCTL1, uctl_char | uctl_pena | uctl_pev);
	_BIC (UCTL1, SWRST);
#if UART_RATE_SETTABLE
	zz_uart [1] . flags = UART_RATE_INDEX;
#endif

#endif	/* N_UARTS */
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
    {	12, (UART_CLOCK_RATE/ 1200) % 256, (UART_CLOCK_RATE/ 1200) / 256 },
    {	24, (UART_CLOCK_RATE/ 2400) % 256, (UART_CLOCK_RATE/ 2400) / 256 },
    {	48, (UART_CLOCK_RATE/ 4800) % 256, (UART_CLOCK_RATE/ 4800) / 256 },
    {	96, (UART_CLOCK_RATE/ 9600) % 256, (UART_CLOCK_RATE/ 9600) / 256 },
    {  144, (UART_CLOCK_RATE/14400) % 256, (UART_CLOCK_RATE/14400) / 256 },
    {  192, (UART_CLOCK_RATE/19200) % 256, (UART_CLOCK_RATE/19200) / 256 },
    {  288, (UART_CLOCK_RATE/28800) % 256, (UART_CLOCK_RATE/28800) / 256 },
    {  384, (UART_CLOCK_RATE/38400) % 256, (UART_CLOCK_RATE/38400) / 256 }
#endif

};

#define	N_RATES		(sizeof(urates) / sizeof(uart_rate_t))

bool zz_uart_setrate (word rate, uart_t *ua) {

	byte j;

	for (j = 0; j < N_RATES; j++) {
		if (rate == urates [j] . rate) {
			// Found
#if N_UARTS > 1
			if (ua != zz_uart) {
				// UART_B
				UBR01 = urates [j].A;
#if UART_CLOCK_RATE == 32768
				UBR11 = 0;
				UMCTL1 = urates [j].B;
#else
				UBR01 = urates [j].B;
				UMCTL1 = 0;
#endif
			} else {
#endif	/* N_UARTS */
				UBR00 = urates [j].A;
#if UART_CLOCK_RATE == 32768
				UBR10 = 0;
				UMCTL0 = urates [j].B;
#else
				UBR10 = urates [j].B;
				UMCTL0 = 0;
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

#define uart_xpending(u)	((u)->selector?(IFG2&UTXIFG1):(IFG1&UTXIFG0))

#define	uart_write8(u,w)	do { \
					if ((u)->selector) \
						TXBUF1 = (w); \
					else \
						TXBUF0 = (w); \
				} while (0)

#define uart_enable_write_int(u) \
				do { \
					if ((u)->selector) \
						uart_b_enable_write_int; \
					else \
						uart_a_enable_write_int; \
				} while (0)

#define uart_disable_int(u)	do { \
					if ((u)->selector) \
						uart_b_disable_int; \
					else \
						uart_a_disable_int; \
				} while (0)

#define uart_disable_read_int(u) \
				do { \
					if ((u)->selector) \
						uart_b_disable_read_int; \
					else \
						uart_a_disable_read_int; \
				} while (0)

#define uart_disable_write_int(u) \
				do { \
					if ((u)->selector) \
						uart_b_disable_write_int; \
					else \
						uart_a_disable_write_int; \
				} while (0)

#define uart_rpending(u)	((u)->selector?(IFG2&URXIFG1):(IFG1&URXIFG0))

#define	uart_read(u)		(((u)->selector)?RXBUF1:RXBUF0)

#define uart_enable_read_int(u)	do { \
					if ((u)->selector) \
						uart_b_enable_read_int; \
					else \
						uart_a_enable_read_int; \
				} while (0)

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
							set_ready_to_receive;
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
					set_ready_to_receive;
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
				if (!peer_ready_to_receive) {
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
				set_ready_to_receive;
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
	u->flags = 0;
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
interrupt (UART0TX_VECTOR) uart0tx_int (void) {
	RISE_N_SHINE;

	if ((zz_uart [0] . flags & UART_FLAGS_OUT) == 0) {
		// Keep the interrupt pending
		_BIS (IFG1, UTXIFG0);
	} else {
		_BIC (zz_uart [0] . flags, UART_FLAGS_OUT);
		TXBUF0 = zz_uart [0] . out;
	}
	// Disable until a character arrival
	_BIC (IE1, UTXIE0);
	i_trigger (ETYPE_IO, devevent (UART_A, WRITE));
}

interrupt (UART0RX_VECTOR) uart0rx_int (void) {

#define	ua	(zz_uart + 0)

#if UART_INPUT_BUFFER_LENGTH > 1

	if (ua->ib_count <= UART_INPUT_BUFFER_LENGTH) {
		// Not full
		ua->in [ua->ib_in] = RXBUF0;
		if (++(ua->ib_in) == UART_INPUT_BUFFER_LENGTH)
			ua->ib_in = 0;
		ua->ib_count++;
#if UART_INPUT_FLOW_CONTROL
		if (ua->ib_count > UART_INPUT_BUFFER_LENGTH/2)
			clear_ready_to_receive;
#endif
	}

	if ((ua -> flags & UART_FLAGS_IN)) {
		RISE_N_SHINE;
		i_trigger (ETYPE_IO, devevent (UART_A, READ));
	}
#else

#if UART_INPUT_FLOW_CONTROL
	clear_ready_to_receive;
#endif
	RISE_N_SHINE;

	if ((ua -> flags & UART_FLAGS_IN)) {
		// Keep the interrupt pending
		_BIS (IFG1, URXIFG0);
	} else {
		_BIS (ua -> flags, UART_FLAGS_IN);
		ua -> in = RXBUF0;
	}
	_BIC (IE1, URXIE0);
	i_trigger (ETYPE_IO, devevent (UART_A, READ));
#endif

}

static int ioreq_uart_a (int op, char *b, int len) {
	return ioreq_uart (ua, op, b, len);
}

#undef ua

#if UART_DRIVER > 1

interrupt (UART1TX_VECTOR) uart1tx_int (void) {
	RISE_N_SHINE;

	if ((zz_uart [1] . flags & UART_FLAGS_OUT) == 0) {
		// Keep the interrupt pending
		_BIS (IFG2, UTXIFG1);
	} else {
		_BIC (zz_uart [1] . flags, UART_FLAGS_OUT);
		TXBUF1 = zz_uart [1] . out;
	}
	// Disable until a character arrival
	_BIC (IE2, UTXIE1);
	i_trigger (ETYPE_IO, devevent (UART_B, WRITE));
}

interrupt (UART1RX_VECTOR) uart1rx_int (void) {

#define	ua	(zz_uart + 1)

#if UART_INPUT_BUFFER_LENGTH > 1

	if (ua->ib_count <= UART_INPUT_BUFFER_LENGTH) {
		// Not full
		ua->in [ua->ib_in] = RXBUF1;
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
		_BIS (IFG1, URXIFG1);
	} else {
		_BIS (ua -> flags, UART_FLAGS_IN);
		ua -> in = RXBUF1;
	}
	_BIC (IE2, URXIE1);
	i_trigger (ETYPE_IO, devevent (UART_B, READ));
#endif
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
	set_ready_to_receive;
#endif
	zz_uart [devnum] . selector = devnum;
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
