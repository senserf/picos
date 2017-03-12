#include "kernel.h"

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2017                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

extern 	__pi_pcb_t	*__pi_curr;
extern 	address		__pi_utims [MAX_UTIMERS];

void __pi_malloc_init (void);

// ============================================================================
// Spin delays ================================================================
// ============================================================================

static inline void _gdelay (volatile word n) {
	while (n--);
}

void udelay (volatile word n) {
	while (n) {
		_gdelay (__USEC_DELAY);
		n--;
	}
}

void mdelay (volatile word n) {
	while (n) {
		udelay (999);
		n--;
	}
}

// ============================================================================
// RESET ======================================================================
// ============================================================================

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
	powerdown ();
	diag ("PicOS halted");
	mdelay (500);
	while (1)
		__WFI ();
}

#ifdef	SENSOR_DIGITAL
// ============================================================================
// Access to battery monitor and temperature sensor ===========================
// ============================================================================
void __pi_batmon (word st, const byte *sen, address val) {

	static byte where = 0;

	if (where) {
		// Measure
Measure:
		if (sen [1]) {
			// Temperature
			*val = (word) AONBatMonTemperatureGetDegC ();
		} else {
			// Voltage
			*val = (word) AONBatMonBatteryVoltageGet ();
		}
		where = 0;
		AONBatMonDisable ();
	} else {
		// Initialize
		AONBatMonEnable ();
		if (st == WNONE) {
			// 250 us doesn't work, 500 does, looks like we need
			// 1ms to feel safe
			mdelay (1);
			goto Measure;
		}
		where = 1;
		delay (1, st);
		release;
	}
}

#endif

#if DIAG_MESSAGES > 1
// ============================================================================
// SYSERROR ===================================================================
// ============================================================================

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

#if LEDS_DRIVER
	while (1) {
		all_leds_blink;
	}
#else
	halt ();
#endif

#endif	/* RESET_ON_SYSERR */
}


// ============================================================================
// Device drivers =============================================================
// ============================================================================

#if MAX_DEVICES

// UART is the only device these days

#if UART_DRIVER
#define	N_UARTS		UART_DRIVER
static void devinit_uart (int);
#endif

const static devinit_t devinit [] = {
#if	UART_DRIVER
	{ devinit_uart, UART_A },
#endif
	{ NULL, 0 },
};

#endif	/* MAX_DEVICES */

#if UART_TCV
#define	N_UARTS		UART_TCV
#endif

// ============================================================================
// Power management (to do) ===================================================
// ============================================================================

void powerdown (void) {

	HWREG (NVIC_SYS_CTRL) |= NVIC_SYS_CTRL_SLEEPDEEP;
}

void powerup (void) {

	HWREG (NVIC_SYS_CTRL) &= ~(NVIC_SYS_CTRL_SLEEPDEEP);
}

// ============================================================================
// The clock ==================================================================
// ============================================================================

// Interval for which the delay timer was last set; if setdel is nonzero, it
// means that the timer is running
static word setdel = 0;

static inline word gettav () {
//
// Read the timer value
//
	// This is the full 64-bit timer which we position at milliseconds
	// and extract the 16-bit millisecond count from
	return (word)(AONRTCCurrent64BitValueGet () >> (32 - 10));
}

static inline lword settav (word del) {
//
// Calculate the comparator value for the new delay
//
	return (lword)(AONRTCCurrent64BitValueGet () >> 16) + TCI_TINCR (del);
}

void tci_run_delay_timer () {
//
// Set the delay timer according to __pi_mintk
//
	setdel = __pi_mintk - __pi_old;

	// Set the comparator
	HWREG (AON_RTC_BASE + AON_RTC_O_CH0CMP) = settav (setdel);

	// Enable the event
	sti_tim;
}

void tci_run_auxiliary_timer () {
//
// Start the auxiliary timer
//
	// Disable (in case we are resetting)
	cli_aux;

	HWREG (AON_RTC_BASE + AON_RTC_O_CH2CMP) = settav (1);

	// Enable
	sti_aux;
}
	
