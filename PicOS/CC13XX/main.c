#include "kernel.h"

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2017                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

extern 	__pi_pcb_t	*__pi_curr;
extern 	address		__pi_utims [MAX_UTIMERS];

// ============================================================================
// The clock ==================================================================
// ============================================================================

// Interval for which the delay timer was last set
static word setdel = 0;

static inline word gettav () {
//
// Read the timer value
//
	// This is the full 64-bit timer which we position at milliseconds
	// and extract the 16-bit millisecond count from
	return (word)((AONRTCCurrent64BitValueGet () >> (32 - 10)) & 0xffff);
}

static inline lword settav (word del) {
//
// Calculate the comparator value for the new delay
//
	return (lword)(AONRTCCurrent64BitValueGet () >> 16) +
		TCI_TINCR ((lword) del);
}

void tci_run_delay_timer () {
//
// Set the delay timer according to __pi_mintk
//
	word d;

	// Time to elapse in msec
	d = __pi_mintk - __pi_old;

	// Top off at half the range
	setdel = (d > TCI_MAXDEL) ? TCI_MAXDEL : d;

	// Set the comparator
	HWREG (AON_RTC_BASE + AON_RTC_O_CH0CMP) = settav (del);

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

	sint events;

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
		RISE_N_SHINE;

#ifdef	MONITOR_PIN_CLOCK
		_PVS (MONITOR_PIN_CLOCK, 0);
#endif
		RTNI;
	}
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

void system_init () {

	// Initialize DIO ports








	PRCMPowerDomainOn (PRCM_DOMAIN_PERIPH);
	while (PRCMPowerDomainStatus (PRCM_DOMAIN_PERIPH) !=
		PRCM_DOMAIN_POWER_ON);

	PRCMPeripheralRunEnable (PRCM_PERIPH_GPIO);
	PRCMPeripheralSleepEnable (PRCM_PERIPH_GPIO);
	PRCMPeripheralDeepSleepEnable (PRCM_PERIPH_GPIO);
	PRCMLoadSet ();

	// Start RTC defining the events; we use channel 0 for the delay clock
	// and channel 2 for the AUX clock; the seconds clock comes for free

	// The increment value on channel 2 set to 1 msec
	AONRTCIncValueCh2Set (TCI_INCR (1));
	// Enable continuous operation of channel 2
	HWREGBITW (AON_RTC_BASE + AON_RTC_O_CHCTL,
		AON_RTC_CHCTL_CH2_CONT_EN_BITN) = 1;

	// Define the combined event as consisting of channels 0 and 2 +
	// enable the clock
	HWREG (AON_RTC_BASE + AON_RTC_O_CTL) =
		AON_RTC_CTL_COMB_EV_MASK_CH0 |
		AON_RTC_CTL_COMB_EV_MASK_CH2 |
		AON_RTC_CTL_EN;

	// Enable RTC interrupts
	IntEnable (INT_UART0_COMB);

	for (int i = 0; i < MAX_DEVICES; i++)
		if (devinit [i] . init != NULL)
			devinit [i] . init (devinit [i] . param);

	// Kick the auxiliary timer right away, in case there is something
	// required by the drivers
	void tci_run_auxiliary_timer ();
}

int main (void) {

#if STACK_GUARD
	{
		register sint i;
		for (i = 0; i < (STACK_SIZE / sizeof (lword)); i++)
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

	// Fall through to the scheduler

#include "scheduler.h"

}
