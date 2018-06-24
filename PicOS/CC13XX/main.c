#include "kernel.h"

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2018                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#if	defined(EEPROM_PRESENT) || defined(SDCARD_PRESENT)
#include "storage.h"
#endif

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
// Keeping domains on/off =====================================================
// ============================================================================

void __pi_ondomain (lword d) {
//
// Add the indicated domains to the on set
//
	if (d & PRCM_DOMAIN_RFCORE) {
		// Make sure the effective PD mode is at most 1
		if (__pi_systat.effpdm > RFCORE_PD_MODE)
			__pi_systat.effpdm = RFCORE_PD_MODE;
	}

	PRCMPowerDomainOn (d);
	while (PRCMPowerDomainStatus (d) != PRCM_DOMAIN_POWER_ON);

	__pi_systat.ondmns |= d;
}

void __pi_offdomain (lword d) {
//
// Remove the indicated domains from the on set
//
	PRCMPowerDomainOff (d);
	while (PRCMPowerDomainStatus (d) != PRCM_DOMAIN_POWER_OFF);

	if (d & PRCM_DOMAIN_RFCORE) {
		if (__pi_systat.reqpdm > __pi_systat.effpdm)
			__pi_systat.effpdm = __pi_systat.reqpdm;
	}

	__pi_systat.ondmns &= ~d;
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
	diag ("PicOS halted");
	mdelay (500);
	setpowermode (2);
	// Make all threads disappear; no need to worry about the memory leak;
	// we are going down
	__PCB = NULL;
	release;
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

#if	DIAG_MESSAGES > 1
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

#if	RESET_ON_SYSERR

#if	LEDS_DRIVER
	for (__pi_mintk = 0; __pi_mintk < 32; __pi_mintk++)
		all_leds_blink;
#endif
	reset ();

#else	/* RESET_ON_SYSERR */

// ============================================================================

#if	LEDS_DRIVER
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

#if	MAX_DEVICES

// UART is the only device these days

#if	UART_DRIVER
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

#if	UART_TCV
#define	N_UARTS		UART_TCV
#endif

// Note that I2C and SSI are dynamically switched on and off, so we should
// probably do the same with UART allowing the app to switch it on and off;
// this way the power status of serial domain should be kept track of in some
// flag (FIXME later)
#if	defined(N_UARTS) || I2C_INTERFACE || SSI_INTERFACE
// Note: in the future we may decide to switch these things on and off
// dynamically; not sure if it is worthwhile from the viewpoint of power
// savings, because the true low power mode (STANDBY or SHUTDOWN) forces
// the serial domain off, so we probably don't care whether it is on or off
// during regular or deep sleep; if we do, we shall keep track whether the
// domain is on or off and switch it dynamically;
#define	NEED_SERIAL_DOMAIN
#endif

#ifdef	IOCPORTS

const lword port_confs [] = IOCPORTS;

static inline void port_config () {
//
// Port configuration; we use the RESERVED parts for some extras
//
	int pin;

	for (int i = 0; i < sizeof (port_confs) / sizeof (lword); i++) {

		pin = (port_confs [i] >> 19) & 0x1f;
		HWREG (IOC_BASE + (pin << 2)) = port_confs [i] & 0x7f077f3f;
		if (port_confs [i] & 0x80)
			// Output
			GPIO_setOutputEnableDio (pin, GPIO_OUTPUT_ENABLE);
		if (port_confs [i] & 0x40)
			GPIO_setDio (pin);
		else
			GPIO_clearDio (pin);
	}
}

#else
#define	port_config()	CNOP
#endif

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
// Called to stop the timer, if running, and tally up the ticks; fixed not
// to use sti_tim
//
	if (!force)
		// Just checking in a safe loop of the scheduler; no problem
		// about setdel possibly changing to zero, which we will find
		// out on the next (event) turn of the scheduler loop
		return setdel;

	// Now we have to be accurate
	if (setdel == 0)
		// Timer must be disabled when setdel is found to be zero
		// in a non-interrupt; so the return is safe
		return NO;

	cli_tim;

	// Have to check again with the interrupt disabled; setdel is allowed to
	// become zero only once per setting (and per cli_tim)
	if (setdel) {
		// Determine the difference between the comparator and the clock
		__pi_new += setdel - (TCI_INCRT (HWREG (AON_RTC_BASE +
				AON_RTC_O_CH0CMP)) - gettav ());
		// Use it only once
		setdel = 0;
		// Racing with the interrupt: make sure the event is disabled
		HWREG (AON_RTC_BASE + AON_RTC_O_EVFLAGS) = AON_RTC_EVFLAGS_CH0;
	}

	return NO;
}