void AONRTCIntHandler () {

	lword events;

	events = HWREG (AON_RTC_BASE + AON_RTC_O_EVFLAGS) & 
	    (AON_RTC_EVFLAGS_CH2 | AON_RTC_EVFLAGS_CH0);

	// Clear the events
	HWREG (AON_RTC_BASE + AON_RTC_O_EVFLAGS) = events;

	if (events & AON_RTC_EVFLAGS_CH2) {

		// Auxiliary (automatically set for the next tick, unless
		// disabled)

		Boolean aux_timer_inactive;

#ifdef	MONITOR_PIN_CLOCKA
		_PVS (MONITOR_PIN_CLOCKA, 1);
#endif
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
		if (aux_timer_inactive) {
			// Nobody wants us, disable channel 2
			cli_aux;
		}

#ifdef	MONITOR_PIN_CLOCKA
	_PVS (MONITOR_PIN_CLOCKA, 0);
#endif
	}

	if (events & AON_RTC_EVFLAGS_CH0) {

		// This one serves only delays; disable immediately
#ifdef	MONITOR_PIN_CLOCK
		_PVS (MONITOR_PIN_CLOCK, 1);
#endif
		cli_tim;

		__pi_new += setdel;
		setdel = 0;
		RISE_N_SHINE;

#ifdef	MONITOR_PIN_CLOCK
		_PVS (MONITOR_PIN_CLOCK, 0);
#endif
	}

	RTNI;
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
			// Determine the difference between the comparator
			// and the clock
			__pi_new += setdel - (TCI_INCRT (HWREG (AON_RTC_BASE +
				AON_RTC_O_CH0CMP)) - gettav ());
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

#ifdef	IOCPORTS

const lword port_confs [] = IOCPORTS;

static inline void port_config () {

	for (int i = 0; i < sizeof (port_confs) / sizeof (lword); i++) {
		HWREG (IOC_BASE + ((port_confs [i] >> 17) & 0x7c)) =
			port_confs [i] & 0xff07ffff;
	}
}

#else
#define	port_config()	CNOP
#endif

#ifdef N_UARTS

// The data type is different with UART_TCV; I know this is clumsy

#if N_UARTS > 1
#error "S: only one UART is available on CC13XX, but N_UARTS > 1"
#endif

uart_t	__pi_uart [N_UARTS];

static word urates [N_UARTS];

Boolean __pi_uart_setrate (word rate, uart_t *ua) {

#if N_UARTS > 1
	lword base;
	int unum;

	if (ua == __pi_uart) {
		base = UART0_BASE;
		unum = 0;
	} else {
		base = UART1_BASE;
		unum = 1;
	}
#else
#define	base UART0_BASE
#define	unum 0
#endif

	if (rate < 24 && rate > 2560)
		return NO;

	// Everything else is OK

	UARTDisable (base);

	UARTConfigSetExpClk (base,
		SysCtrlClockGet (),
		((uint32_t)rate) * 100,
		UART_CONFIG_WLEN_8	|
		UART_CONFIG_STOP_ONE	|
		UART_CONFIG_PAR_NONE	|
		0
	);

	UARTEnable (base);
	urates [unum] = rate;
	return YES;
}

#if N_UARTS > 1
#undef base
#undef unum
#endif

word __pi_uart_getrate (const uart_t *ua) {

#if N_UARTS > 1
	return urates [(ua == __pi_uart) ? 0 : 1];
#else
	return urates [0];
#endif
}

static void preinit_uart () {

	PRCMPowerDomainOn (PRCM_DOMAIN_SERIAL);
	while (PRCMPowerDomainStatus (PRCM_DOMAIN_PERIPH) !=
		PRCM_DOMAIN_POWER_ON);

#ifdef	N_UARTS
	// The first UART
	PRCMPeripheralRunEnable (PRCM_PERIPH_UART0);
	PRCMPeripheralSleepEnable (PRCM_PERIPH_UART0);
	PRCMPeripheralDeepSleepEnable (PRCM_PERIPH_UART0);
	PRCMLoadSet ();

	// Seems to be needed, otherwise the initialization fails
	mdelay (10);

	UARTFIFOEnable (UART0_BASE);
	UARTFIFOLevelSet (UART0_BASE, UART_FIFO_TX4_8, UART_FIFO_RX4_8);
	UARTHwFlowControlDisable (UART0_BASE);

	// Set the initial (default) rate
	__pi_uart_setrate (UART_RATE/100, __pi_uart);
#endif
	// UART_B ...
}

#endif	/* UART_DRIVER || UART_TCV */

#if UART_DRIVER

// ============================================================================
// The UART driver ============================================================
// ============================================================================

static int ioreq_uart_a (int operation, char *buf, int len) {

	switch (operation) {

		case READ:

			// Save the original length
			operation = len;
Redo_rx:
			while (len && uart_a_char_available) {
				// There is something in the RX FIFO
				*buf++ = uart_a_read;
				len--;
			}

			if (len != operation) {
				// We have managed to collect something,
				// return this without waiting for more
				return operation - len;
			}

			// Nothing available, have to wait
			uart_a_disable_int;
			if (uart_a_char_available) {
				// Check again (race)
				uart_a_enable_int;
				goto Redo_rx;
			}

			// Now have to wait for sure
			_BIS (__pi_uart->flags, UART_FLAGS_IN);
			return -2;

		case WRITE:

#if defined(blue_ready) && defined(BLUETOOTH_UART)
			// Bluetooth on UART
			if ((__pi_uart->flags & UART_FLAGS_NOTRANS) == 0 &&
			    // BT must be connected
			    !blue_ready)
				// Ignore the write, return OK
				return len;
#endif	/* Transparent BT */

			operation = len;
Redo_tx:
			while (len && uart_a_room_in_tx) {
				// FIFO can accommodate one more
				uart_a_write (*buf);
				buf++;
				len--;
			}

			if (len != operation) {
				// Fine!
				return operation - len;
			}

			// Have to wait
			uart_a_disable_int;
			if (uart_a_room_in_tx) {
				uart_a_enable_int;
				goto Redo_tx;
			}

			_BIS (__pi_uart->flags, UART_FLAGS_OUT);
			return -2;
			
		case NONE:

			// Cleanup
			uart_a_enable_int;
			return 0;

		case CONTROL:

#if UART_RATE_SETTABLE
			if (len == UART_CNTRL_SETRATE) {
				if (__pi_uart_setrate (*((word*)buf),
					__pi_uart))
						return 1;
				syserror (EREQPAR, "uar");
			}

			if (len == UART_CNTRL_GETRATE) {
				*((word*)buf) = __pi_uart_getrate (__pi_uart);
				return 1;
			}
#endif

#ifdef blue_ready
			if (len == UART_CNTRL_TRANSPARENT) {
				if (*((word*)buf))
					_BIC (__pi_uart->flags,
						UART_FLAGS_NOTRANS);
				else
					_BIS (__pi_uart->flags,
						UART_FLAGS_NOTRANS);
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

void UART0IntHandler () {

	uart_a_clear_interrupts;

	if (uart_a_char_available && (__pi_uart->flags & UART_FLAGS_IN)) {
		_BIC (__pi_uart->flags, UART_FLAGS_IN);
		RISE_N_SHINE;
		i_trigger (devevent (UART_A, READ));
	}

	if (uart_a_room_in_tx && (__pi_uart->flags & UART_FLAGS_OUT)) {
		_BIC (__pi_uart->flags, UART_FLAGS_OUT);
		RISE_N_SHINE;
		i_trigger (devevent (UART_A, WRITE));
	}

	RTNI;
}

static void devinit_uart (int devnum) {

	adddevfunc (ioreq_uart_a, devnum);

	UARTIntEnable (UART0_BASE, UART_INT_RX | UART_INT_RT | UART_INT_TX);
	IntEnable (INT_UART0_COMB);
}

#endif /* UART_DRIVER */

// ============================================================================
// Pin interrupts and buttons =================================================
// ============================================================================

void GPIOIntHandler () {
//
// The fallback policy is ignore
//
#ifdef	BUTTON_LIST
#include "irq_buttons.h"
#endif

#ifdef	INPUT_PIN_LIST
#include "irq_pin_sensor.h"
#endif
	// Room for more
}

// ============================================================================

#ifdef	BUTTON_LIST

void __buttons_setirq (int val) {

	int i;
	lword bn;

	cli;
	for (i = 0; i < N_BUTTONS; i++) {
		bn = BUTTON_GPIO (__button_list [i]);
		GPIO_clearEventDio (bn);
		HWREGBITW (IOC_BASE + (bn << 2), IOC_IOCFG0_EDGE_IRQ_EN_BITN) =
			val;
	}
	sti;
}

#endif

#ifdef	INPUT_PIN_LIST

void __pinlist_setirq (int val) {

	int i;
	lword bn;

	cli;
	for (i = 0; i < N_PINLIST; i++) {
		bn = INPUT_PINLIST_GPIO (__input_pins [i]);
		GPIO_clearEventDio (bn);
		HWREGBITW (IOC_BASE + (bn << 2), IOC_IOCFG0_EDGE_IRQ_EN_BITN) =
			val;
	}
	sti;
}

#endif

// ============================================================================
// ============================================================================

static void sync_tim () {
//
// Make sure there are no pending requests for the clock; this just reads the
// SYNC register
//
	volatile long d;

	d = HWREG (AON_RTC_BASE + AON_RTC_O_SYNC);
}

void system_init () {

	PRCMPowerDomainOn (PRCM_DOMAIN_PERIPH);
	while (PRCMPowerDomainStatus (PRCM_DOMAIN_PERIPH) !=
		PRCM_DOMAIN_POWER_ON);

	PRCMPeripheralRunEnable (PRCM_PERIPH_GPIO);
	PRCMPeripheralSleepEnable (PRCM_PERIPH_GPIO);
	PRCMPeripheralDeepSleepEnable (PRCM_PERIPH_GPIO);
	PRCMLoadSet ();

	// Initialize DIO ports
	port_config ();

#if LEDS_DRIVER
	// LEDS
#ifdef	LED0_pin
	GPIO_setOutputEnableDio (LED0_pin, 1);
	// LED0_OFF;
#endif
#ifdef	LED1_pin
	GPIO_setOutputEnableDio (LED1_pin, 1);
	// LED1_OFF;
#endif
#ifdef	LED2_pin
	GPIO_setOutputEnableDio (LED2_pin, 1);
	// LED2_OFF;
#endif
#ifdef	LED3_pin
	GPIO_setOutputEnableDio (LED3_pin, 1);
	// LED3_OFF;
#endif
	// This will also turn them off
	all_leds_blink;
#endif

#ifdef	RADIO_PINS_PREINIT
	RADIO_PINS_PREINIT;
#endif

	// RTC: we use channel 0 for the delay clock and channel 2 for the AUX
	// clock; the seconds clock comes for free

	// Clock (64 bits):
	// Sec: xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx
	// Sub: xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx
	// Tick increment        10 0000 0000 0000 0000 0000
	// MSec increment  100 0000 0000 0000 0000 0000 0000

	// The autoincrement value on channel 2 set to 1 msec
	AONRTCIncValueCh2Set (TCI_TINCR (1));
	// Enable continuous operation of channel 2
	HWREGBITW (AON_RTC_BASE + AON_RTC_O_CHCTL,
		AON_RTC_CHCTL_CH2_CONT_EN_BITN) = 1;

	// Define the combined event as consisting of channels 0 and 2 and
	// enable the clock
	HWREG (AON_RTC_BASE + AON_RTC_O_CTL) =
		AON_RTC_CTL_COMB_EV_MASK_CH0 |
		AON_RTC_CTL_COMB_EV_MASK_CH2 |
// For the radio
AON_RTC_CTL_RTC_UPD_EN |
		AON_RTC_CTL_EN;

	// Enable RTC interrupts
	IntEnable (INT_AON_RTC_COMB);

	// Initialize the memory allocator
	__pi_malloc_init ();

	// Enable GPIO interrupts
	IntEnable (INT_AON_GPIO_EDGE);

#if MAX_TASKS > 0
	// Rigid task table (I don't think we use it any more)

	{
		__pi_pcb_t *p;

		for_all_tasks (p)
			/* Mark all task table entries as available */
			p->code = NULL;
	}
#endif

#ifdef	EMERGENCY_STARTUP_CONDITION

	if (EMERGENCY_STARTUP_CONDITION) {
		EMERGENCY_STARTUP_ACTION;
	}
#endif


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
        	", (C) Olsonet Communications, 2002-2017");
	diag ("Leftover RAM: %d bytes",
		(word)((aword)STACK_END - (aword)(&__bss_end__)));
#endif

#endif	/* DIAG_MESSAGES */

	dbg_1 (0x1000 | SYSVER_X);
	dbg_1 ((word)STACK_END - (word)(&__BSS_END)); // RAM in bytes

#if MAX_DEVICES
	for (int i = UART; i < MAX_DEVICES; i++)
		if (devinit [i] . init != NULL)
			devinit [i] . init (devinit [i] . param);
#endif
	// Kick the auxiliary timer in case something is needed by the
	// drivers
	tci_run_auxiliary_timer ();
}

// static volatile lword __saved_sp;

#if 0
void __run_everything () {

#include "scheduler.h"

}
#endif

__attribute__ ((noreturn)) void __pi_release () {

	__set_MSP ((lword)(STACK_START));

#include "scheduler.h"

}

int main (void) {

#if STACK_GUARD && 0
	{
		register sint i;
		for (i = 0; i < (STACK_SIZE / sizeof (lword)) - 16; i++)
			*((((lword*)STACK_END) - 1) + i) = STACK_SENTINEL;
	}
#endif
	system_init ();

#if TCV_PRESENT
	tcv_init ();
#endif

	// Assume root process identity
	__pi_curr = (__pi_pcb_t*) fork (root, 0);
	// Delay root startup for 16 msec to make sure that the drivers go
	// first
	delay (16, 0);

	sti;

	__pi_release ();
}
