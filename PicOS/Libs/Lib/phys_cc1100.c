/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// This one is intepreted in cc1100.h and indicates that this inclusion of the
// file should declare the static arrays with settings
#define	CC1100_DEFINE_RF_SETTINGS	1

#include "kernel.h"
#include "tcvphys.h"
#include "cc1100.h"

static int option (int, address);
static void chip_reset();

		// Pointer to the static reception buffer
static word	*rbuff = NULL,
		// How many ticks (msecs) to delay before transmission (this is
		// a utimer)
		bckf_timer = 0,
		// PHY ID
		physid,
		// Network ID
		statid = 0;

#if (RADIO_OPTIONS & 0x40)
static byte	*supplements = NULL;	// Register definition supplements
#endif

#if (RADIO_OPTIONS & 0x10)
static byte	gwch;			// Guard watch flags
#endif

#if RADIO_LBT_MODE == 3
// We need a retry counter
static byte	retr = 0;
#endif

#if (RADIO_OPTIONS & 0x04)
// Counts events ==============================================================
//
//	0 - receptions (all events when receiver was awakened by an incoming
//	    packet, i.e., the start vector [SYSTEM_IDENT] was recognized)
//	1 - successfully received packets; note that with hardware CRC +
//	    AUTOFLUSH, 0 and 1 will be equal
//	2 - the number of packets extracted from the transmit queue (ones the
//	    driver ever tried to transmit)
//	3 - the number of xmt packets dropped on retry LBT_RETR_LIMIT
//	4 - LBT/congestion indicator: EMA d(n) = 0.75 * d(n-1) + 0.25 * b, where
//	    b is the backoff experienced when trying to access the channel for
//	    TX; this is counted over all attempts to access the channel and
//	    maxed at 0x0fff, i.e., 4095
//	5 - the maximum accumulated backoff time suffered by a single packet so
//	    far
#define	RERR_RCPA	0
#define	RERR_RCPS	1
#define	RERR_XMEX	2
#define	RERR_XMDR	3
#define	RERR_CONG	4
#define	RERR_MAXB	5
#define	RERR_CURB	6

static word	rerror [RERR_CURB + 1];

// The last entry is auxiliary
#define	RERR_SIZE	(sizeof (word) * RERR_CURB)

static void set_congestion_indicator (word v) {

	// EMA of waiting time
	if ((rerror [RERR_CONG] = (rerror [RERR_CONG] * 3 + v) >> 2) > 0x0fff)
		// Keeping the max at 0xfff will avoid overflow as the maximum
		// value of increment is 255
		rerror [RERR_CONG] = 0xfff;

	if (v) {
		// Accumulate in 4
		if (rerror [RERR_CURB] + v < rerror [RERR_CURB])
			// Overflow
			rerror [RERR_CURB] = 0xffff;
		else
			rerror [RERR_CURB] += v;
	} else {
		// Update max
		if (rerror [RERR_MAXB] < rerror [RERR_CURB])
			rerror [RERR_MAXB] = rerror [RERR_CURB];
		rerror [RERR_CURB] = 0;
	}
}

#else
#define	set_congestion_indicator(v)	CNOP
#endif	/* RADIO_OPTIONS & 0x04 */

// ============================================================================

word		__pi_v_drvprcs, __pi_v_qevent;

static byte	RxST,			// Receiver status
		xpower,			// Power select
		rbuffl,
		vrate = RADIO_BITRATE_INDEX,	// Rate select
		channr = RADIO_DEFAULT_CHANNEL;

#if (RADIO_OPTIONS & 0x10)
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

	// Wait for ready
	while (!(RF1AIFCTL1 & RFINSTRIFG));

	// Address + instruction
	RF1AINSTRB = (addr | RF_REGWR);
	RF1ADINB = val;
}

