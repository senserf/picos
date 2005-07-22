#include "kernel.h"

/* ============================================================================ */
/*                       PicOS                                                  */
/*                                                                              */
/* main.c for eCOG                                                              */
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

extern 			pcb_t		*zz_curr;
extern 			word  		zz_mintk;
extern 	volatile 	word 		zz_lostk;
extern 			address	zz_utims [MAX_UTIMERS];
extern	void		_reset_vector__;

systat_t 		zz_systat;
word			zz_malloc_length;
word			zz_restart_sp;

#define	STACK_SENTINEL	0xB779

void	zz_malloc_init (void);

/* ========================== */
/* Device driver initializers */
/* ========================== */
#if	UART_DRIVER
static void	devinit_uart (int);
#endif

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
	TBCCR0 = 4095;	// 1 tick per second
}

void clockup (void) {
	TBCCR0 = 3;	// 1024 ticks per second
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

#if	DIAG_MESSAGES

#define	diag_wchar(c,a)		TXBUF0 = (byte)(c)
#define	diag_wait(a)		while ((IFG1 & UTXIFG0) == 0)
#define	diag_disable_int(a)	uart_a_disable_int
#define	diag_enable_int(a)	do { \
					uart_a_enable_read_int; \
					uart_a_enable_write_int; \
				} while (0)

void diag (const char *mess, ...) {
/* ================================ */
/* Writes a direct message to UART0 */
/* ================================ */

	va_list	ap;
	word i, val, v;
	char *s;

	va_start (ap, mess);
	diag_disable_int (a);

	while  (*mess != '\0') {
		if (*mess == '%') {
			mess++;
			switch (*mess) {
			  case 'x' :
				val = va_arg (ap, word);
				for (i = 0; i < 16; i += 4) {
					v = (val >> 12 - i) & 0xf;
					if (v > 9)
						v = (word)'a' + v - 10;
					else
						v = (word)'0' + v;
					diag_wait (a);
					diag_wchar (v, a);
				}
				break;
			  case 'd' :
				val = va_arg (ap, word);
				if (val & 0x8000) {
					diag_wait (a);
					diag_wchar ('-', a);
					val = (~val) + 1;
				}
			    DI_SIG:
				i = 10000;
				while (1) {
					v = val / i;
					if (v || i == 1) break;
					i /= 10;
				}
				while (1) {
					diag_wait (a);
					diag_wchar (v + '0', a);
					val = val - (v * i);
					i /= 10;
					if (i == 0) break;
					v = val / i;
				}
				break;
			  case 'u' :
				val = va_arg (ap, word);
				goto DI_SIG;
			  case 's' :
				s = va_arg (ap, char*);
				while (*s != '\0') {
					diag_wait (a);
					diag_wchar (*s, a);
					s++;
				}
				break;
			  default:
				diag_wait (a);
				diag_wchar ('%', a);
				diag_wait (a);
				diag_wchar (*mess, a);
			}
			mess++;
		} else {
			diag_wait (a);
			diag_wchar (*mess++, a);
		}
	}

	diag_wait (a);
	diag_wchar ('\r', a);
	diag_wait (a);
	diag_wchar ('\n', a);
	diag_wait (a);

	diag_enable_int (a);
}

#else

void diag (const char *mess, ...) { }

#endif

#ifdef	DEBUG_BUFFER

word	debug_buffer_pointer = 0;
word	debug_buffer [DEBUG_BUFFER];

void dbb (word d) {

	if (debug_buffer_pointer == DEBUG_BUFFER)
		debug_buffer_pointer = 0;

	debug_buffer [debug_buffer_pointer++] = d;
	debug_buffer [debug_buffer_pointer == DEBUG_BUFFER ? 0 :
		debug_buffer_pointer] = 0xdead;
}

#endif

#ifdef	DUMP_PCB
void dpcb (pcb_t *p) {

	diag ("PR %x: S%x T%x E (%x %x) (%x %x) (%x %x)", (word) p,
		p->Status, p->Timer,
		p->Events [0] . Status,
		p->Events [0] . Event,
		p->Events [1] . Status,
		p->Events [1] . Event,
		p->Events [2] . Status,
		p->Events [2] . Event);
}
#else
#define dpcb(a)
#endif

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
	BCSCTL1 = RSEL2 + RSEL1 + RSEL0 + XT2OFF;
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

	if (TBCCR0 == 3) {
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

		if (zz_lostk & 1024) {
			// Run the scheduler at least once every second - to keep the
			// second clock up to date
			RISE_N_SHINE;
			return;
		}

		if (zz_mintk && zz_mintk <= zz_lostk)
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
	zz_lostk += JIFFIES;
	RISE_N_SHINE;
}

static void mem_init () {

	// This is in words
	zz_malloc_length = ((word) STACK_END - (word)&__bss_end) / 2;
#if	STACK_GUARD
	// One word used as the stack sentinel
	zz_malloc_length--;
#endif
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
}

/* ------------------------------------------------------------------------ */
/* ============================ DEVICE DRIVERS ============================ */
/* ------------------------------------------------------------------------ */

#if	UART_DRIVER
/* ======== */
/* The UART */
/* ======== */

static uart_t	uart [2];

