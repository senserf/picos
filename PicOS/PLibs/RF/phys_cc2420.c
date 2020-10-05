/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "kernel.h"
#include "tcvphys.h"
#include "cc2420.h"

// ============================================================================
// Chip access functions (SPI) ================================================
// ============================================================================

static void spi_out (byte b) {

	register byte i;

	for (i = 0; i < 8; i++) {
		if (b & 0x80)
			cc2420_si_up;
		else
			cc2420_si_down;
		b <<= 1;
		cc2420_sclk_up;
		CC2420_SPI_WAIT;
		cc2420_sclk_down;
		CC2420_SPI_WAIT;
	}
}

static byte spi_in () {

	register byte i, val;

	for (i = val = 0; i < 8; i++) {
		val <<= 1;
		if (cc2420_so_val)
			val |= 1;
		cc2420_sclk_up;
		CC2420_SPI_WAIT;
		cc2420_sclk_down;
		CC2420_SPI_WAIT;
	}

	return val;
}

static void set_reg (byte addr, word val) {

	CC2420_SPI_START;
	spi_out (addr);
	spi_out ((byte)(val >> 8));
	spi_out ((byte)(val     ));
	CC2420_SPI_END;
}

static word get_reg (byte addr) {

	register word val;

	CC2420_SPI_START;
	spi_out (addr | 0x40);
	val = ((word)spi_in ()) << 8;
	val |= spi_in ();
	CC2420_SPI_END;
	return val;
}
	
static byte strobe (byte b) {
//
// This one sends a byte and retrieves the status value
//
	register byte i, val;

	CC2420_SPI_START;
	for (i = val = 0; i < 8; i++) {
		if (b & 0x80)
			cc2420_si_up;
		else
			cc2420_si_down;
		val <<= 1;
		if (cc2420_so_val)
			val |= 1;
		b <<= 1;
		cc2420_sclk_up;
		CC2420_SPI_WAIT;
		cc2420_sclk_down;
		CC2420_SPI_WAIT;
	}
	CC2420_SPI_END;
	return val;
}

static void chip_reset () {

	CC2420_SPI_END;
	cc2420_reset_down;
	udelay (100);
	cc2420_reset_up;
	udelay (100);
}

// ============================================================================
// End chip access functions ==================================================
// ============================================================================

static const word fmregsv [] = {
	// Values of fixed registers to modify
	0x22E2,		// MDMCTRL0 = 0010 0010 1110 0010
			//              ^
			//	accept all frame types (ignored, see below)
			//	           ^
			//	disable hardware address decoding
			//			^^
			//	CCA = 1 if below threshold (01) & not receiving
			//	(10)
			//			  ^
			//	AUTOCRC -----------
			//			     ^^^^
			//	Preamble length, # leading zeros, 10 == 2
	RADIO_SYSTEM_IDENT,	// SYNCWORD
	0x2A56,		// RXCTRL1  = 0010 1010 0101 0110
			// 	recommended different from default
	0x01C4,		// SECCTRL0 = 0000 0001 1100 0100
			//		     ^ - mac security not used
	0x007F,		// IOCFG0   = 0000 0000 0111 1111
			//                  ^^^ ^ - signal polarity high
			//			 ^^^ ^^^^ - FIFOP threshold
};

static const byte fmregsn [] = {
	// Numbers of registers to modify with fixed values
	CC2x20_MDMCTRL0,
	CC2x20_SYNCWORD,
	CC2x20_RXCTRL1,
	CC2x20_SECCTRL0,
	CC2x20_IOCFG0,
};

static const byte palevels [] = {
//
// Power level settings low to high; combine with 0xA000 and store into TXCTRL
// (reg 0x15)
//
	0xE3, 0xE7, 0xEB, 0xEF, 0xF3, 0xF7, 0xFB, 0xFF
};

#if RADIO_LBT_MODE == 3
static const byte ccathresholds [] = {
	(byte)(-32), (byte)(-32), (byte)(-24), (byte)(-16), (byte)(-8),
	(byte)( -4), 0          , 0x7F };
#endif

static byte	ccaths	= RADIO_CC_THRESHOLD,
	   	power	= RADIO_DEFAULT_POWER,
	   	channel	= RADIO_DEFAULT_CHANNEL,
	   	rbuffl,		// Input buffer length
		TxFF = NO,	// Transmit FIFO full flag
		RxST;