static byte cc1100_get_reg (byte addr) {

	byte val;

	RF1AINSTR1B = (addr | 0x80); 
	// A simple return should do
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

static void set_reg_group (const byte *grp) {

	for ( ; *grp != 255; grp += 2) {
		cc1100_set_reg (grp [0], grp [1]);
	}
}

static void init_cc_regs () {

	// General
	set_reg_group (cc1100_rfsettings);

	// Rate-specific
	set_reg_group (cc1100_ratemenu [vrate]);

	// PATABLE is handled separately
	cc1100_set_reg_burst (CCxxx0_PATABLE, patable, sizeof (patable));

#if (RADIO_OPTIONS & 0x40)
	// Supplements
	if (supplements != NULL)
		set_reg_group (supplements);
#endif
	// Power and channel number are also separate
	cc1100_set_power ();
	cc1100_setchannel (channr);
}

static byte cc1100_status () {

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
		mdelay (1);
	}
#if (RADIO_OPTIONS & 0x01)
	diag ("CC1100: %u ST HNG!!", (word) seconds ());
#endif
	chip_reset ();
	goto ReTry;
}

static void enter_idle () {

	int i;

#if (RADIO_OPTIONS & 0x20)
// ============================================================================

ReTry:
	i = 32;

	while (cc1100_status () != CC1100_STATE_IDLE) {
		i--;
		cc1100_strobe (CCxxx0_SIDLE);
		if (i < 16) {
			if (i == 0) {
#if (RADIO_OPTIONS & 0x01)
				diag ("CC1100: %u ID HNG!!",
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

#if (RADIO_OPTIONS & 0x20)
// ============================================================================
ReTry:
	i = 32;
	while (1) {
		cc1100_strobe (CCxxx0_SRX);
		if (cc1100_status () == CC1100_STATE_RX)
			return;
		i--;
		if (i < 16) {
			if (i == 0) {
#if (RADIO_OPTIONS & 0x01)
				diag ("CC1100: %u RX HNG!!",
					(word) seconds ());
#endif
				chip_reset ();
				goto ReTry;
			}
		}
		udelay (100);
	}
#else
// ============================================================================
	for (i = 0; i < 16; i++) {
		cc1100_strobe (CCxxx0_SRX);
		if (cc1100_status () == CC1100_STATE_RX)
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

#if (RADIO_OPTIONS & 0x01)
	diag ("CC1100: CANT RX!!");
#endif	/* 0x01 */

#endif	/* 0x20 */
// ============================================================================
}

static int cc1100_rx_status () {

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
#if (RADIO_OPTIONS & 0x01)
	diag ("CC1100: %u RXST = %x/%x!!", (word) seconds (), val, b);
#endif

#endif
	return -1;
}

void cc1100_rx_flush () {

	int i;

#if (RADIO_OPTIONS & 0x20)
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
#if (RADIO_OPTIONS & 0x01)
				diag ("CC1100: %u FL HNG!!", (word) seconds ());
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

static void cc1100_rx_reset () {

	chip_reset ();
	enter_rx ();
}

static word clear_to_send () {

	// Make sure our status is sane (FIXME: try removing this)
	// cc1100_status ();
	cc1100_strobe (CCxxx0_STX);
	// We succeed if we have entered TX
	return (cc1100_status () == CC1100_STATE_TX);
}

static void power_down () {

#if (RADIO_OPTIONS & 0x02)
	diag ("CC1100: %u POWER DOWN", (word) seconds ());
#endif
	enter_idle ();
	cc1100_strobe (CCxxx0_SPWD);
#ifndef	__CC430__
	// We do it twice to make sure; kind of stupid, but I do not absolutely
	// trust the SPI interface. No need to be paranoid on the CC430 where
	// the commands are simply written to a register.
	cc1100_strobe (CCxxx0_SPWD);
#endif
}

static void chip_reset () {

	full_reset;
	init_cc_regs ();
#if (RADIO_OPTIONS & 0x02)
	diag ("CC1100: %u CHIP RESET", (word) seconds ());
#endif
}

static void ini_cc1100 () {

	// Initialize the requisite pins
	ini_regs;
	chip_reset ();

// ============================================================================
#if (RADIO_OPTIONS & 0x80)
#if ENTROPY_COLLECTION

	{	word i;

		for (i = 0; i < 8; i++) {
			enter_rx ();
			// The worst case settling time for RSSI after RX is
			// ca. 750 us
			mdelay (1);
			entropy = (entropy << 4) |
				(cc1100_get_reg (CCxxx0_RSSI) & 0xF);
			enter_idle ();
		}
#if (RADIO_OPTIONS & 0x02)
		diag ("CC1100 ENTR: %x%x", (word)entropy, (word)(entropy>>16));
#endif
	}
#endif
#endif
// ============================================================================
	power_down ();

	// Read the chip number reg and write a message to the UART

#if DIAG_MESSAGES
	diag ("CC1100: %d, %d.%dMHz, %d/%dkHz=%d.%dMHz", vrate,
		CC1100_BFREQ, CC1100_BFREQ_10, RADIO_DEFAULT_CHANNEL,
			CC1100_CHANSPC_T1000, CC1100_DFREQ, CC1100_DFREQ_10);
#endif
	dbg_1 (0x2000); // CC1100 initialized
	dbg_1 ((cc1100_get_reg (CCxxx0_SYNC1) << 8) |
			cc1100_get_reg (CCxxx0_SYNC0));
	dbg_1 ((cc1100_get_reg (CCxxx0_VERSION) << 8) |
			cc1100_get_reg (CCxxx0_MCSM1));
}

#if SOFTWARE_CRC
#include "checksum.h"
#endif

static void do_rx_fifo () {

	int len, paylen;
	byte *eptr;

#if SOFTWARE_CRC == 0
	byte b;
#endif
	LEDI (2, 1);
	// We are making progress as far as reception
	guard_stop (WATCH_RCV | WATCH_PRG);

	if (RxST) {
		// If we are switched off, just clean the FIFO and return
#if (RADIO_OPTIONS & 0x02)
		diag ("CC1100: %u RX OFF CLEANUP", (word) seconds ());
#endif
		cc1100_rx_flush ();
		goto Rtn;
	}

	if ((len = cc1100_rx_status ()) < 0) {
		// Error: normally FIFO overrun (shouldn't happen)
#if (RADIO_OPTIONS & 0x01)
		diag ("CC1100: %u RX BAD STAT!!", (word) seconds ());
#endif
ResNX:
		cc1100_rx_reset ();
		// Skip reception
		goto Rtn;
	}

#if (RADIO_OPTIONS & 0x04)
	if (rerror [RERR_RCPA] == MAX_WORD)
		// Zero out slots 0 and 1
		memset (rerror + RERR_RCPA, 0, sizeof (word) * 2);
	rerror [RERR_RCPA] ++;
#endif
	// len is the physical payload, i.e., the number of bytes in the pipe

#if SOFTWARE_CRC == 0
	// There is no software CRC at the end, so the minimum includes:
	// paylen + 2 bytes of SID + 2 status bytes
	if ((len & 1) == 0 || len < 5) {
#else
	// There is software CRC at the end, two bytes extra
	if ((len & 1) == 0 || len < 7) {
#endif
		// Physical payload length must be even, so the extra byte
		// for payload length will make it odd

#if (RADIO_OPTIONS & 0x01)
		diag ("CC1100: %u RX BAD PL: %d", (word) seconds (), len);
#endif
		goto ResNX;
	}

	// This is the logical payload length, i.e., the first byte of whatever
	// has arrived; note that it covers software checksum, if SOFTWARE_CRC
	paylen = cc1100_get_reg (CCxxx0_RXFIFO);

	// Note that paylen must equal len - 3, regardless of SOFTWARE_CRC,
	// because of the two status bytes appended at the end by the chip
	if (paylen != len - 3) {

#if (RADIO_OPTIONS & 0x01)
		diag ("CC1100: %u RX PL MIS: %d/%d", (word) seconds (), len,
			paylen);
#endif
		goto ResNX;
	}

	// We shall copy --len bytes, i.e., paylen + 2 (or, put differently,
	// whatever is present in the FIFO), because we need the status bytes
	// as well
	if (--len > rbuffl) {
#if (RADIO_OPTIONS & 0x01)
		diag ("CC1100: %u RX LONG: %d", (word) seconds (), paylen);
#endif
		goto ResNX;
	}

	// Copy the FIFO contents
	cc1100_get_reg_burst (CCxxx0_RXFIFO, (byte*)rbuff, (byte) len);

	// We have emptied the FIFO, so we can start RCV for another one
	enter_rx ();

	if (statid != 0 && statid != 0xffff) {
		// Admit only packets with agreeable statid
		if (rbuff [0] != 0 && rbuff [0] != statid) {
			// Drop
#if (RADIO_OPTIONS & 0x01)
			diag ("CC1100: %u RX BAD STID: %x", (word) seconds (),
				rbuff [0]);
#endif
			goto Rtn;
		}
	}

#if SOFTWARE_CRC
	// Verify software CRC (at this point len = paylen + 2)
	len = paylen >> 1;
	if (w_chk (rbuff, len, 0)) {
		// Bad checksum
#if (RADIO_OPTIONS & 0x01)
		diag ("CC1100: %u RX CKS (S) %x %x %x", (word) seconds (),
			(word*)(rbuff) [0],
			(word*)(rbuff) [1],
			(word*)(rbuff) [2]);
#endif
		// Ignore
		goto Rtn;
	}

	// Move the extra two bytes from after paylen over CRC (tweaking them
	// properly)
	eptr = (byte*)rbuff + paylen;
	((byte*)rbuff) [paylen - 2] = (*(eptr + 1) & 0x7f);
	// This number is signed and starts negative, so we have
	// to bias it properly
	((byte*)rbuff) [paylen - 1] = *((char*)eptr) + 128;
	add_entropy (rbuff [len - 1]);

#else	/* SOFTWARE_CRC (now for the hardware option) */

	// Status bytes (now in place)
	eptr = (byte*)rbuff + paylen;
	b = *(eptr+1);
	add_entropy (rbuff [len]);
	// Add the two status byte to the payload
	paylen += 2;

#if AUTOFLUSH_FLAG
	// No need to verify the checksum
	*(eptr+1) = *((char*)eptr) + 128;
	*eptr = (b & 0x7f);
#else
	if (b & 0x80) {
		// CRC OK
		*(eptr+1) = *((char*)eptr) + 128;
		*eptr = (b & 0x7f);
	} else {
		// Bad checksum
#if (RADIO_OPTIONS & 0x01)
		diag ("CC1100: %u RX CKS (H) %x %x %x", (word) seconds (),
			(word*)(rbuff) [0],
			(word*)(rbuff) [1],
			(word*)(rbuff) [2]);
#endif
		// Ignore
		goto Rtn;
	}
#endif	/* AUTOFLUSH_FLAG */
#endif	/* SOFTWARE_CRC */

#if (RADIO_OPTIONS & 0x04)
	rerror [RERR_RCPS] ++;
#endif

#if (RADIO_OPTIONS & 0x02)
	diag ("CC1100: %u RX OK %x %x %x", (word) seconds (),
		(word*)(rbuff) [0],
		(word*)(rbuff) [1],
		(word*)(rbuff) [2]);
#endif
	tcvphy_rcv (physid, rbuff, paylen);
Rtn:
	gbackoff (RADIO_LBT_BACKOFF_RX);
	LEDI (2, 0);
}

#define	DR_LOOP		0
#define	DR_SWAIT	1

thread (cc1100_driver)

  address xbuff;
  int paylen;
#if SOFTWARE_CRC
  int len;
#endif

  entry (DR_LOOP)

	if (RxST != 2) {
		// The receiver is formally off, but still running, so let us
		// clear the FIFO
		while (RX_FIFO_READY)
			do_rx_fifo ();
	}

	if (tcvphy_top (physid) == NULL) {
		if (RxST == 1) {
			// Nothing to transmit, receiver off -> power down
			power_down ();
			// Flag == powered down
			RxST = 2;
		}
WEvent:
		wait (__pi_v_qevent, DR_LOOP);
		if (RxST == 0)
			rcv_enable_int;
		release;
	}

	if (bckf_timer) {
		delay (bckf_timer, DR_LOOP);
		goto WEvent;
	}

	// We've got something to transmit
	if (RxST == 2) {
		// We are powered down
		cc1100_rx_reset ();
		RxST = 1;
	}

#if RADIO_LBT_MODE == 3
	// Staged/limited
	if (retr < LBT_RETR_FORCE_RCV) {
		// Retry count below forcing threshold
		if (retr == 0) {
			// Initialize MCSM1
			cc1100_set_reg (CCxxx0_MCSM1, MCSM1_LBT_FULL);
#if (RADIO_OPTIONS & 0x04)
			if (rerror [RERR_XMEX] == MAX_WORD)
				memset (rerror + RERR_XMEX, 0,
					sizeof (word) * 2);
			else
				rerror [RERR_XMEX] ++;
#endif
		}
		cc1100_set_reg (CCxxx0_AGCCTRL1, cc1100_agcctrl_table [retr]);
	} else {
		// Forcing
		cc1100_set_reg (CCxxx0_MCSM1,
			retr < LBT_RETR_FORCE_OFF ?
				// Still honor own receptions in pogress
				MCSM1_LBT_RCV :
				// Complete OFF
				MCSM1_LBT_OFF);
	}
#endif

	if (clear_to_send () == NO) {

#if RADIO_LBT_MODE == 3
		if (retr == LBT_RETR_LIMIT-1) {
			// Limit reached, drop the packet
#if (RADIO_OPTIONS & 0x04)
			// This one is never bigger than RERR_XMEX
			rerror [RERR_XMDR] ++;
#endif
			// Pretend the packet has been transmitted
			set_congestion_indicator (0);
#if (RADIO_OPTIONS & 0x01)
			diag ("CC1100: RTL!!");
#endif
			goto FEXmit;
		}
		retr++;
#endif

#if ((RADIO_OPTIONS & 0x05) == 0x05)
		if (rerror [RERR_CONG] >= 0x0fff)
				diag ("CC1100: LBT CNG");
#endif

#if RADIO_LBT_BACKOFF_EXP == 0
		// Aggressive transmitter
		delay (1, DR_LOOP);
		set_congestion_indicator (1);
		release;
#else
		// Backoff
		gbackoff (RADIO_LBT_BACKOFF_EXP);
		set_congestion_indicator (bckf_timer);
		proceed (DR_LOOP);
#endif
	} else {
		set_congestion_indicator (0);
	}

	// This cannot possibly fail
	xbuff = tcvphy_get (physid, &paylen);

	// A packet arriving from TCV always contains CRC slots, even if
	// we are into hardware CRC; its minimum legit length is SID + CRC

#if SOFTWARE_CRC
	// In this case, rbuff is 2 bytes larger than the largest transmittable
	// packet, as (on top of CRC) it must accommodate two extra (status)
	// bytes
	sysassert (paylen <  rbuffl && paylen >= 4 && (paylen & 1) == 0,
		"cc1100 xmt pktl");
#else
	// Here, there is no software CRC, so the two status bytes take the
	// place of CRC (which is not used)
	sysassert (paylen <= rbuffl && paylen >= 4 && (paylen & 1) == 0,
		"cc1100 xmt pktl");
#endif
	LEDI (1, 1);

	if (statid != 0xffff)
		// This means "honor the packet's statid"
		xbuff [0] = statid;

#if SOFTWARE_CRC
	// Calculate CRC
	len = (paylen >> 1) - 1;
	((word*)xbuff) [len] = w_chk ((word*)xbuff, len, 0);
#else
	paylen -= 2;		// Ignore the CRC bytes
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

#if RADIO_LBT_XMIT_SPACE
	utimer_set (bckf_timer, RADIO_LBT_XMIT_SPACE);
#endif

FEXmit:

#if RADIO_LBT_MODE == 3
	retr = 0;
#endif
	proceed (DR_LOOP);

endthread

#if (RADIO_OPTIONS & 0x10)
// ============================================================================

#define	GU_ACTION	0

thread (cc1100_guard)

  word stat;

  entry (GU_ACTION)

#if (RADIO_OPTIONS & 0x02)
	diag ("CC1100: %u GRD ...", (word) seconds ());
#endif
	if (guard_hung) {
#if (RADIO_OPTIONS & 0x01)
		diag ("CC1100: %u GRD RST: %x!!", (word) seconds (), gwch);
#endif
Reset:
		guard_clear;
		cc1100_rx_reset ();
		p_trigger (__pi_v_drvprcs, __pi_v_qevent);
		delay (GUARD_LONG_DELAY, GU_ACTION);
		release;
	}

	if (RxST == 2) {
		// The chip is powered down, so don't bother
		delay (GUARD_LONG_DELAY, GU_ACTION);
		release;
	}

	if (RX_FIFO_READY) {
		// This one should go away eventually
		guard_start (WATCH_RCV);
		delay (GUARD_SHORT_DELAY, GU_ACTION);
		// Won't hurt
		p_trigger (__pi_v_drvprcs, __pi_v_qevent);
		release;
	}

	stat = cc1100_status ();

	if (stat == CC1100_STATE_TX) {
		// This one will go away eventually as well
		guard_start (WATCH_XMT);
		delay (GUARD_SHORT_DELAY, GU_ACTION);
		p_trigger (__pi_v_drvprcs, __pi_v_qevent);
		release;
	}

	if (stat != CC1100_STATE_RX) {
		// Something is wrong: note that stat == IDLE implies
		// RX_FIFO_READY
#if (RADIO_OPTIONS & 0x01)
		diag ("CC1100: %u GRD BAD ST: %d!!",
			(word) seconds (), stat);
#endif
		goto Reset;
	}

	// Do not break reception in progress
	stat = cc1100_get_reg (CCxxx0_RXBYTES);

	if (stat & 0x80) {
		// Overflow
#if (RADIO_OPTIONS & 0x01)
		diag ("CC1100: %u GRD FIFO!!", (word) seconds ());
#endif
		goto Reset;
	}

	if (stat == 0) {
		// Recalibrate
#if (RADIO_OPTIONS & 0x02)
		diag ("CC1100: %u GRD RECAL", (word) (word) seconds ());
#endif
		enter_idle ();
		enter_rx ();
		// Will reset the chip periodically on LONG_DELAY if nothing
		// happens in between
		guard_start (WATCH_PRG);
		delay (GUARD_LONG_DELAY, GU_ACTION);
	} else {
		guard_start (WATCH_RCV);
		delay (GUARD_SHORT_DELAY, GU_ACTION);
	}
	p_trigger (__pi_v_drvprcs, __pi_v_qevent);

endthread

#endif	/* GUARD process present */

void phys_cc1100 (int phy, int mbs) {
//
// Note that mbs DOES COVER the checksum. Put this into your stupid head once
// for all (just a strong "mental" note to myself). 
//
// A packet sent over the radio starts with a length byte (called the payload
// length) and its full length is in fact odd. What follows the length byte is
// the TCV packet to be actually received (the payload). The maximum payload
// length is 60 bytes, because the FIFO size is 64 bytes, and one of them is
// used for the payload length byte, and two status bytes are appended at the
// end upon reception. Those status bytes have nothing to do with the CRC:
// they are the Link Quality and RSSI bytes calculated by the chip.
// 
// When we operate with software checksum (SOFTWARE_CRC), the checksum
// calculated by the driver arrives as the last two bytes of the payload. In
// this mode, the "useful" packet length is limited to 58 bytes. The maximum
// legit value for mbs is then 60.
//
// With hardware checksum (SOFTWARE_CRC == 0), the useful packet length is
// limited to 60 bytes (because there is no need for software checksum to
// be received). The PHY will accept 62 as the value of mbs.
//
// Note that, regardless of the value of SOFTWARE_CRC, the driver appends at
// the end of the useful payload two bytes (RSSI and link quality) for which it
// expects room in the buffer. Thus, in both cases, the amount of memory
// reserved for the reception buffer is exactly mbs, even though its useful
// length is mbs - 2.
//
	if (rbuff != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "cc1100");

	if (mbs < 6 || mbs > CC1100_MAXPLEN) {
		if (mbs == 0)
			mbs = CC1100_MAXPLEN;
		else
			syserror (EREQPAR, "cc1100 mbs");
	}

#ifndef	__CC430__
#if 	(RADIO_OPTIONS & 0x08)
	csn_down;
	mdelay (2);
	if (so_val) {
		// This precludes hangups that would have been caused by stuck
		// so_val on our first attempt to talk to the chip
		syserror (EHARDWARE, "cc1100 chip");
	}
	// csn_up
	SPI_END;
#endif
#endif

	rbuffl = (byte) mbs;	// buffer length in bytes, including checksum
#if SOFTWARE_CRC
	rbuffl += 2;		// add room for two status bytes
#endif
	if ((rbuff = umalloc (rbuffl)) == NULL)
		syserror (EMALLOC, "cc1100");

	statid = 0;
	physid = phy;

	/* Register the phy */
	__pi_v_qevent = tcvphy_reg (phy, option, INFO_PHYS_CC1100);

	LEDI (0, 0);
	LEDI (1, 0);
	LEDI (2, 0);

	// Things start in the off state (ini_cc1100 powers down the chip)
	RxST = 2;
	xpower = RADIO_DEFAULT_POWER;
	/* Initialize the device */
	ini_cc1100 ();

	/* Install the backoff timer */
	utimer_add (&bckf_timer);
	utimer_set (bckf_timer, 0);

	/* Start the processes */
	if ((__pi_v_drvprcs = runthread (cc1100_driver)) == 0
#if (RADIO_OPTIONS & 0x10)
		|| runthread (cc1100_guard) == 0
#endif
								)
		syserror (ERESOURCE, "cc1100");
}

static int option (int opt, address val) {
/*
 * Option processing
 */
	int ret = 0;

	switch (opt) {

	    case PHYSOPT_STATUS:

		ret = 2 | (RxST == 0);
		goto RVal;

	    case PHYSOPT_RXON:

		if (RxST == 2)
			// We have been switched off
			cc1100_rx_reset ();

		RxST = 0;
		LEDI (0, 1);
OREvnt:
		p_trigger (__pi_v_drvprcs, __pi_v_qevent);

	    case PHYSOPT_TXON:
	    case PHYSOPT_TXOFF:
	    case PHYSOPT_TXHOLD:

		break;

	    case PHYSOPT_RXOFF:

		if (RxST == 0)
			RxST = 1;

		LEDI (0, 0);
		goto OREvnt;

	    case PHYSOPT_CAV:

		/* Force an explicit backoff */
		if (val == NULL)
			// Random backoff
			gbackoff (RADIO_LBT_BACKOFF_EXP);
		else
			utimer_set (bckf_timer, *val);
		goto OREvnt;

	    case PHYSOPT_SETPOWER:

		if (val == NULL)
			// Default
			xpower = RADIO_DEFAULT_POWER;
		else if (*val > 7)
			xpower = 7;
		else
			xpower = *val;

		// This will fail if the chip is switched off, but so what?
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

#if (RADIO_OPTIONS & 0x40)
		supplements = (byte*) val;
#endif
		chip_reset ();

		// Check if should bring it up
		if (RxST == 2)
			power_down ();
		else
			enter_rx ();
		break;

	    case PHYSOPT_GETRATE:

		ret = (int) vrate;
		goto RVal;

	    case PHYSOPT_GETMAXPL:

		ret = rbuffl
#if SOFTWARE_CRC
			- 2
#endif
				;
		break;

#if (RADIO_OPTIONS & 0x04)
	    case PHYSOPT_ERROR:

		if (val != NULL) {
			if (rerror [RERR_CURB] > rerror [RERR_MAXB])
				rerror [RERR_MAXB] = rerror [RERR_CURB];
			memcpy (val, rerror, RERR_SIZE);
		} else
			memset (rerror, 0, sizeof (rerror));

		break;
#endif
	    default:

		syserror (EREQPAR, "cc1100 opt");

	}
	return ret;
RVal:
	if (val != NULL)
		*val = ret;

	return ret;
}