static const word uart_rates [] = {
					 1200, 0x1B, 0x03,
					 2400, 0x0D, 0x6B,
					 4800, 0x06, 0x6F,
					 9600, 0x03, 0x4A };
					// We cannot go any further with ACLK used as
					// the strobbing clock. We would need a special
					// crystal for higher rates as the DCO clock is
					// not very reliable.

static INLINE void uart_setrate (int dev, word rate) {

	int i;

	for (i = 0; i < sizeof (uart_rates) / 2 - 3; i += 3)
		if (uart_rates [i] >= rate)
			break;

	if (dev) {
		UBR01 = uart_rates [i+1];
		UBR11 = 0;
		UMCTL1 = uart_rates [i+2];
	} else {
		UBR00 = uart_rates [i+1];
		UBR10 = 0;
		UMCTL0 = uart_rates [i+2];
	}
}

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
				uart_disable_read_int (u);
				/* One character at a time */
				iotrigger (UART_BASE + u->selector, REQUEST);
				if (len && uart_rpending (u)) {
					*buf = uart_read (u);
					return 1;
				}
				_BIS (u->flags, UART_FLAGS_WREAD);
				return -2;
			}

		case WRITE:

			if ((u->flags & UART_FLAGS_LOCK)) {
				if (len && uart_xpending (u)) {
					uart_write8 (u, *buf);
					return 1;
				}
				return 0;
			} else {
				uart_disable_write_int (u);
				iotrigger (UART_BASE + u->selector, REQUEST);
				if (len == 0)
					// In case
					return -1;
				if (uart_xpending (u)) {
					uart_write8 (u, *buf);
					return 1;
				}
				_BIS (u->flags, UART_FLAGS_WWRITE);
				return -2;
			}

		case NONE:

			// Cleanup
			if ((u->flags & UART_FLAGS_WREAD))
				uart_enable_read_int (u);
			if ((u->flags & UART_FLAGS_WWRITE))
				uart_enable_write_int (u);

			return 0;

		case CONTROL:

			switch (len) {

				case UART_CNTRL_LCK:

					if (*buf)
						uart_lock (u);
					else
						uart_unlock (u);
					return 1;

				case UART_CNTRL_RATE:

					uart_setrate (u->selector,
						*((word*)buf));
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
	uart_enable_write_int (u);
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
	_BIC (IE1, UTXIE0);
	_BIS (IFG1, UTXIFG0);
	_BIC (uart [0] . flags, UART_FLAGS_WWRITE);
	i_trigger (ETYPE_IO, devevent (UART_A, WRITE));
}

interrupt (UART0RX_VECTOR) uart0rx_int (void) {
	RISE_N_SHINE;
	_BIC (IE1, URXIE0);
	_BIS (IFG1, URXIFG0);
	_BIC (uart [0] . flags, UART_FLAGS_WREAD);
	i_trigger (ETYPE_IO, devevent (UART_A, READ));
}

static int ioreq_uart_a (int op, char *b, int len) {
	return ioreq_uart (&(uart [0]), op, b, len);
}

#if UART_DRIVER > 1

interrupt (UART1TX_VECTOR) uart1tx_int (void) {
	RISE_N_SHINE;
	_BIC (IE2, UTXIE1);
	_BIS (IFG2, UTXIFG1);
	_BIC (uart [1] . flags, UART_FLAGS_WWRITE);
	i_trigger (ETYPE_IO, devevent (UART_B, WRITE));
}

interrupt (UART1RX_VECTOR) uart1rx_int (void) {
	RISE_N_SHINE;
	_BIC (IE2, URXIE1);
	_BIS (IFG2, URXIFG1);
	_BIC (uart [1] . flags, UART_FLAGS_WREAD);
	i_trigger (ETYPE_IO, devevent (UART_B, READ));
}

static int ioreq_uart_b (int op, char *b, int len) {
	return ioreq_uart (&(uart [1]), op, b, len);
}

#endif

/* =========== */
/* Initializer */
/* =========== */
static void devinit_uart (int devnum) {

#if UART_DRIVER > 1
	if (devnum == 0) {
#endif
		// UART_A
		_BIS (P3SEL, 0x30);	// 4, 5 special function
		// _BIS (P3DIR, 0x10);
		// _BIS (P3DIR, 0x20);
		_BIS (ME1, UTXE0 + URXE0);
		_BIS (UCTL0, CHAR);	// 1 stop bit, 8 bits, no parity
		_BIS (UTCTL0, SSEL0);	// ACLK
		_BIS (IFG1, UTXIFG0);
		_BIC (UCTL0, SWRST);
		adddevfunc (ioreq_uart_a, UART_A);
#if UART_DRIVER > 1
	} else {
		// UART_B
		_BIS (P3SEL, 0xc0);	// 6, 7 special function
		_BIS (ME2, UTXE1 + URXE1);
		_BIS (UCTL1, CHAR);	// 1 stop bit, 8 bits, no parity
		_BIS (UTCTL1, SSEL0);	// ACLK
		_BIS (IFG2, UTXIFG1);
		_BIC (UCTL1, SWRST);
		adddevfunc (ioreq_uart_b, UART_B);
	}
#endif
	uart [devnum] . selector = devnum;
	_BIS (uart [devnum] . flags, UART_FLAGS_LOCK);
	uart_setrate (devnum, 9600);
	uart_unlock (uart + devnum);
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
