/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

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
		// byte
		*buffer++ = RF1ADOUT1B;
	}

	// The last one
	*buffer = RF1ADOUT0B;
}

#define	strobe(b) cc1100_strobe (b)

static byte cc1100_strobe (byte b) {

	volatile word sb;

	// Clear the status-read flag 
	RF1AIFCTL1 &= ~(RFSTATIFG);
	  
	// Wait until ready for next instruction
	while (!(RF1AIFCTL1 & RFINSTRIFG));

	// Take the ready status before issuing the command to prevent race.
	// Note that I am trying to follow recommendations in the "official"
	// examples/application notes, while fixing obvious problems in those
	// examples. This is one such fix.
	sb = chip_not_ready;
	// Issue the command
	RF1AINSTRB = b; 

	if (b == CCxxx0_SRES || b == CCxxx0_SNOP)
		goto Ret;

	if (b != CCxxx0_SPWD) {
		// We worry about the chip being ready
		if (sb) {
			// Transiting from SLEEP
			while (chip_not_ready);
			udelay (DELAY_CHIP_READY);
		}
	}

	while ((RF1AIFCTL1 & RFSTATIFG) == 0);
Ret:
	sb = RF1ASTATB;
	return (sb & CC1100_STATE_MASK);
}

#else

// ============================================================================
// SPI BASED (external chip) ==================================================
// ============================================================================

// Needed by full_reset
#define	spi_out(b) cc1100_spi_out (b)

static void cc1100_spi_out (byte b) {

	register int i;

	for (i = 0; i < 8; i++) {
		if (b & 0x80)
			cc1100_si_up;
		else
			cc1100_si_down;
		b <<= 1;
		cc1100_sclk_up;
		CC1100_SPI_WAIT;
		cc1100_sclk_down;
		CC1100_SPI_WAIT;
	}
}

static byte cc1100_spi_out_stat (byte b) {

	register byte i, val;

	for (i = val = 0; i < 8; i++) {
		if (b & 0x80)
			cc1100_si_up;
		else
			cc1100_si_down;
		val <<= 1;
		if (cc1100_so_val)
			val |= 1;
		b <<= 1;
		cc1100_sclk_up;
		CC1100_SPI_WAIT;
		cc1100_sclk_down;
		CC1100_SPI_WAIT;
	}
	return (val & CC1100_STATE_MASK);
}

static byte cc1100_spi_in () {

	register byte i, val;

	for (i = val = 0; i < 8; i++) {
		val <<= 1;
		if (cc1100_so_val)
			val |= 1;
		cc1100_sclk_up;
		CC1100_SPI_WAIT;
		cc1100_sclk_down;
		CC1100_SPI_WAIT;
	}

	return val;
}

void rrf_set_reg (byte addr, byte val) {

	CC1100_SPI_START;
	CC1100_SPI_WAIT;
	cc1100_spi_out (addr);
	cc1100_spi_out (val);
	CC1100_SPI_END;
}

byte rrf_get_reg (byte addr) {

	register byte val;

	CC1100_SPI_START;
	cc1100_spi_out (addr | 0x80);
	val = cc1100_spi_in ();
	CC1100_SPI_END;
	return val;
}

void rrf_set_reg_burst (byte addr, byte *buffer, word count) {

	CC1100_SPI_START;
	cc1100_spi_out (addr | 0x40);
	while (count--)
		cc1100_spi_out (*buffer++);
	CC1100_SPI_END;
}

void rrf_get_reg_burst (byte addr, byte *buffer, word count) {

	CC1100_SPI_START;
	cc1100_spi_out (addr | 0xC0);
	while (count--)
		*buffer++ = cc1100_spi_in ();
	CC1100_SPI_END;
}
	
static void cc1100_strobe (byte cmd) {

	CC1100_SPI_START;
	cc1100_spi_out (cmd);
	CC1100_SPI_END;
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

		CC1100_SPI_START;
		val = cc1100_spi_out_stat (CCxxx0_SNOP | 0x80);
		CC1100_SPI_END;
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

	int res;

	if (cc1100_strobe (CCxxx0_SNOP) == CC1100_STATE_IDLE) {
		// The state is right, check RXBYTES
		if (!((res = rrf_get_reg (CCxxx0_RXBYTES)) & 0x80))
			return res;
	}
#else
	byte b, val;

	CC1100_SPI_START;

	val = cc1100_spi_out_stat (CCxxx0_RXBYTES);

	// Get RXBYTES
	b = cc1100_spi_in ();

	CC1100_SPI_END;

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

void rrf_send (byte *pkt, byte len) {

	rrf_set_reg (CCxxx0_TXFIFO, len);
	rrf_set_reg_burst (CCxxx0_TXFIFO, pkt, len);
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

	cc1100_full_reset;

	// Re-initialize registers from the table
	rrf_set_reg_group (rrf_regs);

	// PATABLE is separate
	rrf_set_reg_burst (CCxxx0_PATABLE, rrf_patable, sizeof (rrf_patable));
}

void rrf_init () {

	// Initialize the requisite pins
	cc1100_ini_regs;
	rrf_chip_reset ();
	rrf_enter_rx ();
}