#if RADIO_LBT_MODE == 3
static byte	retr = 0;	// Retry counter for stage LBT
#endif

static word	*rbuff = NULL,
		physid,
		statid = 0,
		bckf_timer = 0;

word		__cc2420_v_drvprcs, __cc2420_v_qevent;

//
// RxST states
//
#define	RCV_STATE_PD	2
#define	RCV_STATE_OFF	1
#define RCV_STATE_ON	0

// ============================================================================

static void setpower (byte lev) {

	set_reg (CC2x20_TXCTRL, 0xA000 | palevels [power = lev]);
}

static void setchannel (byte cha) {

	set_reg (CC2x20_FSCTRL,
		0x4000 | (CC2420_FREQ_BASE + 5 * (channel = cha)));
}

static void setccaths (byte cat) {

	set_reg (CC2x20_RSSI, ((word)(ccaths = cat)) << 8);
}

#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
static void dmpr (const char *m) {
	diag ("%s: %x", m, strobe (CC2x20_SNOP));
	diag (" 11 = %x", get_reg (CC2x20_MDMCTRL0));
	diag (" 13 = %x", get_reg (CC2x20_RSSI));
}
#endif

static void load_regs () {

	int i;

	for (i = 0; i < sizeof (fmregsn); i++)
		set_reg (fmregsn [i], fmregsv [i]);

	setpower (power);
	setchannel (channel);
	setccaths (ccaths);

#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
	dmpr ("LOAD");
#endif

}

static void enter_rx () {

	strobe (CC2x20_SRXON);
	while (cc2420_fifop_val && !cc2420_fifo_val) {
		// This means RXFIFO overflow, we have to flush it
		strobe (CC2x20_SFLUSHRX);
		// They say it should be flushed twice to clear SFD
		// (do we need it?)
		strobe (CC2x20_SFLUSHRX);
	}
#if 1
	while (!(strobe (CC2x20_SNOP) & CC2x20_STAT_PLLOCK));
#endif
}

static void osc_on () {
//
// Turn on the oscillator and enter IDLE
//
	int i;

	// Power up, only SXOSCON strobe is available
	for (i = 0; i < 32; i++) {
		if (strobe (CC2x20_SXOSCON) & CC2x20_STAT_XOSCON)
			// Done
			return;
		mdelay (1);
	}

	syserror (EHARDWARE, "cc24 up");
}

static void power_up () {
//
// Return from sleep
//
	int i;

#ifdef MONITOR_PIN_CC2420_PUP
	_PVS (MONITOR_PIN_CC2420_PUP, 1);
#endif

#if RADIO_DEEP_POWER_DOWN
	// Deep power down mode, re-enable VREG
	cc2420_vrgen_up;
	mdelay (1);
	chip_reset ();
#endif
	osc_on ();

	// The doc implies that if we are here, the chip is in IDLE

#if RADIO_DEEP_POWER_DOWN
	load_regs ();
#endif
#ifdef MONITOR_PIN_CC2420_PUP
	_PVS (MONITOR_PIN_CC2420_PUP, 0);
#endif
}

static void power_down () {

	strobe (CC2x20_SXOSCOFF);
#if RADIO_DEEP_POWER_DOWN
	// They say the chip should be reset before disabling the regulator
	chip_reset ();
	cc2420_vrgen_down;
#endif
	RxST = RCV_STATE_PD;
}

// ============================================================================

