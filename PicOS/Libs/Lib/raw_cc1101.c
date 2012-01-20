/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// This is the set of interface functions for raw access to CC110x from the
// praxis

#include "kernel.h"
#include "raw_cc1101.h"

#ifdef	__CC430__

// ============================================================================
// CC430 (internal MSP430) ====================================================
// ============================================================================

void rrf_set_reg (byte addr, byte val) {

	// Wait for ready
	while (!(RF1AIFCTL1 & RFINSTRIFG));

	// Address + instruction
	RF1AINSTRB = (addr | RF_REGWR);
	RF1ADINB = val;
}

byte rrf_get_reg (byte addr) {

	byte val;

	RF1AINSTR1B = (addr | 0x80); 
	// A simple return should do
  	while (!(RF1AIFCTL1 & RFDOUTIFG));
	val = RF1ADOUT1B;
	return val;
}

void rrf_set_reg_burst (byte addr, byte *buffer, word count) {
//
// Note: this works word-wise, not bytewise (a known bug)
//
	volatile word i;                             

	// Wait for ready for next instruction
	while (!(RF1AIFCTL1 & RFINSTRIFG));

	// Address + instruction
	RF1AINSTRW = (((word)(addr | RF_REGWR)) << 8 ) + *buffer;

	for (i = 1; i < count; i++) {
		// Next byte
		buffer++;
    		RF1ADINB = *buffer;
		// Wait until complete
		while (!(RF1AIFCTL1 & RFDINIFG));
	} 

	i = RF1ADOUTB;
}

void rrf_get_reg_burst (byte addr, byte *buffer, word count) {

	word i;

	// Wait for ready
	while (!(RF1AIFCTL1 & RFINSTRIFG));

	// Address + instruction
	RF1AINSTR1B = (addr | RF_REGRD);

	for (i = 1; i < count; i++) {
		// Wait for the byte
		while (!(RF1AIFCTL1 & RFDOUTIFG));
                // Read DOUT + clear RFDOUTIFG, initialize auto-read for next
		*buffer++ = RF1ADOUT1B;
	}

	// The last one
	*buffer = RF1ADOUT0B;
}

static byte cc1100_strobe (byte b) {

	volatile byte sb;

	// Clear the status-read flag 
	RF1AIFCTL1 &= ~(RFSTATIFG);
	  
	// Wait until ready for next instruction
	while(!(RF1AIFCTL1 & RFINSTRIFG));

	// Issue the command
	RF1AINSTRB = b; 

	while (b != CCxxx0_SRES && b != CCxxx0_SNOP &&
		(RF1AIFCTL1 & RFSTATIFG) == 0);

	sb = RF1ASTATB;

	return (sb & CC1100_STATE_MASK);
}

#else

// ============================================================================
// SPI BASED (external chip) ==================================================
// ============================================================================

static void cc1100_spi_out (byte b) {

	register int i;

	for (i = 0; i < 8; i++) {
		if (b & 0x80)
			si_up;
		else
			si_down;
		b <<= 1;
		sclk_up;
		SPI_WAIT;
		sclk_down;
		SPI_WAIT;
	}
}

static byte cc1100_spi_out_stat (byte b) {

	register byte i, val;

	for (i = val = 0; i < 8; i++) {
		if (b & 0x80)
			si_up;
		else
			si_down;
		val <<= 1;
		if (so_val)
			val |= 1;
		b <<= 1;
		sclk_up;
		SPI_WAIT;
		sclk_down;
		SPI_WAIT;
	}
	return (val & CC1100_STATE_MASK);
}

static byte cc1100_spi_in () {

	register byte i, val;

	for (i = val = 0; i < 8; i++) {
		val <<= 1;
		if (so_val)
			val |= 1;
		sclk_up;
		SPI_WAIT;
		sclk_down;
		SPI_WAIT;
	}

	return val;
}

void rrf_set_reg (byte addr, byte val) {

	SPI_START;
	SPI_WAIT;
	cc1100_spi_out (addr);
	cc1100_spi_out (val);
	SPI_END;
}

byte rrf_get_reg (byte addr) {

	register byte val;

	SPI_START;
	cc1100_spi_out (addr | 0x80);
	val = cc1100_spi_in ();
	SPI_END;
	return val;
}

void rrf_set_reg_burst (byte addr, byte *buffer, word count) {

	SPI_START;
	cc1100_spi_out (addr | 0x40);
	while (count--)
		cc1100_spi_out (*buffer++);
	SPI_END;
}

void rrf_get_reg_burst (byte addr, byte *buffer, word count) {

	SPI_START;
	cc1100_spi_out (addr | 0xC0);
	while (count--)
		*buffer++ = cc1100_spi_in ();
	SPI_END;
}
	