// ============================================================================
// SSI ========================================================================
// ============================================================================

#if 	SSI_INTERFACE

//
// The pins; BNONE means unused, ck == BNONE means module is off
//
static	byte	ssi_ck [2] = { BNONE, BNONE },
		ssi_rx [2] = { BNONE, BNONE },
		ssi_tx [2] = { BNONE, BNONE },
		ssi_fs [2] = { BNONE, BNONE };

//
// Mode index to mode code
//
static const byte ssi_modes [] = {
		(byte) SSI_FRF_MOTO_MODE_0,
		(byte) SSI_FRF_MOTO_MODE_1,
		(byte) SSI_FRF_MOTO_MODE_2,
		(byte) SSI_FRF_MOTO_MODE_3,
		(byte) SSI_FRF_TI,
		(byte) SSI_FRF_NMW };

static const lword ssi_base [2] = { SSI0_BASE, SSI1_BASE };

// Interrupt service function
static __ssi_int_fun_t ssi_int_fun [2] = { NULL, NULL };

void SSI0IntHandler () {

	SSIIntClear (SSI0_BASE, SSI_RXTO | SSI_RXOR | SSI_TXFF);

	if (ssi_int_fun [0])
		ssi_int_fun [0] ();
}

void SSI1IntHandler () {

	SSIIntClear (SSI1_BASE, SSI_RXTO | SSI_RXOR | SSI_TXFF);

	if (ssi_int_fun [1])
		ssi_int_fun [1] ();
}