static void do_rx_fifo () {

	int len, paylen;
	byte b, *eptr;

	LEDI (2, 1);

#ifdef	MONITOR_PIN_CC2420_RX
	_PVS (MONITOR_PIN_CC2420_RX, 1);
#endif
	if (RxST || !cc2420_fifo_val) {
		// Overflow, or switched off, ignore everything (perhaps we
		// should be more careful about this)
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
		diag ("CC2420: %u RX RXOVF", (word) seconds ());
#endif
Clear:
		strobe (CC2x20_SFLUSHRX);
		strobe (CC2x20_SFLUSHRX);
		goto Rtn;
	}

	// Start reading RXFIFO
	CC2420_SPI_START;
	spi_out (CC2x20_RXFIFO | 0x40);

	// Get and validate the length byte
	len = spi_in ();


	if (len & 1 || len < 4 || len > rbuffl) {
		// Bad length, for now clear the whole thing
		CC2420_SPI_END;
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
		diag ("CC2420: %u RX BADL %u", (word) seconds (), len);
#endif
		goto Clear;
	}

	for (paylen = 0; paylen < len && cc2420_fifo_val; paylen++)
		((byte*)rbuff) [paylen] = spi_in ();

	// Done reading FIFO
	CC2420_SPI_END;

	if (paylen != len) {
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
		diag ("CC2420: %u RX LMIS %u %u", (word) seconds (), len,
			paylen);
#endif
		goto Clear;
	}

	if (statid != 0 && statid != 0xffff) {
		// Admit only packets with agreeable statid
		if (rbuff [0] != 0 && rbuff [0] != statid) {
			// Drop
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
			diag ("CC2420: %u RX BAD STID: %x", (word) seconds (),
				rbuff [0]);
#endif
			goto Rtn;
		}
	}
	eptr = ((byte*)rbuff) + paylen - 2;
	add_entropy (*eptr);
	b = *(eptr+1);
	*(eptr+1) = *((char*)eptr) + 128;
	*eptr = b;

#if RADIO_CRC_TRANSPARENT == 0
	if ((b & 0x80) == 0) {
		// Bad CRC
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
		diag ("CC2420: %u RX CKS (H) %x %x %x", (word) seconds (),
			(word*)(rbuff) [0],
			(word*)(rbuff) [1],
			(word*)(rbuff) [2]);
#endif
		// Ignore
		goto Rtn;
	}
#endif	/* RADIO_CRC_TRANSPARENT */

	// Reception

#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
	diag ("CC2420: %u RX OK %x %x %x", (word) seconds (),
		(word*)(rbuff) [0],
		(word*)(rbuff) [1],
		(word*)(rbuff) [2]);
#endif
	tcvphy_rcv (physid, rbuff, paylen);
Rtn:
	gbackoff (RADIO_LBT_BACKOFF_RX);
	LEDI (2, 0);

#ifdef	MONITOR_PIN_CC2420_RX
	_PVS (MONITOR_PIN_CC2420_RX, 0);
#endif
}

#define	DR_LOOP		0
#define	DR_SWAIT	1

thread (cc2420_driver)

  address xbuff;
  int i, paylen;

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
  address ppm;
  word pcav;
#endif

  entry (DR_LOOP)

DR_LOOP__:

	if (RxST != RCV_STATE_PD) {
		while (cc2420_fifop_val)
			do_rx_fifo ();
	}

	// Is there anything to transmit?
	if (TxFF == NO && (xbuff = tcvphy_top (physid)) == NULL) {
		wait (__cc2420_v_qevent, DR_LOOP);
		if (RxST == RCV_STATE_OFF)
			// Nothing to transmit, power down
			power_down ();
		if (RxST != RCV_STATE_PD)
			cc2420_rcv_int_enable;
		release;
	}

	// There is a packet to transmit
#ifdef	MONITOR_PIN_CC2420_TXP
	_PVS (MONITOR_PIN_CC2420_TXP, 1);
#endif
	if (TxFF == NO) {
		// Fill in the FIFO first
#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
		// Check for CAV requested in the packet
		ppm = xbuff + ((tcv_tlength (xbuff) >> 1) - 1);
		if ((pcav = (*ppm) & 0x0fff)) {
			// Remove for next turn
			*ppm &= ~0x0fff;
			utimer_set (bckf_timer, pcav);
			goto Bkf;
		}
#if RADIO_LBT_MODE == 3
		if (*ppm & 0x8000)
			// LBT off requested by the packet
			retr = LBT_RETR_FORCE_OFF;
#endif
#endif
		if (RxST == RCV_STATE_PD) {
			// Need to power up to fill the TXFIFO
			power_up ();
			enter_rx ();
			RxST = RCV_STATE_OFF;
#if 1
			while (cc2420_fifop_val)
				do_rx_fifo ();
#endif
		}

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS) && RADIO_POWER_SETTABLE
		if ((byte)(pcav = (*ppm >> 12) & 0x7) != power)
			setpower ((byte)pcav);