static void cc1100_strobe (byte cmd) {

	SPI_START;
	cc1100_spi_out (cmd);
	SPI_END;
}

#endif

// ============================================================================
// SPI or CC430 ===============================================================
// ============================================================================

void rrf_set_reg_group (const byte *grp) {

	for ( ; *grp != 255; grp += 2) {
		rrf_set_reg (grp [0], grp [1]);
	}
}

byte rrf_status () {

	register byte val;
	int i;
ReTry:
	for (i = 0; i < 32; i++) {

#ifdef	__CC430__

		val = cc1100_strobe (CCxxx0_SNOP);
#else

		SPI_START;
		val = cc1100_spi_out_stat (CCxxx0_SNOP | 0x80);
		SPI_END;
#endif
		switch (val) {

			// Clean up hanging overflow/underflow states

			case CC1100_STATE_TX_UNDERFLOW:

				cc1100_strobe (CCxxx0_SFTX);
				break;

			case CC1100_STATE_RX_OVERFLOW:

				cc1100_strobe (CCxxx0_SFRX);

			// Loop on transitional states until they go away

			case CC1100_STATE_CALIBRATE:
			case CC1100_STATE_SETTLING:

				break;

			default:
				return val;
		}
		udelay (10);
	}

#ifdef RRF_DEBUG
	diag ("CC1100: %u ST HNG!!", (word) seconds ());
#endif
	goto ReTry;
}

void rrf_enter_idle () {

	int i;

	for (i = 0; i < 16; i++) {
		if (rrf_status () == CC1100_STATE_IDLE)
			return;
		cc1100_strobe (CCxxx0_SIDLE);
	}
}

void rrf_enter_rx () {

	int i;

	for (i = 0; i < 16; i++) {
		cc1100_strobe (CCxxx0_SRX);
		if (rrf_status () == CC1100_STATE_RX)
			return;
		// Note: the reason I am entering this state differently than
		// IDLE (see enter_idle above) is primarily a bug/feature of
		// CC430 causing CPU hangups when enter_rx is invoked after a
		// reset following power down (SPD). I don't understand what is
		// going on there, except that inserting this delay:
		udelay (100);
		// between two consecutive retries does help. Apparently, no
		// such delay is needed for enter_idle. Besides, enter_idle
		// is not unlikely to be called when the state is IDLE already,
		// so it may make sense to check first and set second, which is
		// different for enter_rx.
	}

#ifdef RRF_DEBUG
	diag ("CC1100: CANT RX!!");
#endif
}

int rrf_rx_status () {

#ifdef	__CC430__

	if (cc1100_strobe (CCxxx0_SNOP) == CC1100_STATE_IDLE) {

		// The state is right, check RXBYTES
		return cc1100_get_reg (CCxxx0_RXBYTES) & 0x7f;
	}
#else
	register byte b, val;

	SPI_START;

	val = cc1100_spi_out_stat (CCxxx0_RXBYTES);

	// Get RXBYTES
	b = cc1100_spi_in ();

	SPI_END;

	if (val == CC1100_STATE_IDLE || val == CC1100_STATE_RX)
		// The status is right, return #bytes in RX FIFO; note that we
		// also admit RX status which may result from two packets
		// received back to back
		return (b & 0x7f);
#ifdef RRF_DEBUG
	diag ("CC1100: %u RXST = %x/%x!!", (word) seconds (), val, b);
#endif

#endif
	return -1;
}

void rrf_rx_flush () {

	int i;

	for (i = 0; i < 16; i++) {
		cc1100_strobe (CCxxx0_SFRX);
		if (rrf_get_reg (CCxxx0_RXBYTES) == 0)
			return;
	}
}

void rrf_rx_reset () {

	rrf_chip_reset ();
	rrf_enter_rx ();
}

int rrf_cts () {

	// Make sure our status is sane (FIXME: try removing this)
	// rrf_status ();
	cc1100_strobe (CCxxx0_STX);
	// We succeed if we have entered TX
	return (rrf_status () == CC1100_STATE_TX);
}

void rrf_pd () {

	rrf_enter_idle ();
	cc1100_strobe (CCxxx0_SPWD);
#ifndef	__CC430__
	// We do it twice to make sure; kind of stupid, but I do not absolutely
	// trust the SPI interface. No need to be paranoid on the CC430 where
	// the commands are simply written to a register.
	cc1100_strobe (CCxxx0_SPWD);
#endif
}

void rrf_chip_reset () {

	full_reset;

	// Re-initialize registers from the table
	rrf_set_reg_group (rrf_regs);

	// PATABLE is separate
	rrf_set_reg_burst (CCxxx0_PATABLE, rrf_patable, sizeof (rrf_patable));
}

void rrf_init () {

	// Initialize the requisite pins
	ini_regs;
	rrf_chip_reset ();
	// Start in power down
	rrf_pd ();
}