void __ssi_open (sint w, const byte *p, byte m, word rate,
							__ssi_int_fun_t ifun) {
//
// Open SSI interface,
//
//	p 	pins: ck, rx, tx, fs
//	m	mode (see ssi_modes above)
// 	rate	bit rate in 100's bps as for UART
//	ifun	the interrupt function
//
	lword ub;

	if (p == NULL) {
		if (ssi_ck [w] == BNONE)
			return;
	} else {
		if (p [0] == ssi_ck [w])
			return;
	}

	ub = w ? PRCM_PERIPH_SSI1 : PRCM_PERIPH_SSI0;

	if (ssi_ck [w] != BNONE) {

		// Shut down the previous interface

		SSIDisable (ssi_base [w]);
		SSIIntDisable (ssi_base [w], SSI_RXFF | SSI_RXTO | SSI_RXOR |
			SSI_RXFF);
		IntDisable (w ? INT_SSI1_COMB : INT_SSI0_COMB);
		PRCMPeripheralRunDisable (ub);
#if 1
		// Not sure if we need these, because we are not using I2C
		// interrupts, at least not yet
		PRCMPeripheralSleepDisable (ub);
		PRCMPeripheralDeepSleepDisable (ub);
#endif
		PRCMLoadSet ();

		// Reset the pins to standard GPIO
		ssi_int_fun [w] = NULL;
		IOCIOPortIdSet (ssi_ck [w], IOC_PORT_GPIO);
		if (ssi_rx [w] != BNONE) {
			IOCIOPortIdSet (ssi_rx [w], IOC_PORT_GPIO);
			ssi_rx [w] = BNONE;
		}
		if (ssi_tx [w] != BNONE) {
			IOCIOPortIdSet (ssi_tx [w], IOC_PORT_GPIO);
			ssi_tx [w] = BNONE;
		}
		if (ssi_fs [w] != BNONE) {
			IOCIOPortIdSet (ssi_fs [w], IOC_PORT_GPIO);
			ssi_fs [w] = BNONE;
		}
	}

	if (p != NULL && p [0] != BNONE) {

		// Restart for the new configuration

		IOCIOPortIdSet (ssi_ck [w] = p [0], 
			w ? IOC_PORT_MCU_SSI1_CLK : IOC_PORT_MCU_SSI0_CLK);
		if (p [1] != BNONE)
			IOCIOPortIdSet (ssi_rx [w] = p [1],
				w ? IOC_PORT_MCU_SSI1_RX :
					IOC_PORT_MCU_SSI0_RX);
		if (p [2] != BNONE)
			IOCIOPortIdSet (ssi_tx [w] = p [2],
				w ? IOC_PORT_MCU_SSI1_TX :
					IOC_PORT_MCU_SSI0_TX);
		if (p [3] != BNONE)
			IOCIOPortIdSet (ssi_fs [w] = p [3],
				w ? IOC_PORT_MCU_SSI1_FSS :
					IOC_PORT_MCU_SSI0_FSS);

		PRCMPeripheralRunEnable (ub);
#if 1
		PRCMPeripheralSleepEnable (ub);
		PRCMPeripheralDeepSleepEnable (ub);
#endif
		PRCMLoadSet ();

		ssi_int_fun [w] = ifun;

		SSIConfigSetExpClk (ssi_base [w], SysCtrlClockGet (),
			ssi_modes [m],
			SSI_MODE_MASTER,
			(lword) rate * 100,
			8);
		// Note: increasing the width will improve the bandwidth,
		// because the FIFO consists of entries, not bytes

		SSIEnable (ssi_base [w]);
		IntEnable (w ? INT_SSI1_COMB : INT_SSI0_COMB);
		// Interrupts must be enabled by the driver
	} else
		ssi_ck [w] = BNONE;
}

#endif

// ============================================================================
// I2C ========================================================================
// ============================================================================

#if	I2C_INTERFACE

// Note: I2C is interrupt-less for now, but I see a scheme whereby we provide
// an optional interrupt function upon open; this can be done now, because we
// ALWAYS open the interface explicitly before use and, possibly, close it
// afterwards

// The pins
static	byte	i2c_scl = BNONE, i2c_sda = BNONE;

void __i2c_open (byte scl, byte sda, Boolean rate) {

	if (scl == i2c_scl)
		// This is the sole signature of the last open; when this pin
		// matches, we check no further
		return;

	if (i2c_scl != BNONE) {
		// Undo the previous setup
		IOCIOPortIdSet (i2c_sda, IOC_PORT_GPIO);
		IOCIOPortIdSet (i2c_scl, IOC_PORT_GPIO);
		// Shut down the interface
		PRCMPeripheralRunDisable (PRCM_PERIPH_I2C0);
#if 1
		// Not sure if we need these, because we are not using I2C
		// interrupts, at least not yet
		PRCMPeripheralSleepDisable (PRCM_PERIPH_I2C0);
		PRCMPeripheralDeepSleepDisable (PRCM_PERIPH_I2C0);
#endif
		PRCMLoadSet ();
	}

	i2c_sda = sda;
	if ((i2c_scl = scl) != BNONE) {
		// Restart for the new configuration
		IOCIOPortIdSet (i2c_sda, IOC_PORT_MCU_I2C_MSSDA);
		IOCIOPortIdSet (i2c_scl, IOC_PORT_MCU_I2C_MSSCL);

		PRCMPeripheralRunEnable (PRCM_PERIPH_I2C0);
#if 1
		PRCMPeripheralSleepEnable (PRCM_PERIPH_I2C0);
		PRCMPeripheralDeepSleepEnable (PRCM_PERIPH_I2C0);
#endif
		PRCMLoadSet ();

		// The last arg is true/false (fast == 400K, slow == 100K)
		I2CMasterInitExpClk (I2C0_BASE, SysCtrlClockGet (), rate);
		I2CMasterEnable (I2C0_BASE);
	}
}