#endif
		// This cannot possibly fail, xbuf already assigned
		tcvphy_get (physid, &paylen);
		// A packet arriving from TCV always contains CRC slots, even
		// if we are into hardware CRC; its minimum legit length is
		// SID + CRC
		sysassert (paylen <= rbuffl && paylen >= 4 && (paylen & 1) == 0,
			"cc24 py");
		// Fill the FIFO
		strobe (CC2x20_SFLUSHTX);
#if 0
		CC2420_SPI_START;
		spi_out (CC2x20_TXFIFO);
		spi_out ((byte)paylen);
		CC2420_SPI_END;
		for (i = 0; i < paylen; i++) {
			CC2420_SPI_START;
			spi_out (CC2x20_TXFIFO);
			spi_out (((byte*)xbuff) [i]);
			CC2420_SPI_END;
		}
#endif
#if 1
		CC2420_SPI_START;
		spi_out (CC2x20_TXFIFO);
		spi_out ((byte)paylen);
		paylen -= 2;
		for (i = 0; i < paylen; i++)
			spi_out (((byte*)xbuff) [i]);
		CC2420_SPI_END;
#endif
		TxFF = YES;
		tcvphy_end (xbuff);
	}
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
	else {
		dmpr ("FIF");
	}
#endif
	if (bckf_timer) {
		// Backoff wait
#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
Bkf:
#endif
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
		diag ("BKF");
#endif
		wait (__cc2420_v_qevent, DR_LOOP);
		delay (bckf_timer, DR_LOOP);
		if (RxST != RCV_STATE_PD)
			cc2420_rcv_int_enable;
		release;
	}

#if RADIO_LBT_MODE == 3
	setccaths (ccathresholds [retr]);
	// udelay (10);
#endif
	// Attempt transmission
	strobe (CC2x20_STXONCCA);
	if (!(strobe (CC2x20_SNOP) & CC2x20_STAT_TXACTV)) {
		// Channel busy
#if RADIO_LBT_MODE == 3
		if (retr < LBT_RETR_LIMIT-1) {
			retr++;
		}
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
		else {
			diag ("CC2420: RTL!! %x", strobe (CC2x20_SNOP));
			dmpr ("RTL!");
			goto Skip;
		}
#endif
#endif

#if RADIO_LBT_BACKOFF_EXP == 0
		// Aggressive transmitter
		delay (1, DR_LOOP);
		release;
#else
		// Backoff
		gbackoff (RADIO_LBT_BACKOFF_EXP);
		goto DR_LOOP__;
#endif
	}

	// Channel access granted

	LEDI (1, 1);

#ifdef	MONITOR_PIN_CC2420_TXS
	_PVS (MONITOR_PIN_CC2420_TXS, 1);
#endif
	// Wait until done, get hold of SD
SWLoop:
	delay (1, DR_SWAIT);
	release;

  entry (DR_SWAIT)

	if (cc2420_sfd_val)
		goto SWLoop;


#if RADIO_LBT_XMIT_SPACE
	utimer_set (bckf_timer, RADIO_LBT_XMIT_SPACE);
#endif

Skip:
	LEDI (1, 0);
	TxFF = NO;

#ifdef	MONITOR_PIN_CC2420_TXS
	_PVS (MONITOR_PIN_CC2420_TXS, 0);
#endif
	retr = 0;
	goto DR_LOOP__;

endthread

#undef	DR_LOOP
#undef	DR_SWAIT

// ============================================================================

