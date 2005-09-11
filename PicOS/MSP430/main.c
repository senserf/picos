#include "kernel.h"

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

extern 			pcb_t		*zz_curr;
extern 			word  		zz_mintk;
extern 	volatile 	word 		zz_lostk;
extern 			address	zz_utims [MAX_UTIMERS];
extern	void		_reset_vector__;

word			zz_restart_sp;

#define	STACK_SENTINEL	0xB779

/* ====================================================================== */
/* Accept UART rates above 9600. They use SMCLK, which is unreliable, and */
/* thus involve a tricky calibration.                                     */
/* ====================================================================== */

void	zz_malloc_init (void);

/* ========================== */
/* Device driver initializers */
/* ========================== */
#if	UART_DRIVER

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
#endif	/* UART_DRIVER */

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

void powerdown (void) {
	zz_systat.pdmode = 1;
}

void powerup (void) {
	zz_systat.pdmode = 0;
}

void clockdown (void) {
	TBCCR0 = TIMER_B_INIT_LOW;	// 1 or 16 ticks per second
}

void clockup (void) {
	TBCCR0 = TIMER_B_INIT_HIGH;	// 1024 ticks per second
}

void reset (void) {

	cli_tim;
	__asm__ __volatile__("br #_reset_vector__"::);
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
#ifdef	__MSP430_149__
		"1:\n"
		" nop\n"
		" nop\n"
		" dec %[n]\n"
		" jne 1b\n"
#endif
#ifdef	__MSP430_148__
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

void leds (word c) {
/*
 * This is set up for IM2100/DM2100 with 3 leds attached to P4.1-P4.3,
 * being switched on by setting the ports to low impedance. P4.0 is not
 * used, although the virtual LED number zero is assumed to be there.
 */
	int i;

	for (i = 1; i < 4; i++)
		if (c & (1 << i))
			_BIS (P4DIR, (1 << i));
		else
			_BIC (P6OUT, (1 << i));
}

word switches (void) {
/*
 * No switches are used at present
 */
	return 0;
}

#if	DIAG_MESSAGES > 1
void zzz_syserror (int ec, const char *m) {
	diag ("SYSTEM ERROR: %x, %s", ec, m);
#else
void zzz_syserror (int ec) {
	diag ("SYSTEM ERROR: %x", ec);
#endif
	while (1)
		_BIS_SR (LPM4_bits);
}

static void ssm_init () {

	// Disable watchdog timer
	WDTCTL = WDTPW + WDTHOLD;

#ifdef	__MSP430_449__
	// This one needs the capacitance setting
	_BIS (FLL_CTL0, XCAP18PF);
#endif
	// Power status
	powerup ();

	// Set up the CPU clock

#ifdef	__MSP430_148__
	// Maximum DCO frequency
	DCOCTL = DCO2 + DCO1 + DCO0;
	BCSCTL1 = RSEL2 + RSEL1 + RSEL0 + XT2OFF

#if	CRYSTAL_RATE != 32768
	// We are using a high-speed crystal
		+ XTS
#endif

	;
	// Measured MCLK is ca. 4.5 MHz
#endif	/* 148 */

#ifdef	__MSP430_149__
	// Maximum DCO frequency
	DCOCTL = DCO2 + DCO1 + DCO0;
	BCSCTL1 = RSEL2 + RSEL1 + RSEL0 + XT2OFF;
	// Measured MCLK is ca. 4.5 MHz
#endif	/* 149 */

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

	// Select high clock rate
	clockup ();

	// Start it in up mode, interrupts still disabled
	_BIS (TBCTL, MC0);
}

/* =============== */
/* Timer interrupt */
/* =============== */
interrupt (TIMERB0_VECTOR) timer_int () {

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

#if	RADIO_INTERRUPTS
	/* 
	 * This is legacy stuff. I don't think we will ever use it directly
	 * in MSP430.
	 */
#if	RADIO_TYPE != RADIO_XEMICS

		if (rcvhigh && !(zzz_radiostat & 1)) {
			if (zzr_xwait
#if	RADIO_INTERRUPTS > 1
			    && zzz_last_sense < RADIO_INTERRUPTS
#endif
				/* Receiver waiting */
				zzr_xwait -> Status = zzr_xstate << 4;
				zzr_xwait = NULL;
				zzz_last_sense = 0;
				/* Wake up the scheduler */
				RISE_N_SHINE;
				return;
			}
			zzz_last_sense = 0;
		} else {
			if (zzz_last_sense != MAX_INT)
				zzz_last_sense++;
		}
#else	/* XEMICS */
		if (zzz_last_sense != MAX_INT)
			zzz_last_sense++;
#endif	/* XEMICS */
#endif	/* RADIO_INTERRUPTS */

		// Run the scheduler at least once every second - to
		// keep the second clock up to date
		if ((zz_lostk & 1024) || (zz_mintk && zz_mintk <= zz_lostk))
			RISE_N_SHINE;

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
}

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

	for_all_tasks (p)
		/* Mark all task table entries as available */
		p->code = NULL;

#if	UART_DRIVER
	/* UART_A is initialized first, to enable diagnostic output */
	devinit [UART] . init (devinit [UART] . param);
	diag ("\r\nPicOS v" SYSVERSION ", "
        	"Copyright (C) Olsonet Communications, 2002-2005");
	diag ("Leftover RAM: %d bytes", (word)STACK_END - (word)(&__bss_end));
#if	SDRAM_PRESENT
	// Stub
#endif
#endif
	/* Initialize other devices and create their drivers */
	for (i = UART+1; i < MAX_DEVICES; i++)
		if (devinit [i] . init != NULL)
			devinit [i] . init (devinit [i] . param);

	/* Make SMCLK available on P5.5 */
	_BIS (P5OUT, 0x20);
	_BIS (P5SEL, 0x20);
}

/* ------------------------------------------------------------------------ */
/* ============================ DEVICE DRIVERS ============================ */
/* ------------------------------------------------------------------------ */

#if	UART_DRIVER
/* ======== */
/* The UART */
/* ======== */

uart_t	zz_uart [UART_DRIVER];

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
		ua->in [ua->ib_in] = RXBUF0;
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

#define	utctl		SSEL0

#if	CRYSTAL_RATE == 32768
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

#else	/* CRYSTAL_RATE > 32768 */

#if UART_RATE < 1200 || UART_RATE > 38400
#error "Illegal UART_RATE, must be between 1200 and 38400"
#endif

// No need to use corrections for high-speed crystals
#define	umctl		0
#define	ubr0		((CRYSTAL_RATE/UART_RATE) % 256)
#define	ubr1		((CRYSTAL_RATE/UART_RATE) / 256)

#endif	/* CRYSTAL_RATE */

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

#if UART_DRIVER > 1
	if (devnum == 0) {
#endif
		// UART_A
		_BIS (P3SEL, 0x30);	// 4, 5 special function
		// _BIS (P3DIR, 0x10);
		// _BIS (P3DIR, 0x20);
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
		adddevfunc (ioreq_uart_a, UART_A);
#if UART_DRIVER > 1
	} else {
		// UART_B
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

#if	LCD_DRIVER
#endif

#if	LEDS_DRIVER
#endif

/* ============== */
/* End of drivers */
/* ============== */