// ============================================================================

static lword i2c_wait () {

	lword st;

	while (1) {
		st = HWREG (I2C0_BASE + I2C_O_MSTAT);
		if ((st & I2C_MSTAT_BUSY) == 0)
			return (st & (I2C_MSTAT_ERR | I2C_MSTAT_ARBLST));
	}
}

Boolean __i2c_op (byte ad, const byte *xm, lword xl, byte *rx, lword rl) {
//
// Issue a transaction
//
	lword dl;

	if ((dl = xl)) {
		// Transmit present
		I2CMasterSlaveAddrSet (I2C0_BASE, ad, false);
		I2CMasterDataPut (I2C0_BASE, *xm);
		xm++;
		dl--;
		i2c_wait ();
		// The command [ ACK STOP START RUN ]
		I2CMasterControl (I2C0_BASE, (dl == 0 && rl == 0) ?
			// Just send the byte and stop; unfortunately, we have
			// to handle those different cases separately, because
			// start and stop bits are set in the same go
			I2C_MASTER_CMD_SINGLE_SEND :		// 0111
			// Start, don't stop, there is more
			I2C_MASTER_CMD_BURST_SEND_START);	// 0011

		if (i2c_wait ()) {
Error:
			I2CMasterControl (I2C0_BASE,
				// 0100
				I2C_MASTER_CMD_BURST_RECEIVE_ERROR_STOP);
			return YES;
		}

		while (dl) {
			// Keep going for more bytes to send
			I2CMasterDataPut (I2C0_BASE, *xm);
			xm++;
			dl--;
			I2CMasterControl (I2C0_BASE, (dl == 0 && rl == 0) ?
				I2C_MASTER_CMD_BURST_SEND_FINISH :	// 0101
				I2C_MASTER_CMD_BURST_SEND_CONT);	// 0001
			if (i2c_wait ())
				goto Error;
		}
	}

	// Now receive

	if (rl) {
		I2CMasterSlaveAddrSet (I2C0_BASE, ad, true);
		rl--;
		I2CMasterControl (I2C0_BASE, rl ? 
			I2C_MASTER_CMD_BURST_RECEIVE_START :	// 1011
			I2C_MASTER_CMD_SINGLE_RECEIVE);		// 0111
		if (i2c_wait ())
			goto Error;
		*rx++ = I2CMasterDataGet (I2C0_BASE);

		while (rl) {
			rl--;
			I2CMasterControl (I2C0_BASE, rl ? 
				I2C_MASTER_CMD_BURST_RECEIVE_CONT :	// 1001
				I2C_MASTER_CMD_BURST_RECEIVE_FINISH);	// 0101
			if (i2c_wait ())
				goto Error;
			*rx++ = I2CMasterDataGet (I2C0_BASE);
		}
	}

	return NO;
}

#endif

// ============================================================================

#ifdef	N_UARTS

// The data type is different with UART_TCV; I know this is clumsy

#if	N_UARTS > 1
#error "S: only one UART is available on CC13XX, but N_UARTS > 1"
#endif

uart_t	__pi_uart [N_UARTS];

static word urates [N_UARTS];

Boolean __pi_uart_setrate (word rate, uart_t *ua) {

	if (rate < 24 && rate > 2560)
		return NO;

	// Everything else is OK

	UARTDisable (UART0_BASE);

	UARTConfigSetExpClk (UART0_BASE,
		SysCtrlClockGet (),
		((uint32_t)rate) * 100,
		UART_CONFIG_WLEN_8	|
		UART_CONFIG_STOP_ONE	|
		UART_CONFIG_PAR_NONE	|
		0
	);

	UARTEnable (UART0_BASE);
	urates [0] = rate;
	return YES;
}