static int option (int opt, address val) {

	int ret = 0;

	switch (opt) {

	    case PHYSOPT_STATUS:

		ret = 2 | (RxST == RCV_STATE_ON);
		goto RVal;

	    case PHYSOPT_RXON:

		if (RxST == RCV_STATE_PD) {
			// We have been switched off
			power_up ();
			enter_rx ();
		}
		RxST = RCV_STATE_ON;
		LEDI (0, 1);
OREvnt:
		p_trigger (__cc2420_v_drvprcs, __cc2420_v_qevent);

	    case PHYSOPT_TXON:
	    case PHYSOPT_TXOFF:
	    case PHYSOPT_TXHOLD:

		goto RRet;

	    case PHYSOPT_RXOFF:

		if (RxST == RCV_STATE_ON)
			RxST = RCV_STATE_OFF;
		LEDI (0, 0);
		goto OREvnt;

	    case PHYSOPT_SETSID:

		statid = (val == NULL) ? 0 : *val;
		goto RRet;

            case PHYSOPT_GETSID:

		ret = (int) statid;
		goto RVal;

	    case PHYSOPT_GETMAXPL:

		ret = rbuffl;
		goto RVal;

#if RADIO_CAV_SETTABLE

	    case PHYSOPT_CAV:

		// Force an explicit backoff
		if (val == NULL)
			// Random backoff
			gbackoff (RADIO_LBT_BACKOFF_EXP);
		else
			utimer_set (bckf_timer, *val);
		goto OREvnt;
#endif

#if RADIO_POWER_SETTABLE

	    case PHYSOPT_GETPOWER:

		ret = (int) power;
		goto RVal;
#endif

#if RADIO_CHANNEL_SETTABLE

	    case PHYSOPT_GETCHANNEL:

		ret = (int) channel;
		goto RVal;
#endif
	}

	// These require power up
	if (RxST == RCV_STATE_PD)
		power_up ();

	switch (opt) {

#if RADIO_POWER_SETTABLE && (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS) == 0

	    case PHYSOPT_SETPOWER:

		setpower ((val == NULL) ? RADIO_DEFAULT_POWER :
			((*val > 7) ? 7 : (byte) (*val)));
		break;
#endif

#if RADIO_CHANNEL_SETTABLE

	    case PHYSOPT_SETCHANNEL:

		setchannel ((val == NULL) ? RADIO_DEFAULT_CHANNEL :
			((*val > 15) ? 15 : (byte) (*val)));
		break;

#endif

	    default:

		syserror (EREQPAR, "cc24 op");
	}

	if (RxST == RCV_STATE_PD)
		power_down ();

RRet:
	return ret;

RVal:
	if (val != NULL)
		*val = ret;

	goto RRet;
}

// ============================================================================

void phys_cc2420 (int phy, int mbs) {

#if (RADIO_OPTIONS & RADIO_OPTION_NOCHECKS) == 0
	if (rbuff != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "cc24");
#endif

	if (mbs == 0)
		mbs = CC2420_MAXPLEN;

#if (RADIO_OPTIONS & RADIO_OPTION_NOCHECKS) == 0
	if (mbs < 6 || mbs > CC2420_MAXPLEN)
		syserror (EREQPAR, "cc24 mb");
#endif

	rbuffl = (byte) mbs;

	rbuff = umalloc (rbuffl);

#if (RADIO_OPTIONS & RADIO_OPTION_NOCHECKS) == 0
	if (rbuff == NULL)
		syserror (EMALLOC, "cc24");
#endif
	physid = phy;

	// Register the phy
	__cc2420_v_qevent = tcvphy_reg (phy, option, INFO_PHYS_CC2420);

	LEDI (0, 0);
	LEDI (1, 0);
	LEDI (2, 0);

#if RADIO_DEEP_POWER_DOWN == 0
	// power_up won't do this
	cc2420_vrgen_up;
	mdelay (1);
	chip_reset ();
	osc_on ();
	load_regs ();
	// We are in idle
#endif
	power_down ();

#if DIAG_MESSAGES
	diag ("CC2420: 2.4GHz, %d", RADIO_DEFAULT_CHANNEL);
#endif

#if (RADIO_OPTIONS & RADIO_OPTION_ENTROPY) && ENTROPY_COLLECTION

	// Collect initial entropy
	{
		word i;

		power_up ();
		enter_rx ();

		for (i = 0; i < 8; i++) {
			mdelay (1);
			entropy = (entropy << 4) |
				(get_reg (CC2x20_RSSI) & 0xF);
		}
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
		diag ("CC2420 ENTR: %x%x", (word)entropy, (word)(entropy>>16));
#endif
		power_down ();
	}
#endif

	// Install the backoff timer
	utimer_add (&bckf_timer);

	// Start the driver process
	__cc2420_v_drvprcs = runthread (cc2420_driver);
#if (RADIO_OPTIONS & RADIO_OPTION_NOCHECKS) == 0
	if (__cc2420_v_drvprcs == 0)
		syserror (ERESOURCE, "cc24");
#endif
}
