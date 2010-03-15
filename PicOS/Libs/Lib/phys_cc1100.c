/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define	DEFINE_RF_SETTINGS	1

#include "kernel.h"
#include "tcvphys.h"
#include "cc1100.h"

#define	GDELAY(s) 	ldelay (GUARD_LONG_DELAY, s)

static int option (int, address);
static void chip_reset();

		// Pointer to the static reception buffer
static word	*rbuff = NULL,
		bckf_timer = 0,

		physid,
		statid = 0;

#if (RADIO_GUARD & 0x01)
static byte	gwch;			// Guard watch
#endif

#if RADIO_TRACK_ERRORS
static word	rerror [4];
#endif

word		zzv_drvprcs, zzv_qevent;

static byte	RxOFF,			// Transmitter on/off flags
		TxOFF,
		xpower,			// Power select
		rbuffl,
		vrate = RADIO_BITRATE_INDEX,	// Rate select
		channr = 0;

#if (RADIO_GUARD & 0x01)
// Guard process present ======================================================

#define	WATCH_RCV	0x01
#define WATCH_XMT	0x02
#define	WATCH_PRG	0x80
#define	WATCH_HNG	0x0F

#define		guard_start(f)	_BIS (gwch, (f))
#define		guard_stop(f)	_BIC (gwch, (f))
#define		guard_hung	gwch
#define		guard_clear	(gwch = 0)

#else

#define		guard_start(f)	do { } while (0)
#define		guard_stop(f)	do { } while (0)

#endif
// ============================================================================

/* ========================================= */

static byte patable [] = CC1100_PATABLE;

#ifdef	__CC430__

// ============================================================================
// CC430 (internal MSP430) ====================================================
// ============================================================================

static void cc1100_set_reg (byte addr, byte val) {

	// volatile word i;

	// Wait for ready
	while (!(RF1AIFCTL1 & RFINSTRIFG));

	// Address + instruction
	RF1AINSTRB = (addr | RF_REGWR);
	RF1ADINB = val;

	// Wait until complete
	// while (!(RF1AIFCTL1 & RFDINIFG));

	// Must read this
	// i = RF1ADOUTB;  
}

static byte cc1100_get_reg (byte addr) {

	byte val;

	RF1AINSTR1B = (addr | 0x80); 
	// A simple return should do (check later)
  	while (!(RF1AIFCTL1 & RFDOUTIFG));
	val = RF1ADOUT1B;
	return val;
}