word __pi_uart_getrate (const uart_t *ua) {

	return urates [0];
}

static void reinit_uart () {
//
// The part to be done after standby-mode power up to account for the UART's
// lack of retention
//
	UARTFIFOEnable (UART0_BASE);
	UARTFIFOLevelSet (UART0_BASE, UART_FIFO_TX4_8, UART_FIFO_RX4_8);
	UARTHwFlowControlDisable (UART0_BASE);

	__pi_uart_setrate (urates [0], __pi_uart);
}
	
static void preinit_uart () {

	PRCMPeripheralRunEnable (PRCM_PERIPH_UART0);
	PRCMPeripheralSleepEnable (PRCM_PERIPH_UART0);
	PRCMPeripheralDeepSleepEnable (PRCM_PERIPH_UART0);
	PRCMLoadSet ();

	urates [0] = UART_RATE / 100;

	reinit_uart ();
}

#endif	/* N_UARTS */

#if	UART_DRIVER

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

#if	defined(blue_ready) && defined(BLUETOOTH_UART)
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

#if	UART_RATE_SETTABLE
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

#ifdef	blue_ready
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

static void devinit_uart (int devnum) {

	adddevfunc (ioreq_uart_a, devnum);
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

#endif

#if	UART_TCV

void UART0IntHandler () {

	uart_a_clear_interrupts;

#define	UA		__pi_uart
#define	RBUF 		uart_a_read
#define	XBUF_STORE(a)	uart_a_write (a)

	while (uart_a_char_available) {

		// Should we check UA->r_istate == IRQ_R_OFF (to hold
		// advance characters in the FIFO)?

#include "irq_uart_r.h"

	}

	while (UA->x_istate != IRQ_X_OFF && uart_a_room_in_tx) {

#include "irq_uart_x.h"

	}

#undef	UA
#undef	RBUF
#undef	XBUF_STORE

	RTNI;
}

#endif

#ifdef	N_UARTS

static inline void enable_uart_interrupts () {

	UARTIntEnable (UART0_BASE, UART_INT_RX | UART_INT_RT | UART_INT_TX);
}

#endif

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

#include "board_pins_interrupts.h"

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

void system_init () {

	Boolean wfsd;		// Wakeup from shutdown

	__pi_ondomain (PRCM_DOMAIN_PERIPH);

	PRCMPeripheralRunEnable (PRCM_PERIPH_GPIO);
	PRCMPeripheralSleepEnable (PRCM_PERIPH_GPIO);
	PRCMPeripheralDeepSleepEnable (PRCM_PERIPH_GPIO);
	PRCMLoadSet ();

#if 0
	// This is set by config, I guess
#if USE_FLASH_CACHE
	VIMSModeSafeSet (VIMS_BASE, VIMS_MODE_ENABLED, true);
#else
	VIMSModeSafeSet (VIMS_BASE, VIMS_MODE_DISABLED, true);
#endif
#endif

	// Initialize DIO ports
	port_config ();

	if (SysCtrlResetSourceGet () == RSTSRC_WAKEUP_FROM_SHUTDOWN) {
		// Waking from shutdown, shoul unfreeze I/O right after setting
		// up the port config, so we can control the peripherals, and,
		// e.g., blink the LEDs ;-)
		wfsd = YES;
		PowerCtrlIOFreezeDisable ();
	} else {
		wfsd = NO;
	}

#if	LEDS_DRIVER
	all_leds_blink;
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

#ifdef	EMERGENCY_STARTUP_CONDITION

	if (EMERGENCY_STARTUP_CONDITION) {
		EMERGENCY_STARTUP_ACTION;
	}
#endif

#ifdef	NEED_SERIAL_DOMAIN
	__pi_ondomain (PRCM_DOMAIN_SERIAL);
#endif

#ifdef	N_UARTS
	preinit_uart ();
#endif

#ifdef	SENSOR_INITIALIZERS
	// May need I2C for this
	__pi_init_sensors ();
#endif

#ifdef	ACTUATOR_INITIALIZERS
	__pi_init_actuators ();
#endif

#ifdef	EXTRA_INITIALIZERS
	// Extra initialization
	EXTRA_INITIALIZERS;
#endif
	if (!wfsd) {

		// Don't show the banner, if waking from shutdown

#if	DIAG_MESSAGES
		diag ("");

#ifdef	BANNER
		diag (BANNER);
#else
		diag ("PicOS v" SYSVER_S "/" SYSVER_R
#ifdef	SYSVER_B
			"-" SYSVER_B
#endif
        		", (C) Olsonet Communications, 2002-2018");
		diag ("Leftover RAM: %d bytes",
			(word)((aword)STACK_END - (aword)(&__bss_end__)));
#endif

#endif	/* DIAG_MESSAGES */

		dbg_1 (0x1000 | SYSVER_X);
		dbg_1 ((word)STACK_END - (word)(&__BSS_END)); // RAM in bytes
	}

#if	MAX_DEVICES
	for (int i = UART; i < MAX_DEVICES; i++)
		if (devinit [i] . init != NULL)
			devinit [i] . init (devinit [i] . param);
#endif

#ifdef	N_UARTS
	// The same for UART_DRIVER and UART_TCV
	enable_uart_interrupts ();
	IntEnable (INT_UART0_COMB);
#endif
	// Kick the auxiliary timer in case something is needed by the
	// drivers
	tci_run_auxiliary_timer ();
}

// ============================================================================
// Power management ===========================================================
// ============================================================================

void setpowermode (word mode) {
//
// This just sets the mode; the action is carried out when we are about to
// execute WFI
//
	if (mode > 3)
		// This is the maximum
		mode = 3;

	__pi_systat.effpdm = ((__pi_systat.reqpdm = mode) <= RFCORE_PD_MODE ||
	    (__pi_systat.ondmns & PRCM_DOMAIN_RFCORE) == 0) ?
		mode : RFCORE_PD_MODE;
}

__attribute__((noreturn)) void hibernate () {
//
// Unconditional shutdown
//
#if 0
	// OSC source must be HF at this point (it is, but let's make it
	// absolutely foolproof)
    	if (OSCClockSourceGet (OSC_SRC_CLK_HF) != OSC_RCOSC_HF) {
       		OSCClockSourceSet (OSC_SRC_CLK_HF |
			OSC_SRC_CLK_MF, OSC_RCOSC_HF);
       		while (!OSCHfSourceReady ());
       		OSCHfSourceSwitch ();
       	}
#endif

#if 0
	// Disable CRYPTO and UDMA (we don't need them [yet])
       	PRCMPeripheralDeepSleepDisable (PRCM_PERIPH_CRYPTO);
       	PRCMPeripheralDeepSleepDisable (PRCM_PERIPH_UDMA);
       	PRCMLoadSet ();
       	while (!PRCMLoadGet ());
#endif
	// Power off AUX and disconnect from bus
	AUXWUCPowerCtrl (AUX_WUC_POWER_OFF);

	// Remove AUX force ON
	HWREG (AON_WUC_BASE + AON_WUC_O_AUXCTL) &= ~AON_WUC_AUXCTL_AUX_FORCE_ON;
	HWREG (AON_WUC_BASE + AON_WUC_O_JTAGCFG) &=
		~AON_WUC_JTAGCFG_JTAG_PD_FORCE_ON;

	// Reset AON event source IDs to prevent pending events from powering
	// on MCU/AUX
	HWREG (AON_EVENT_BASE + AON_EVENT_O_MCUWUSEL) = 0x3F3F3F3F;
	HWREG (AON_EVENT_BASE + AON_EVENT_O_AUXWUSEL) = 0x3F3F3F3F;

	// Pins should be configured for wakeup by now; sync AON
	SysCtrlAonSync ();

	// Enable shutdown, latch the IO
	AONWUCShutDownEnable ();

	// Sync AON again (why do we have to do it twice?)
	SysCtrlAonSync ();

	// Wait until AUX powered off
	while (AONWUCPowerStatusGet () & AONWUC_AUX_POWER_ON);

	// Request to power off the MCU when it goes to deep sleep
	PRCMMcuPowerOff ();

	// Turn off all domains
	PRCMPowerDomainOff (
			PRCM_DOMAIN_RFCORE |
			PRCM_DOMAIN_SERIAL |
			PRCM_DOMAIN_PERIPH |
			PRCM_DOMAIN_CPU    |
			PRCM_DOMAIN_VIMS );

	// Deep sleep
	while (1)
		// Once is enough, but then gcc complains that the function
		// returns; so what is the attribute for?
		PRCMDeepSleep ();
}
	
lword system_event_count;	// For debugging, but maybe it should stay

static inline void __do_wfi_as_needed () {
//
// WFI in the right power mode
//
	system_event_count ++;

	switch (__pi_systat.effpdm) {

		case 0:

			__WFI ();
			return;

				// ============================================
		case 1:		// IDLE MODE ==================================
				// ============================================

			// Flash not needed
			HWREG (PRCM_BASE + PRCM_O_PDCTL1VIMS) |=
				PRCM_PDCTL1VIMS_ON;

			// Do we need that?
			PRCMCacheRetentionEnable ();

			// Turn off the CPU power domain
			PRCMPowerDomainOff (PRCM_DOMAIN_CPU);

			// Complete AON writes
			SysCtrlAonSync ();

			// Enter deep sleep
			PRCMDeepSleep ();

			// PRCMDeepSleep is Equivalent to this
			// HWREG (NVIC_SYS_CTRL) |= NVIC_SYS_CTRL_SLEEPDEEP;
			// __WFI ();
			// HWREG (NVIC_SYS_CTRL) &= ~(NVIC_SYS_CTRL_SLEEPDEEP);

			// We are mostly woken by the timer; previously, I had
			// here SysCtrlAonSync, but it didn't work; this makes
			// more sense, intuitively
			SysCtrlAonUpdate ();

			return;

				// ============================================
		case 2:		// STANDBY MODE ===============================
				// ============================================

			AONIOCFreezeEnable ();
			// We know that XOSC_HF is not active, because RF is
			// off; the only idle mode compatible with RF is IDLE

			// Allow AUX to power down
			AONWUCAuxWakeupEvent (AONWUC_AUX_ALLOW_SLEEP);

			// Pending AON writes
			SysCtrlAonSync ();

			PRCMLoadSet ();

			// Domains off
			PRCMPowerDomainOff (__pi_systat.ondmns |
				PRCM_DOMAIN_CPU);

			// Request uLDO during standby
			PRCMMcuUldoConfigure (true);

			// This is needed here!!! Why???
			while (VIMSModeGet (VIMS_BASE) == VIMS_MODE_CHANGING);
#if USE_FLASH_CACHE == 0
			// No flash cache, make sure VIMS is down
			// Turn off the VIMS
			VIMSModeSet (VIMS_BASE, VIMS_MODE_OFF);
			// This applies to GPRAM then, it is lost on sleep?
			PRCMCacheRetentionDisable ();
#endif
			// Setup recharge parameters
			SysCtrlSetRechargeBeforePowerDown (
				XOSC_IN_HIGH_POWER_MODE
			);

			// Wait for AON writes to complete
			SysCtrlAonSync ();

			// Enter deep sleep
			PRCMDeepSleep ();

			// HWREG (NVIC_SYS_CTRL) |= NVIC_SYS_CTRL_SLEEPDEEP;
			// __WFI ();
			// HWREG (NVIC_SYS_CTRL) &= ~(NVIC_SYS_CTRL_SLEEPDEEP);

#if USE_FLASH_CACHE == 0
			// Restore the cache mode; note: make sure that it is
			// globally enabled, which is the default
			VIMSModeSet (VIMS_BASE, VIMS_MODE_ENABLED);
			// This has to be cleaned up!! We only cover two
			// possibilities now; the third one is no cache, no
			// extra GPRAM; probably the most interesting one!
			PRCMCacheRetentionEnable ();
#endif
			// Force power on of AUX; this also counts as a write
			// so a following sync will force an update of AON
			// registers
			AONWUCAuxWakeupEvent (AONWUC_AUX_WAKEUP);

			// Restore power domain states 
			PRCMPowerDomainOn (__pi_systat.ondmns);

			// Make sure clock settings take effect (not needed
			// after PRCMPowerDomainOn??)
			PRCMLoadSet ();

			// Release uLDO
			PRCMMcuUldoConfigure (false);

			// Wait for the domains to come up
			while (PRCMPowerDomainStatus (__pi_systat.ondmns) !=
				PRCM_DOMAIN_POWER_ON);

			// Disable I/O freeze
			AONIOCFreezeDisable ();

			// Ensure shadow RTC is updated
			SysCtrlAonUpdate ();

			// Wait for AUX to power up
			while (!(AONWUCPowerStatusGet () &
				AONWUC_AUX_POWER_ON));

			// Adjust recharge parameters
			SysCtrlAdjustRechargeAfterPowerDown ();

			// Restart no-retention modules; they are static
			// at present, but we may want to power them up and
			// down dynamically in the future

#ifdef	N_UARTS
			// This assumes that PRCM_DOMAIN_SERIAL is up:
			// __pi_systat.ondmns & PRCM_DOMAIN_SERIAL
			reinit_uart ();
			enable_uart_interrupts ();
#endif

#if I2C_INTERFACE
			// Force I2C reinit on first I/O
			i2c_scl = BNONE;
#endif
			// ... other domains (we don't do this with RF on)
			return;

				// ============================================
		default:	// SHUTDOWN ===================================
				// ============================================

		hibernate ();

		// No return

	}
}

#if	STACK_GUARD

word __pi_stackfree (void) {

	word sc;

	for (sc = 0; sc < STACK_SIZE / sizeof (lword); sc++)
		if (*(((lword*)STACK_END) + sc) != STACK_SENTINEL)
			break;
	return sc;
}

#endif

__attribute__ ((noreturn)) void __pi_release () {

	__set_MSP ((lword)(STACK_START));

	check_stack_overflow;

#include "scheduler.h"

}

int main (void) {

#if	STACK_GUARD
	{
		register sint i;
		for (i = 0; i < (STACK_SIZE / sizeof (lword)) - 16; i++)
			*((((lword*)STACK_END) - 1) + i) = STACK_SENTINEL;
	}
#endif
	system_init ();

#if	TCV_PRESENT
	tcv_init ();
#endif
	// For standby mode wakeup on timer
	AONEventMcuWakeUpSet (AON_EVENT_MCU_EVENT0, AON_EVENT_RTC_COMB_DLY);
	// Edge on any I/O
	AONEventMcuWakeUpSet (AON_EVENT_MCU_EVENT1, AON_EVENT_IO);

	// Assume root process identity
	__pi_curr = (__pi_pcb_t*) fork (root, 0);
	// Delay root startup for 16 msec to make sure that the drivers go
	// first
	delay (16, 0);

	sti;

	__pi_release ();
}

//
// Notes:
//
//	Do not enter PD if aux timer is active (aux_timer_inactive = 0);
//	how to do it? Not possible, I am afraid ... because the interrup
//	won't break the SLEEP. Force RISE_N_SHINE when aux_timer_inactive
//	becomes set and PD mode is selected!!!!! Not needed, the SLEEP loop
//	turns after every interrupt!!!
//
//	We must keep track of power domains. For example, if UART is present,
//	it must be powered up (this can be done statically), but what about
//	radio? OK, for now, we may prevent deep sleep if radio is on (status
//	change involves a SLEEP loop turn).
//
//	How to avoid the minute wakeups on MAX delay?
//