static void cc1100_set_reg_burst (byte addr, byte *buffer, word count) {
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

static void cc1100_get_reg_burst (byte addr, byte *buffer, word count) {

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

	while (b != CCxxx0_SRES && (RF1AIFCTL1 & RFSTATIFG) == 0);

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

static void cc1100_set_reg (byte addr, byte val) {

	SPI_START;
	SPI_WAIT;
	cc1100_spi_out (addr);
	cc1100_spi_out (val);
	SPI_END;
}

static byte cc1100_get_reg (byte addr) {

	register byte val;

	SPI_START;
	cc1100_spi_out (addr | 0x80);
	val = cc1100_spi_in ();
	SPI_END;
	return val;
}

static void cc1100_set_reg_burst (byte addr, byte *buffer, word count) {

	SPI_START;
	cc1100_spi_out (addr | 0x40);
	while (count--)
		cc1100_spi_out (*buffer++);
	SPI_END;
}

static void cc1100_get_reg_burst (byte addr, byte *buffer, word count) {

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

static void cc1100_set_power () {

	cc1100_set_reg (CCxxx0_FREND0, FREND0_SET | xpower);
}

static byte cc1100_setchannel (byte ch) {

	byte old;

	old = channr;
	channr = ch;
	cc1100_set_reg (CCxxx0_CHANNR, channr);
	return old;
}

static word cc1100_setparam (byte *pa) {

	word cnt;

	if (pa == NULL) {
		chip_reset ();
		return 0;
	}

	cnt = 0;

	while (*pa != 255) {
		cc1100_set_reg (*pa, *(pa+1));
		pa += 2;
		cnt++;
	}

	return cnt;
}

static void init_cc_regs () {

	const byte *cur;

	for (cur = cc1100_rfsettings; *cur != 255; cur += 2) {
		cc1100_set_reg (cur [0], cur [1]);
	}

	// Rate-specific registers
	for (cur = cc1100_ratemenu [vrate]; *cur != 255; cur += 2) {
		cc1100_set_reg (cur [0], cur [1]);
	}

	// Power setting is handled separately
	cc1100_set_reg_burst (CCxxx0_PATABLE, patable, sizeof (patable));

	cc1100_set_power ();
	// And so is the channel number
	cc1100_setchannel (channr);
}

static byte cc1100_status () {

	register byte val;
	int i;
ReTry:
	for (i = 0; i < 32; i++) {

		i--;

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
		mdelay (1);
	}
#if RADIO_DEBUG
	diag ("CC1100: %u STATUS HUNG!!", (word) seconds ());
#endif
	chip_reset ();
	goto ReTry;
}

static void enter_idle () {

	int i;

#if (RADIO_GUARD & 0x02)
// ============================================================================

ReTry:
	i = 32;

	while (cc1100_status () != CC1100_STATE_IDLE) {
		i--;
		cc1100_strobe (CCxxx0_SIDLE);
		if (i < 16) {
			if (i == 0) {
#if RADIO_DEBUG
				diag ("CC1100: %u IDLE HUNG!!",
					(word) seconds ());
#endif
				chip_reset ();
				goto ReTry;

			}
			mdelay (2);
		}
	}
#else
// ============================================================================
	for (i = 0; i < 16; i++) {
		if (cc1100_status () == CC1100_STATE_IDLE)
			return;
		cc1100_strobe (CCxxx0_SIDLE);
	}
#endif
// ============================================================================
}

static void enter_rx () {

	int i;

#if RADIO_SPDIFIDLE
	powerup ();
#endif

#if (RADIO_GUARD & 0x02)
// ============================================================================
ReTry:
	i = 32;
	while (cc1100_status () != CC1100_STATE_RX) {
		i--;
		cc1100_strobe (CCxxx0_SRX);
		if (i < 16) {
			if (i == 0) {
#if RADIO_DEBUG
				diag ("CC1100: %u ENTER RX HUNG!!",
					(word) seconds ());
#endif
				chip_reset ();
				goto ReTry;
			}
			mdelay (2);
		}
	}
#else
// ============================================================================
	for (i = 0; i < 16; i++) {
		if (cc1100_status () == CC1100_STATE_RX)
			return;
		cc1100_strobe (CCxxx0_SRX);
	}
#endif
// ============================================================================
}

int cc1100_rx_status () {

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

	if (val == CC1100_STATE_IDLE)
		// The status is right, return #bytes in RX FIFO
		return (b & 0x7f);
#if RADIO_DEBUG
	diag ("CC1100: %u RXST = %x/%x", (word) seconds (), val, b);
#endif

#endif
	return -1;
}

void cc1100_rx_flush () {

	int i;

#if (RADIO_GUARD & 0x02)
// ============================================================================
ReTry:
	i = 32;
	while (1) {
		cc1100_strobe (CCxxx0_SFRX);
		if (cc1100_get_reg (CCxxx0_RXBYTES) == 0)
			return;	// FIXME
		i--;
		if (i < 16) {
			if (i == 0) {
#if RADIO_DEBUG
				diag ("CC1100: %u FLUSH HUNG!!",
					(word) seconds ());
#endif
				mdelay (1);
				chip_reset ();
				mdelay (1);
				goto ReTry;
			}
			mdelay (2);
		}
	}
#else
// ============================================================================

	for (i = 0; i < 16; i++) {
		cc1100_strobe (CCxxx0_SFRX);
		if (cc1100_get_reg (CCxxx0_RXBYTES) == 0)
			return;
	}
#endif
// ============================================================================
}

void cc1100_rx_reset () {

	chip_reset ();
	enter_rx ();
}

static word clear_to_send () {

	// Make sure our status is sane (FIXME: try removing this)
	cc1100_status ();
	cc1100_strobe (CCxxx0_STX);
	// We succeed if we have entered TX
	return (cc1100_status () == CC1100_STATE_TX);
}

static void power_down () {

#if RADIO_TRACE
	diag ("CC1100: %u POWER DOWN", (word) seconds ());
#endif
	enter_idle ();
	cc1100_strobe (CCxxx0_SPWD);
	STROBE_WAIT;
	cc1100_strobe (CCxxx0_SPWD);
#if RADIO_SPDIFIDLE
	powerdown ();
#endif
}

static void chip_reset () {

	full_reset;
	init_cc_regs ();
#if RADIO_TRACE
	diag ("CC1100: %u CHIP RESET", (word) seconds ());
#endif
}

static void ini_cc1100 () {

	// Initialize the requisite pins
	ini_regs;

	chip_reset ();
	power_down ();

	// Read the chip number reg and write a message to the UART

#if DIAG_MESSAGES

	diag ("CC1100 initialized: %d [%x] %x",
		vrate,
		(cc1100_get_reg (CCxxx0_SYNC1)   << 8) |
		 cc1100_get_reg (CCxxx0_SYNC0)		,
		(cc1100_get_reg (CCxxx0_VERSION) << 8) |
		 cc1100_get_reg (CCxxx0_MCSM1)		);
#endif

	dbg_1 (0x2000); // CC1100 initialized
	dbg_1 ((cc1100_get_reg (CCxxx0_SYNC1) << 8) |
			cc1100_get_reg (CCxxx0_SYNC0));
	dbg_1 ((cc1100_get_reg (CCxxx0_VERSION) << 8) |
			cc1100_get_reg (CCxxx0_MCSM1));
}

#if RADIO_CRC_MODE > 1
#include "checksum.h"
#endif

static void do_rx_fifo () {

	int len, paylen;
	byte *eptr;

#if RADIO_CRC_MODE <= 1
	byte b;
#endif
	// We are making progress as far as reception
	guard_stop (WATCH_RCV | WATCH_PRG);

	if (RxOFF) {
		// If we are switched off, just clean the FIFO and return
#if RADIO_TRACE
		diag ("CC1100: %u RX OFF CLEANUP", (word) seconds ());
#endif
		cc1100_rx_flush ();
		goto Rtn;
	}

	if ((len = cc1100_rx_status ()) < 0) {
		// Error: normally FIFO overrun (shouldn't happen)
#if RADIO_DEBUG
		diag ("CC1100: %u RX BAD STATUS", (word) seconds ());
#endif
		cc1100_rx_reset ();
		// Skip reception
		goto Rtn;
	}

#if RADIO_TRACK_ERRORS
	if (rerror [0] == MAX_WORD)
		memset (rerror, 0, sizeof (rerror));
	rerror [0] ++;
#endif

	if ((len & 1) == 0 || len < 7) {
		// Actual payload length must be even
#if RADIO_TRACK_ERRORS
		rerror [2] ++;
#endif
#if RADIO_DEBUG
		diag ("CC1100: %u RX BAD PL: %d", (word) seconds (), len);
#endif
		cc1100_rx_reset ();
		goto Rtn;
	}

	paylen = cc1100_get_reg (CCxxx0_RXFIFO);
	if ((paylen & 1) || paylen != len - 3) {
#if RADIO_TRACK_ERRORS
		rerror [2] ++;
#endif
#if RADIO_DEBUG
		diag ("CC1100: %u RX PL MIS: %d/%d", (word) seconds (), len,
			paylen);
#endif
		cc1100_rx_reset ();
		goto Rtn;
	}

	if (paylen > rbuffl) {
#if RADIO_TRACK_ERRORS
		rerror [2] ++;
#endif
#if RADIO_DEBUG
		diag ("CC1100: %u RX LONG: %d", (word) seconds (), paylen);
#endif
		cc1100_rx_reset ();
		goto Rtn;
	}

	// Include the status bytes
	cc1100_get_reg_burst (CCxxx0_RXFIFO, (byte*)rbuff, (byte) paylen + 2);

	// We have extracted the packet, so we can start RCV for another one

#if 0
	// A precaution: make sure the FIFO is truly empty
	while (RX_FIFO_READY) {
#if RADIO_DEBUG
		diag ("CC1100: %u RX FIFO NONEMPTY", (word) seconds ());
#endif
		cc1100_rx_reset ();
	}
#endif

	enter_rx ();

	if (statid != 0 && statid != 0xffff) {
		// Admit only packets with agreeable statid
		if (rbuff [0] != 0 && rbuff [0] != statid) {
			// Drop
#if RADIO_TRACK_ERRORS
			rerror [3] ++;
#endif
#if RADIO_DEBUG
			diag ("CC1100: %u RX BAD STID: %x", (word) seconds (),
				rbuff [0]);
#endif
			goto Rtn;
		}
	}

#if RADIO_CRC_MODE > 1
	// Verify CRC
	len = paylen >> 1;
	if (w_chk (rbuff, len, 0)) {
		// Bad checksum
#if RADIO_TRACK_ERRORS
		rerror [1] ++;
#endif
#if RADIO_DEBUG
		diag ("CC1100: %u RX CKS (S) %x %x %x", (word) seconds (),
			(word*)(rbuff) [0],
			(word*)(rbuff) [1],
			(word*)(rbuff) [2]);
#endif
		// Ignore
		goto Rtn;
	}

	eptr = (byte*)rbuff + paylen;

	((byte*)rbuff) [paylen - 2] = (*(eptr + 1) & 0x7f);
	// This number is signed and starts negative, so we have
	// to bias it properly
	((byte*)rbuff) [paylen - 1] = *((char*)eptr) + 128;
	add_entropy (rbuff [len-1]);

#else	/* RADIO_CRC_MODE (the hardware case) */

	// Status bytes
	eptr = (byte*)rbuff + paylen;
	b = *(eptr+1);
	add_entropy (rbuff [len]);
	paylen += 2;

	if (b & 0x80) {
		// CRC OK
		*(eptr+1) = *((char*)eptr) + 128;
		// Swap these two
		*eptr = (b & 0x7f);
	} else {
		// Bad checksum
#if RADIO_TRACK_ERRORS
		rerror [1] ++;
#endif
#if RADIO_DEBUG
		diag ("CC1100: %u RX CKS (H) %x %x %x", (word) seconds (),
			(word*)(rbuff) [0],
			(word*)(rbuff) [1],
			(word*)(rbuff) [2]);
#endif
		// Ignore
		goto Rtn;
	}
#endif	/* RADIO_CRC_MODE */

#if RADIO_TRACE
	diag ("CC1100: %u RX OK %x %x %x", (word) seconds (),
		(word*)(rbuff) [0],
		(word*)(rbuff) [1],
		(word*)(rbuff) [2]);
#endif
	tcvphy_rcv (physid, rbuff, paylen);
Rtn:
	if (backoff_after_receive)
		gbackoff;
}

#define	DR_LOOP		0
#define	DR_SWAIT	1

thread (cc1100_driver)

  address xbuff;
  int paylen, len;

  entry (DR_LOOP)

	if (TxOFF & 1) {
		// The transmitter is OFF solid
		if (RxOFF == 1) {
			// Power down the chip
			power_down ();
			RxOFF = 2;
		}
		if (TxOFF == 3)
			tcvphy_erase (physid);

		// Receive or drain
		while (RX_FIFO_READY) {
			LEDI (2, 1);
			do_rx_fifo ();
			LEDI (2, 0);
		}
XRcv:
		wait (zzv_qevent, DR_LOOP);
		if (RxOFF == 0)
			rcv_enable_int;
		release;
	}

	while (RX_FIFO_READY) {
		LEDI (2, 1);
		do_rx_fifo ();
		LEDI (2, 0);
	}

	if (bckf_timer) {
		delay (bckf_timer, DR_LOOP);
		goto XRcv;
	}
	
	if (tcvphy_top (physid) == NULL) {
		// Nothing to transmit
		if (TxOFF) {
			// TxOFF == 2 -> draining: stop xmt
			TxOFF = 3;
			proceed (DR_LOOP);
		}
		// Wait
		goto XRcv;
	}

	// Try to grab the chip for TX
	if (clear_to_send () == NO) {
		// We have to wait
		if (aggressive_transmitter) {
			delay (1, DR_LOOP);
			release;
		} else {
			gbackoff;
			proceed (DR_LOOP);
		}
	}

	if ((xbuff = tcvphy_get (physid, &paylen)) == NULL) {
		// The last check: the packet may have been removed while
		// waiting on LBT
		enter_rx ();
		proceed (DR_LOOP);
	}

#if RADIO_CRC_MODE > 1
	sysassert (paylen <  rbuffl && paylen >= 6 && (paylen & 1) == 0,
		"phys_cc1100 xmt pktl");
#else
	sysassert (paylen <= rbuffl && paylen >= 6 && (paylen & 1) == 0,
		"phys_cc1100 xmt pktl");
#endif
	LEDI (1, 1);

	if (statid != 0xffff)
		// This means "honor the packet's statid
		xbuff [0] = statid;

#if RADIO_CRC_MODE > 1
	// Calculate CRC
	len = (paylen >> 1) - 1;
	((word*)xbuff) [len] = w_chk ((word*)xbuff, len, 0);
#else
	paylen -= 2;		// Ignore the checksum bytes
#endif
	// Send the length byte ...
	cc1100_set_reg (CCxxx0_TXFIFO, paylen);

	// ... and the packet
	cc1100_set_reg_burst (CCxxx0_TXFIFO, (byte*)xbuff, (byte) paylen);

	// ... and release it
	tcvphy_end (xbuff);

	// Wait for some minimum time needed to transmit the packet

	delay (approx_xmit_time (paylen), DR_SWAIT);

	release;

  entry (DR_SWAIT)

#ifdef CC_BUSY_WAIT_FOR_EOT
	while (cc1100_status () == CC1100_STATE_TX);
#else
	if (cc1100_status () == CC1100_STATE_TX) {
		delay (TXEND_POLL_DELAY, DR_SWAIT);
		release;
	}
#endif
	guard_stop (WATCH_XMT | WATCH_PRG);
	LEDI (1, 0);
	bckf_timer = XMIT_SPACE;
	proceed (DR_LOOP);

endthread

#if (RADIO_GUARD & 0x01)
// ============================================================================

#define	GU_ACTION	0

thread (cc1100_guard)

  word stat;

  entry (GU_ACTION)

#if RADIO_TRACE
	diag ("CC1100: %u GUARD ...", (word) seconds ());
#endif
	if (guard_hung) {
#if RADIO_DEBUG
		diag ("CC1100: %u GUARD RESET: %x", (word) seconds (), gwch);
#endif
Reset:
		guard_clear;
		chip_reset ();
		enter_rx ();
		p_trigger (zzv_drvprcs, ETYPE_USER, zzv_qevent);
		GDELAY (GU_ACTION);
		release;
	}

	if ((TxOFF & 1) && RxOFF) {
		// The chip is powered down, so don't bother
		GDELAY (GU_ACTION);
		release;
	}

	if (RX_FIFO_READY) {
		// This one should go away eventually
		guard_start (WATCH_RCV);
		delay (GUARD_SHORT_DELAY, GU_ACTION);
		// Won't hurt
		p_trigger (zzv_drvprcs, ETYPE_USER, zzv_qevent);
		release;
	}

	stat = cc1100_status ();

	if (stat == CC1100_STATE_TX) {
		// This one will go away eventually as well
		guard_start (WATCH_XMT);
		delay (GUARD_SHORT_DELAY, GU_ACTION);
		p_trigger (zzv_drvprcs, ETYPE_USER, zzv_qevent);
		release;
	}

	if (stat != CC1100_STATE_RX) {
		// Something is wrong: note that stat == IDLE implies
		// RX_FIFO_READY
#if RADIO_DEBUG
		diag ("CC1100: %u GUARD BAD ST: %d", (word) seconds (), stat);
#endif
		goto Reset;
	}

	// Do not break reception in progress
	stat = cc1100_get_reg (CCxxx0_RXBYTES);

	if (stat & 0x80) {
		// Overflow
#if RADIO_DEBUG
		diag ("CC1100: %u GUARD FIFO", (word) seconds ());
#endif
		goto Reset;
	}

	if (stat == 0) {
		// Recalibrate
#if RADIO_TRACE
		diag ("CC1100: %u GUARD RECAL", (word) (word) seconds ());
#endif
		enter_idle ();
		enter_rx ();
		// Will reset the chip periodically on LONG_DELAY if nothing
		// happens in between
		guard_start (WATCH_PRG);
		GDELAY (GU_ACTION);
	} else {
		guard_start (WATCH_RCV);
		delay (GUARD_SHORT_DELAY, GU_ACTION);
	}
	p_trigger (zzv_drvprcs, ETYPE_USER, zzv_qevent);

endthread

#endif	/* RADIO_GUARD */

void phys_cc1100 (int phy, int mbs) {
/*
 * mbs does not cover the checksum
 */
	if (rbuff != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "phys_cc1100");

	if (mbs < 6 || mbs > CC1100_MAXPLEN) {
		if (mbs == 0)
			mbs = CC1100_MAXPLEN;
		else
			syserror (EREQPAR, "phys_cc1100 mbs");
	}

	rbuffl = (byte) mbs;	// buffer length in bytes, excluding checksum
#if RADIO_CRC_MODE > 1
	rbuffl += 2;
#endif
	if ((rbuff = umalloc (rbuffl)) == NULL)
		syserror (EMALLOC, "phys_cc1100");

	statid = 0;
	physid = phy;

	/* Register the phy */
	zzv_qevent = tcvphy_reg (phy, option, INFO_PHYS_CC1100);

	LEDI (0, 0);
	LEDI (1, 0);
	LEDI (2, 0);

	// Things start in the off state
	RxOFF = TxOFF = 1;
	// Default power corresponds to the center == 0dBm
	xpower = 2;
	/* Initialize the device */
	ini_cc1100 ();

	/* Install the backoff timer */
	utimer (&bckf_timer, YES);
	bckf_timer = 0;

	/* Start the processes */
	if ((zzv_drvprcs = runthread (cc1100_driver)) == 0
#if (RADIO_GUARD & 0x01)
		|| runthread (cc1100_guard) == 0
#endif
								)
		syserror (ERESOURCE, "phys_cc1100");
}

static int option (int opt, address val) {
/*
 * Option processing
 */
	int ret = 0;

	switch (opt) {

	    case PHYSOPT_STATUS:

		if (TxOFF == 0)
			ret = 2;
		if ((TxOFF & 1) == 0) {
			// On or draining
			if (tcvphy_top (physid) != NULL || cc1100_status () ==
			    CC1100_STATE_TX)
				ret |= 4;
		}
		if (RxOFF == 0)
			ret |= 1;

		goto RVal;

	    case PHYSOPT_TXON:

		if (RxOFF) {
			// Start up
			chip_reset ();
			// Even if RX is off, this is our default mode
			enter_rx ();
			// Flag == need to power down on TxOFF
			RxOFF = 1;
			LEDI (0, 1);
		} else {
			LEDI (0, 2);
		}
		TxOFF = 0;
		trigger (zzv_qevent);
		break;

	    case PHYSOPT_RXON:

		if ((TxOFF & 1)) {
			// Start up
			chip_reset ();
			enter_rx ();
		}

		RxOFF = 0;

		if (TxOFF)
			LEDI (0, 1);
		else
			LEDI (0, 2);
		trigger (zzv_qevent);
		break;

	    case PHYSOPT_TXOFF:

		/* Drain */
		TxOFF = 2;
		if (RxOFF)
			LEDI (0, 0);
		else
			LEDI (0, 1);
		trigger (zzv_qevent);
		break;

	    case PHYSOPT_TXHOLD:

		TxOFF = 1;
		if (RxOFF)
			LEDI (0, 0);
		else
			LEDI (0, 1);
		trigger (zzv_qevent);
		break;

	    case PHYSOPT_RXOFF:

		RxOFF = 1;
		if (TxOFF)
			LEDI (0, 0);
		else
			LEDI (0, 1);
		trigger (zzv_qevent);
		break;

	    case PHYSOPT_CAV:

		/* Force an explicit backoff */
		if (val == NULL)
			// Random backoff
			gbackoff;
		else
			bckf_timer = *val;
		trigger (zzv_qevent);
		break;

	    case PHYSOPT_SETPOWER:

		if (val == NULL)
			// Default
			xpower = 2;
		else if (*val > 7)
			xpower = 7;
		else
			xpower = *val;
		cc1100_set_power ();

		break;

	    case PHYSOPT_GETPOWER:

		ret = (int) xpower;
		goto RVal;

	    case PHYSOPT_SETSID:

		statid = (val == NULL) ? 0 : *val;
		break;

            case PHYSOPT_GETSID:

		ret = (int) statid;
		goto RVal;

	    case PHYSOPT_SETCHANNEL:

		if (val == NULL)
			// Default, immediate
			ret = cc1100_setchannel (0);
		else
			ret = cc1100_setchannel ((*val) & 0xff);
		break;

	    case PHYSOPT_GETCHANNEL:

		ret = channr;
		goto RVal;

	    case PHYSOPT_SETRATE:

		vrate = (val == NULL) ? RADIO_BITRATE_INDEX :
			(*val >= CC1100_NRATES ? CC1100_NRATES - 1 : *val);

		// Fall through: SETRATE requires RESET

	    case PHYSOPT_RESET:

		chip_reset ();

		// Check if should bring it up

		if (RxOFF == 0 || (TxOFF & 1) == 0) {
			// Bring it up
			enter_rx ();
		} else {
			power_down ();
		}

		break;

	    case PHYSOPT_GETRATE:

		ret = (int) vrate;
		goto RVal;

	    case PHYSOPT_SETPARAM:

		ret = cc1100_setparam ((byte*)val);
		break;

	    case PHYSOPT_GETMAXPL:

		ret = rbuffl
#if RADIO_CRC_MODE > 1
			- 2
#endif
				;
		break;

#if RADIO_TRACK_ERRORS
	    case PHYSOPT_ERROR:

		if (val != NULL)
			memcpy (val, rerror, sizeof (rerror));
		return (int) rerror [0] - rerror [1] - rerror [2] - rerror [3];
#endif
	    default:

		syserror (EREQPAR, "phys_cc1100 option");

	}
	return ret;
RVal:
	if (val != NULL)
		*val = ret;

	return ret;
}
