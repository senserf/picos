/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2015                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// This one is interpreted in cc1100.h and indicates that this inclusion of the
// file should declare the static arrays with settings
#define	CC1100_DEFINE_RF_SETTINGS	1

#include "kernel.h"
#include "tcvphys.h"
#include "cc1100.h"

// ============================================================================
// Chip access functions ======================================================
// ============================================================================

#ifdef	__CC430__

// Internal (CC430)

#if RADIO_WOR_MODE
byte		cc1100_worstate;	// WOR interrupt machine state
#endif

static void set_reg (byte addr, byte val) {

	// Wait for ready
	while (!(RF1AIFCTL1 & RFINSTRIFG));

	// Address + instruction
	RF1AINSTRB = (addr | RF_REGWR);
	RF1ADINB = val;
}

static byte get_reg (byte addr) {

	byte val;

	RF1AINSTR1B = (addr | 0x80); 
	// CHECKME: will a simple return do?
  	while (!(RF1AIFCTL1 & RFDOUTIFG));
	val = RF1ADOUT1B;
	return val;
}

static void set_reg_burst (byte addr, byte *buffer, word count) {
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

static void get_reg_burst (byte addr, byte *buffer, word count) {

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

#if RADIO_WOR_MODE == 0
// Must be visible from the interrupt service routine
static
#endif

byte cc1100_strobe (byte b) {
//
// This one must be global for WOR
//
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

	if (b != CCxxx0_SPWD
#if RADIO_WOR_MODE
	    // This strobe is never used if WOR_MODE is 0
	    && b != CCxxx0_SWOR
#endif
				) {
		// We worry about the chip being ready
		if (sb) {
			// Transiting from SLEEP or WOR
			while (chip_not_ready);
			udelay (DELAY_CHIP_READY);
		}
	}

	while ((RF1AIFCTL1 & RFSTATIFG) == 0);
Ret:
	sb = RF1ASTATB;
	return (sb & CC1100_STATE_MASK);
}

static int rx_status () {

	int res;

	if (strobe (CCxxx0_SNOP) == CC1100_STATE_IDLE) {
		// The state is right, check RXBYTES
		if (!((res = get_reg (CCxxx0_RXBYTES)) & 0x80))
			// Treat FIFO overflow as forced garbage, even if,
			// theoretically, it can mean two packets glued
			// together, with the first one receivable. Note that
			// a nasty overflow case can result in garbage with the
			// length byte looking accidentally right (P = 1/256)
			// and the CRC OK bit of the overflow status byte
			// right, too (P = 1/2). Thus a long garbage packet
			// (with correct SYNC/NID) might be receivable with
			// P = 1/512 (or so).
			return res;
	}

	return -1;
}

#else

// SPI-based

static byte spi_out (byte b) {

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

static byte spi_in () {

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

static void set_reg (byte addr, byte val) {

	CC1100_SPI_START;
	CC1100_SPI_WAIT;
	spi_out (addr);
	spi_out (val);
	CC1100_SPI_END;
}

static byte get_reg (byte addr) {

	register byte val;

	CC1100_SPI_START;
	spi_out (addr | 0x80);
	val = spi_in ();
	CC1100_SPI_END;
	return val;
}

static void set_reg_burst (byte addr, byte *buffer, word count) {

	CC1100_SPI_START;
	spi_out (addr | 0x40);
	while (count--)
		spi_out (*buffer++);
	CC1100_SPI_END;
}

static void get_reg_burst (byte addr, byte *buffer, word count) {

	CC1100_SPI_START;
	spi_out (addr | 0xC0);
	while (count--)
		*buffer++ = spi_in ();
	CC1100_SPI_END;
}
	
static byte strobe (byte cmd) {

	register byte res;

	CC1100_SPI_START;
	res = spi_out (cmd);
	CC1100_SPI_END;
	return res;
}

static int rx_status () {

	byte b, val, c;

	c = 255;
	while (1) {
		CC1100_SPI_START;
		// Owing to the bug mentioned in the errata, we read RXBYTES
		// twice
		val = spi_out (CCxxx0_RXBYTES);
		// Get RXBYTES
		b = spi_in ();
		CC1100_SPI_END;
		if (b == c)
			return ((val == CC1100_STATE_IDLE ||
				 val == CC1100_STATE_RX) && b <= 64) ?
				// The status is right, return #bytes in RX
				// FIFO; note that we also admit RX status
				// which may result from two packets received
				// back to back
					b : -1;
		c = b;
	}
}

#endif

// ============================================================================
// End chip access functions ==================================================
// ============================================================================

static byte	rbuffl,		// Input buffer length, basically a constant
		RxST;		// Logical state of the receiver

#if RADIO_LBT_MODE == 3
static byte	retr = 0;	// Retry counter for staged LBT
#endif

#if RADIO_POWER_SETTABLE
static byte	power = RADIO_DEFAULT_POWER;
#endif

#if RADIO_CHANNEL_SETTABLE
static byte	channel = RADIO_DEFAULT_CHANNEL;
#endif

#if RADIO_BITRATE_SETTABLE
static byte	vrate = 0;	// Rate index
#endif

#if RADIO_WOR_MODE
static byte	woron = 0;				// WOR "on" flag
static word	wor_idle_timeout = RADIO_WOR_IDLE_TIMEOUT,
		wor_preamble_time = RADIO_WOR_PREAMBLE_TIME;
#endif

static const byte patable [] = CC1100_PATABLE;

//
// RxST states
//
#define	RCV_STATE_PD	2	// Power down or WOR
#define	RCV_STATE_OFF	1
#define RCV_STATE_ON	0

static word	*rbuff = NULL,
		physid,
		statid = 0,
		bckf_timer = 0;

#if (RADIO_OPTIONS & RADIO_OPTION_RBKF)
static word	bckf_lbt = 0;

#define	update_bckf_lbt(v)	bckf_lbt += (v)

#else

#define	update_bckf_lbt(v)	CNOP

#endif

word		__pi_v_drvprcs, __pi_v_qevent;

// ============================================================================
#if (RADIO_OPTIONS & RADIO_OPTION_STATS)
// ============================================================================
// Count events ===============================================================
// ============================================================================
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
//
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
#endif	/* RADIO_OPTIONS & RADIO_OPTION_STATS */

// ============================================================================

static byte read_status () {

	byte val;
	word i;

	for (i = 0; i < 32; i++) {

		val = strobe (CCxxx0_SNOP);

		switch (val) {

			// Clean up overflow/underflow states
			case CC1100_STATE_TX_UNDERFLOW:
				strobe (CCxxx0_SFTX);
				break;
			case CC1100_STATE_RX_OVERFLOW:
				strobe (CCxxx0_SFRX);
				break;

			// Loop on these until they go away
			case CC1100_STATE_CALIBRATE:
			case CC1100_STATE_SETTLING:
				udelay (DELAY_SETTLING_STATE);
				break;

			default:
				return val;
		}
	}

	syserror (EHARDWARE, "cc11 sh");
}

// ============================================================================

static void enter_idle () {

	word i;

	for (i = 0; i < 15; i++) {
		strobe (CCxxx0_SIDLE);
		if (read_status () == CC1100_STATE_IDLE)
			return;
	}

	syserror (EHARDWARE, "cc11 ei");
}

// ============================================================================

static void enter_rx () {

	word i;

	for (i = 0; i < 15; i++) {
		strobe (CCxxx0_SRX);
		if (read_status () == CC1100_STATE_RX)
			return;
		udelay (DELAY_SRX_FAILURE);
	}

	syserror (EHARDWARE, "cc11 er");
}

static void set_reg_group (const byte *r, const byte *v, word n) {

	do { --n; set_reg (r [n], v [n]); } while (n);
}

// ============================================================================

static void power_down () {

#if defined(__CC430__) && RADIO_WOR_MODE
	// Switch off the WOR interrupt automaton
	cc1100_worstate = 0;
#endif
	enter_idle ();

#if RADIO_WOR_MODE
	if (woron) {
		// The procedure of entering WOR is a bit tricky, somewhat
		// trickier than one would expect based on the doc. One problem
		// is the lingering "PQT reached" signal from the last RX mode,
		// which is always on, because we don't use PQT for normal
		// reception (so the threshold is permanently "reached"). We
		// want to get rid of this signal BEFORE we switch the role of
		// GDO0 to "PQT reached" event, as otherwise the interrupt will
		// occur immediately. So we start with a dummy setting of
		// PKTCTRL1 to the highest possible PQT:
		set_reg (CCxxx0_PKTCTRL1, 0xE0);
		// ... then we enter RX to clear the PQT reached signal
		enter_rx ();
		// ... then we enter IDLE again in preparation for WOR
		enter_idle ();
		// ... then we set the registers for WOR operation
		set_reg_group (cc1100_wor_sr, cc1100_wor_von,
			sizeof (cc1100_wor_sr));
		// ... now we can enable the interrupt, which is not pending
		// any more. Note that whoever calls power_down running this
		// code (entering WOR) MUST BE already prepared to receive
		// events. A missing/ignored interrupt would hang WOR.
		wor_enable_int;
#ifdef __CC430__
		cc1100_worstate = 1;
#endif
		strobe (CCxxx0_SWOR);

#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
		diag ("CC1100: %u WOR", (word) seconds ());
#endif
	} else
#endif
	{
		strobe (CCxxx0_SPWD);
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
		diag ("CC1100: %u POWER DOWN", (word) seconds ());
#endif
	}

	RxST = RCV_STATE_PD;

#ifdef	MONITOR_PIN_CC1100_PUP
	_PVS (MONITOR_PIN_CC1100_PUP, 0);
#endif
}

// ============================================================================

#if RADIO_WOR_MODE

static void set_default_wor_params () {

	wor_preamble_time = RADIO_WOR_PREAMBLE_TIME;
	wor_idle_timeout = RADIO_WOR_IDLE_TIMEOUT;

#if (RADIO_OPTIONS & RADIO_OPTION_WORPARAMS)
	set_reg (CCxxx0_WOREVT1, WOR_EVT0_TIME >> 8);
	cc1100_wor_von [0] = CCxxx0_PKTCTRL1_WORx;
	cc1100_wor_von [1] = CCxxx0_AGCCTRL1_WORx;
	cc1100_wor_von [2] = CCxxx0_WORCTRL_WORx;
	cc1100_wor_von [3] = CCxxx0_MCSM2_WORx;
#endif

}

#endif

static void chip_reset () {
//
// Reset the module to standard register setting in power down mode
//
#if RADIO_WOR_MODE
	woron = 0;
#endif
	cc1100_full_reset;
	// Set the registers
	set_reg_burst (0x00, (byte*)cc1100_rfsettings,
		sizeof (cc1100_rfsettings));

#if RADIO_WOR_MODE
	// Reset WOR defaults
	set_default_wor_params ();
#endif
	power_down ();

#if RADIO_BITRATE_SETTABLE
	vrate = RADIO_BITRATE_INDEX;
#endif

#if RADIO_POWER_SETTABLE && (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS) == 0
	power = RADIO_DEFAULT_POWER;
#endif

#if RADIO_CHANNEL_SETTABLE
	channel = RADIO_DEFAULT_CHANNEL;
#endif

}

static void rx_reset () {
//
// This gets us back to normal, i.e., to our default state
//
	word i;

	for (i = 0; i < 16; i++) {
		strobe (CCxxx0_SFRX);
		if (rx_status () == 0) {
			enter_rx ();
			return;
		}
	}
	syserror (EHARDWARE, "cc11 rr");
}

// ============================================================================

static void power_up () {
//
// Return from sleep
//
#ifdef MONITOR_PIN_CC1100_PUP
	_PVS (MONITOR_PIN_CC1100_PUP, 1);
#endif
#if defined(__CC430__) && RADIO_WOR_MODE
	// Disable the WOR interrupt automaton in case we're returning from WOR
	cc1100_worstate = 0;
#endif
	enter_idle ();

#ifdef	__CC430__
	// This delay seems to be needed on CC430 to prevent hangups occurring
	// when transmitting with RXOFF. On my device (CHRONOS). 125 us seems
	// to work fine, while 60 is too small. Let's hope that 200 will do the
	// trick.
	udelay (DELAY_IDLE_PDOWN);
#endif

#if RADIO_WOR_MODE
	if (woron) {
		set_reg_group (cc1100_wor_sr, cc1100_wor_voff,
			sizeof (cc1100_wor_sr));
	}
#endif
	// Load PATABLE
	set_reg_burst (CCxxx0_PATABLE, (byte*) patable, sizeof (patable));
	rx_reset ();
}

// ============================================================================

#if SOFTWARE_CRC
#include "checksum.h"
#endif

// ============================================================================

static void do_rx_fifo () {

	int len, paylen;
	byte *eptr;
#if SOFTWARE_CRC == 0 || RADIO_CRC_TRANSPARENT
	byte b;
#endif
	LEDI (2, 1);

#ifdef	MONITOR_PIN_CC1100_RX
	_PVS (MONITOR_PIN_CC1100_RX, 1);
#endif

	if ((len = rx_status ()) < 0) {
		// Something wrong, bad state
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
		diag ("CC1100: %u RX BAD STAT!!", (word) seconds ());
#endif
RRX:
		// Reset to RX
		enter_idle ();
		rx_reset ();
		goto Rtn;
	}

	if (RxST) {
		// We are formally switched off (but still up, note that RxST
		// cannot be 2 at this point), so just clean the FIFO
		if (len) {
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
			diag ("CC1100: %u RX OFF CLEANUP", (word) seconds ());
#endif
			goto RRX;
		}
		goto Rtn;
	}

#if (RADIO_OPTIONS & RADIO_OPTION_STATS)
	if (rerror [RERR_RCPA] == MAX_WORD)
		// Zero out slots 0 and 1
		memset (rerror + RERR_RCPA, 0, sizeof (word) * 2);
	rerror [RERR_RCPA] ++;
#endif
	// len is the physical payload length, i.e., the number of bytes in the
	// pipe

	if ((len & 1) == 0 || len <
#if SOFTWARE_CRC == 0
	// There is no software CRC at the end, so the minimum includes:
	// paylen + 2 bytes of SID + 2 status bytes
	5
#else
	// There is software CRC at the end, two bytes extra
	7
#endif
	    ) {

#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
		diag ("CC1100: %u RX BAD PL: %d", (word) seconds (), len);
#endif
		goto RRX;
	}

	// This is the logical payload length, i.e., the first byte of whatever
	// has arrived; note that it covers software checksum, if SOFTWARE_CRC
	paylen = get_reg (CCxxx0_RXFIFO);

	// Note that paylen must equal len - 3, regardless of SOFTWARE_CRC,
	// because of the two status bytes appended at the end by the chip
	if (paylen != len - 3) {

#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
		diag ("CC1100: %u RX PL MIS: %d/%d", (word) seconds (), len,
			paylen);
#endif
		goto RRX;
	}

	// We shall copy --len bytes, i.e., paylen + 2 (or, put differently,
	// whatever is present in the FIFO), because we need the status bytes
	// as well
	if (--len > rbuffl) {
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
		diag ("CC1100: %u RX LONG: %d", (word) seconds (), paylen);
#endif
		goto RRX;
	}

	// Copy the FIFO contents
	get_reg_burst (CCxxx0_RXFIFO, (byte*)rbuff, (byte) len);

	// We have emptied the FIFO, so we can immediately start RCV for
	// another packet
	enter_rx ();

	if (statid != 0 && statid != 0xffff) {
		// Admit only packets with agreeable statid
		if (rbuff [0] != 0 && rbuff [0] != statid) {
			// Drop
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
			diag ("CC1100: %u RX BAD STID: %x", (word) seconds (),
				rbuff [0]);
#endif
			goto Rtn;
		}
	}

#if SOFTWARE_CRC
	// Verify software CRC (at this point len = paylen + 2)
	len = paylen >> 1;

#if RADIO_CRC_TRANSPARENT
	// Receive packets with bad checksum
	eptr = (byte*)rbuff + paylen;
	b = *(eptr + 1) & 0x7f;
	if (w_chk (rbuff, len, 0))
		b |= 0x80;
	((byte*)rbuff) [paylen - 2] = b;
#else
	if (w_chk (rbuff, len, 0)) {
		// Bad checksum
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
		diag ("CC1100: %u RX CKS (S) %x %x %x", (word) seconds (),
			(word*)(rbuff) [0],
			(word*)(rbuff) [1],
			(word*)(rbuff) [2]);
#endif
		// Ignore
		goto Rtn;
	}

	// Move the extra two bytes from after paylen over CRC (tweaking them
	// a bit)
	eptr = (byte*)rbuff + paylen;
	((byte*)rbuff) [paylen - 2] = (*(eptr + 1) & 0x7f);

#endif	/* RADIO_CRC_TRANSPARENT */

	// This number is signed and starts negative, so we have to bias it
	((byte*)rbuff) [paylen - 1] = *((char*)eptr) + 128;
	add_entropy (rbuff [len - 1]);

#else	/* SOFTWARE_CRC (the hardware option) */

	// Status bytes (now in place)
	eptr = (byte*)rbuff + paylen;
	add_entropy (*eptr);
	b = *(eptr+1);
	*(eptr+1) = *((char*)eptr) + 128;
	*eptr = b;
	// Add the two status byte to the payload
	paylen += 2;
	
#if RADIO_CRC_TRANSPARENT == 0 && AUTOFLUSH_FLAG == 0

	if ((b & 0x80) == 0) {
		// Bad checksum
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
		diag ("CC1100: %u RX CKS (H) %x %x %x", (word) seconds (),
			(word*)(rbuff) [0],
			(word*)(rbuff) [1],
			(word*)(rbuff) [2]);
#endif
		// Ignore
		goto Rtn;
	}
#endif	/* RADIO_CRC_TRANSPARENT */

#endif	/* SOFTWARE_CRC */

#if (RADIO_OPTIONS & RADIO_OPTION_STATS)
	rerror [RERR_RCPS] ++;
#endif

#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
	diag ("CC1100: %u RX OK %x %x %x", (word) seconds (),
		(word*)(rbuff) [0],
		(word*)(rbuff) [1],
		(word*)(rbuff) [2]);
#endif
	tcvphy_rcv (physid, rbuff, paylen);
Rtn:
	gbackoff (RADIO_LBT_BACKOFF_RX);
	LEDI (2, 0);

#ifdef	MONITOR_PIN_CC1100_RX
	_PVS (MONITOR_PIN_CC1100_RX, 0);
#endif

}

// ============================================================================
// The thread =================================================================
// ============================================================================

#define	DR_LOOP		0
#define	DR_SWAIT	1
#define	DR_RECALIBRATE	2
#define	DR_WORTMOUT	3
#define	DR_XMIT		4
#define	DR_BREAK	5

thread (cc1100_driver)

  address xbuff;
  int paylen;

#if SOFTWARE_CRC
  int len;
#endif

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
  address ppm;
  word pcav;
#endif

  entry (DR_LOOP)

DR_LOOP__:

	if (RxST != RCV_STATE_PD) {
		// Take care of the RX FIFO; only when the chip is solid off,
		// don't we have to look there
		while (CC1100_RX_FIFO_READY)
			do_rx_fifo ();
	}

#if RADIO_WOR_MODE
	else if (woron) {
		// PD & woron means that we are running WOR; threat this as a
		// signal to get back to normal; if this is an interrupt, it
		// means PQT reached; otherwise, it is a request to the driver,
		// which requires it to have a look around and maybe transmit
		// a packet; in any case, we begin to listen immediately
		power_up ();
		// When woron is set, RxST should never become RCV_STATE_OFF
		RxST = RCV_STATE_ON;
	}
#endif

	// The next thing to find out is whether we have anything to transmit
	if ((xbuff = tcvphy_top (physid)) == NULL) {
		wait (__pi_v_qevent, DR_LOOP);
#if RADIO_WOR_MODE
		if (woron) {
			// Listen for a while before resuming WOR; do that only
			// if nothing happens in the meantime; note: we do not
			// periodically recalibrate here understanding that the
			// wait in RX is going to be quite finite
			delay (wor_idle_timeout, DR_WORTMOUT);
			cc1100_rcv_int_enable;
			release;
		}
#endif
		if (RxST == RCV_STATE_OFF) 
			// Nothing to transmit, no WOR (RxST is never OFF if
			// woron is set), go to PD state; note that power_down
			// sets RxST to RCV_STATE_PD
			power_down ();

		if (RxST != RCV_STATE_PD) {
			// Receiver is on
#if RADIO_RECALIBRATE
			// Periodic recalibration
			delay (RADIO_RECALIBRATE * 1024, DR_RECALIBRATE);
#endif
			cc1100_rcv_int_enable;
		}
		release;
	}

	// There is a packet to transmit

#ifdef	MONITOR_PIN_CC1100_TXP
	_PVS (MONITOR_PIN_CC1100_TXP, 1);
#endif
	if (bckf_timer) {
		// Backoff wait
#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
Bkf:
#endif
		wait (__pi_v_qevent, DR_LOOP);
		delay (bckf_timer, DR_LOOP);
		if (RxST != RCV_STATE_PD)
			cc1100_rcv_int_enable;
		release;
	}

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
	// Check for CAV requested in the packet
	ppm = xbuff + ((tcv_tlength (xbuff) >> 1) - 1);
	if ((pcav = (*ppm) & 0x0fff)) {
		// Remove for next turn
		*ppm &= ~0x0fff;
		utimer_set (bckf_timer, pcav);
		goto Bkf;
	}
#endif
	// Transmission OK

	if (RxST == RCV_STATE_PD) {
		// We are powered down, have to power up, cannot happen with
		// woron, as we should be up at this stage in RX
		power_up ();
		RxST = RCV_STATE_OFF;
	}

	// ====================================================================
	// LBT ================================================================
	// ====================================================================

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS) && RADIO_POWER_SETTABLE
	if ((byte)(pcav = (*ppm >> 12) & 0x7) != power) {
		power = (byte)pcav;
		set_reg (CCxxx0_FREND0, FREND0_SET | power);
	}
#endif

#if RADIO_LBT_MODE == 3
	// Staged/limited
#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
	if (*ppm & 0x8000)
		// LBT off requested by the packet
		retr = LBT_RETR_FORCE_OFF;
#endif
	if (retr < LBT_RETR_FORCE_RCV) {
		// Retry count below forcing threshold
		if (retr == 0) {
			// Initialize MCSM1
			set_reg (CCxxx0_MCSM1, MCSM1_LBT_FULL);
#if (RADIO_OPTIONS & RADIO_OPTION_STATS)
			if (rerror [RERR_XMEX] == MAX_WORD)
				memset (rerror + RERR_XMEX, 0,
					sizeof (word) * 2);
			else
				rerror [RERR_XMEX] ++;
#endif
		}
		set_reg (CCxxx0_AGCCTRL1, cc1100_agcctrl_table [retr]);
	} else {
		// Forcing
		set_reg (CCxxx0_MCSM1,
			retr < LBT_RETR_FORCE_OFF ?
				// Still honor own receptions in pogress
				MCSM1_LBT_RCV :
				// Complete OFF
				MCSM1_LBT_OFF);
	}
#endif
	// Channel assessment
	strobe (CCxxx0_STX);
	if (read_status () != CC1100_STATE_TX) {

#if RADIO_LBT_MODE == 3
		if (retr == LBT_RETR_LIMIT-1) {
			// Limit reached, ignore and start again; I guess we
			// used to drop packets here, but it makes no sense,
			// so just pretend you drop it
#if (RADIO_OPTIONS & RADIO_OPTION_STATS)
			// This one is never bigger than RERR_XMEX
			rerror [RERR_XMDR] ++;
#endif
			// Pretend the packet has been transmitted
			set_congestion_indicator (0);
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
			diag ("CC1100: RTL!!");
#endif
			goto FEXmit;
		}
		retr++;
#endif

#if ((RADIO_OPTIONS & (RADIO_OPTION_RBKF | RADIO_OPTION_STATS)) == (RADIO_OPTION_RBKF | RADIO_OPTION_STATS))
		if (rerror [RERR_CONG] >= 0x0fff)
				diag ("CC1100: LBT CNG");
#endif

#if RADIO_LBT_BACKOFF_EXP == 0
		// Aggressive transmitter
		delay (1, DR_LOOP);
		set_congestion_indicator (1);
		update_bckf_lbt (1);
		release;
#else
		// Backoff
		gbackoff (RADIO_LBT_BACKOFF_EXP);
		set_congestion_indicator (bckf_timer);
		update_bckf_lbt (bckf_timer);
		goto DR_LOOP__;
#endif
	} else {
		// Channel access granted
		set_congestion_indicator (0);
	}

#ifdef	MONITOR_PIN_CC1100_TXP
	_PVS (MONITOR_PIN_CC1100_TXP, 0);
#endif

#ifdef	MONITOR_PIN_CC1100_TXS
	_PVS (MONITOR_PIN_CC1100_TXS, 1);
#endif

#if RADIO_WOR_MODE

	if (tcv_isurgent (xbuff)) {
		// A waking packet, start with a large preamble
		delay (wor_preamble_time, DR_XMIT);
		wait (__pi_v_qevent, DR_BREAK);
		release;
	}

  entry (DR_XMIT)

	if ((xbuff = tcvphy_get (physid, &paylen)) == NULL)
		// This can fail, because we may have been sleeping while
		// sending the long preamble
		goto Break;
#else
	// This cannot possibly fail, xbuf already assigned
	tcvphy_get (physid, &paylen);
#endif

	// A packet arriving from TCV always contains CRC slots, even if
	// we are into hardware CRC; its minimum legit length is SID + CRC

#if SOFTWARE_CRC
	// In this case, rbuff is 2 bytes larger than the largest transmittable
	// packet, as (on top of CRC) it must accommodate two extra (status)
	// bytes
	sysassert (paylen <  rbuffl && paylen >= 4 && (paylen & 1) == 0,
		"cc11 py");
#else
	// Here, there is no software CRC, so the two status bytes take the
	// place of CRC (which is not used)
	sysassert (paylen <= rbuffl && paylen >= 4 && (paylen & 1) == 0,
		"cc11 py");
#endif
	LEDI (1, 1);

	if (statid != 0xffff)
		// This means "honor the packet's statid"
		xbuff [0] = statid;

#if (RADIO_OPTIONS & RADIO_OPTION_RBKF)
	xbuff [1] = bckf_lbt;
	bckf_lbt = 0;
#endif

#if SOFTWARE_CRC
	// Calculate CRC
	len = (paylen >> 1) - 1;
	((word*)xbuff) [len] = w_chk ((word*)xbuff, len, 0);
#else
	paylen -= 2;		// Ignore the CRC bytes
#endif
	// Send the length byte ...
	set_reg (CCxxx0_TXFIFO, paylen);

	// ... and the packet
	set_reg_burst (CCxxx0_TXFIFO, (byte*)xbuff, (byte) paylen);

	// ... and release it
	tcvphy_end (xbuff);

	// Wait for some minimum time needed to transmit the packet

	delay (approx_xmit_time (paylen), DR_SWAIT);

	release;

  entry (DR_SWAIT)

#ifdef CC_BUSY_WAIT_FOR_EOT
	while (read_status () == CC1100_STATE_TX);
#else
	if (read_status () == CC1100_STATE_TX) {
		delay (TXEND_POLL_DELAY, DR_SWAIT);
		release;
	}
#endif

#if RADIO_RECALIBRATE
	// Explicit transition to RX to recalibrate
	enter_rx ();
#endif
	LEDI (1, 0);

#if RADIO_LBT_XMIT_SPACE
	utimer_set (bckf_timer, RADIO_LBT_XMIT_SPACE);
#endif

#ifdef	MONITOR_PIN_CC1100_TXS
	_PVS (MONITOR_PIN_CC1100_TXS, 0);
#endif

FEXmit:

#if RADIO_LBT_MODE == 3
	retr = 0;
#endif
	goto DR_LOOP__;

#if RADIO_RECALIBRATE

  entry (DR_RECALIBRATE)

	enter_idle ();
	enter_rx ();
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
	diag ("CC1100: %u RECAL", (word) seconds ());
#endif
	goto DR_LOOP__;
#endif

#if RADIO_WOR_MODE

  entry (DR_WORTMOUT)

	if (woron && !CC1100_RX_FIFO_READY && tcvphy_top (physid) == NULL) {
		// This will enter WOR mode. Note: we could add more conditions:
		//
		//	- PQT reached (that would require running the RX mode
		//	  with PQT enabled, which we don't do)
		//
		//	- FIFO nonempty (meaning a packet is being received)
		//
		// with the intention of catching packets in the process of
		// being received, which are otherwise missed. Let's wait and
		// see if we are losing a lot by ignoring those cases. After
		// all, the role of the timeout (which gets us here) is to make
		// sure that we don't.
		wait (__pi_v_qevent, DR_LOOP);
		power_down ();
		release;
	}

	goto DR_LOOP__;

  entry (DR_BREAK)

	// Abort the preamble, enter RX, and try again
Break:
	enter_rx ();
	goto FEXmit;

#endif

endthread

// ============================================================================

static int option (int opt, address val) {

	int ret = 0;

	switch (opt) {

	    case PHYSOPT_STATUS:

		ret = 2 | (RxST == RCV_STATE_ON);
		goto RVal;

	    case PHYSOPT_RXON:

		if (RxST == RCV_STATE_PD)
			// We have been switched off
			power_up ();
#if RADIO_WOR_MODE
		woron = 0;
#endif
		RxST = RCV_STATE_ON;
		LEDI (0, 1);
OREvnt:
		p_trigger (__pi_v_drvprcs, __pi_v_qevent);

	    case PHYSOPT_TXON:
	    case PHYSOPT_TXOFF:
	    case PHYSOPT_TXHOLD:

		goto RRet;

	    case PHYSOPT_RXOFF:

#if RADIO_WOR_MODE
		if (val != NULL && *val) {
			// Enter WOR
			if (!woron) {
				// Not in WOR already
				woron = 1;
				if (RxST == RCV_STATE_OFF)
					// Make sure that RxST is never
					// RCV_STATE_OFF when woron is set;
					// will truly enter WOR after the first
					// timeout
					RxST = RCV_STATE_ON;
			}
			// If we are already in WOR, this will run us through
			// a wakeup cycle (falling through this if)
		} else {
			// Normal PD
			if (woron || RxST == RCV_STATE_ON)
				RxST = RCV_STATE_OFF;
			woron = 0;
		}
#else
		if (RxST == RCV_STATE_ON)
			RxST = RCV_STATE_OFF;
#endif
		LEDI (0, 0);
		goto OREvnt;

	    case PHYSOPT_SETSID:

		statid = (val == NULL) ? 0 : *val;
		goto RRet;

            case PHYSOPT_GETSID:

		ret = (int) statid;
		goto RVal;

	    case PHYSOPT_GETMAXPL:

		ret = rbuffl
#if SOFTWARE_CRC
			- 2
#endif
		;
		goto RVal;

#if (RADIO_OPTIONS & RADIO_OPTION_STATS)
	    case PHYSOPT_ERROR:

		if (val != NULL) {
			if (rerror [RERR_CURB] > rerror [RERR_MAXB])
				rerror [RERR_MAXB] = rerror [RERR_CURB];
			memcpy (val, rerror, RERR_SIZE);
		} else
			memset (rerror, 0, sizeof (rerror));

		goto RRet;
#endif

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

#if RADIO_BITRATE_SETTABLE

	    case PHYSOPT_GETRATE:

		ret = (int) vrate;
		goto RVal;
#endif

	}

	// These ones cannot (or should not) be handled  in PD mode (including
	// WOR)
	if (RxST == RCV_STATE_PD)
		enter_idle ();

	switch (opt) {

#if RADIO_POWER_SETTABLE && (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS) == 0

	    case PHYSOPT_SETPOWER:

		power = (val == NULL) ? RADIO_DEFAULT_POWER :
			((*val > 7) ? 7 : (byte) (*val));
		set_reg (CCxxx0_FREND0, FREND0_SET | power);
		break;
#endif

#if RADIO_CHANNEL_SETTABLE

	    case PHYSOPT_SETCHANNEL:

		channel = (val == NULL) ? RADIO_DEFAULT_CHANNEL : (byte) (*val);
		set_reg (CCxxx0_CHANNR, channel);
		break;

#endif

#if RADIO_BITRATE_SETTABLE

	    case PHYSOPT_SETRATE:

		// For now, we just DO IT. It is not clear what will happen,
		// if the modification interferes with an ongoing transmission
		// or reception, but we shall see.

		{
			const byte *r;

			vrate = (val == NULL) ? RADIO_BITRATE_INDEX :
				((*val >= CC1100_NRATES) ? CC1100_NRATES - 1 :
					*val);

			r = cc1100_ratemenu [vrate];

			set_reg_group (cc1100_ratereg, r,
				sizeof (cc1100_ratereg));
		}

		break;
#endif
	    case PHYSOPT_RESET:

#if (RADIO_OPTIONS & RADIO_OPTION_REGWRITE)
		if (val != NULL) {

			byte *r;

			for (r = (byte*)val; *r != 255; r += 2)
				set_reg (*r, *(r+1));

			// Do not reset
			break;
		}
#endif
		chip_reset ();
		goto OREvnt;

#if RADIO_WOR_MODE
	    // Change WOR params
	    case PHYSOPT_SETPARAMS:

		if (val == NULL) {
			set_default_wor_params ();
		} else {
			wor_idle_timeout = *val++;
			wor_preamble_time = *val;
#if (RADIO_OPTIONS & RADIO_OPTION_WORPARAMS)
			byte *v = (byte*)(++val);
			// High byte of EVT0 value
			set_reg (CCxxx0_WOREVT1, v [0]);
			// RX time
			cc1100_wor_von [3] = 
				((v [3] == 0) ?
				    // Switch off RSSI thresholding
				    CCxxx0_MCSM2_WOR_P :
				        // RSSI thresholding is on
					CCxxx0_MCSM2_WOR_RP) |
					    ((v [1] > 6) ? 6 : v [1]);
			// PQT
			cc1100_wor_von [0] = (cc1100_wor_von [0] & 0x1F) |
				(((v [2] == 0) ? 1 : ((v [2] > 7) ? 7 :
					v [2])) << 5);
			// RSSI threshold
			cc1100_wor_von [1] = (cc1100_wor_von [1] & 0xF0) |
				((((v [3] > 15) ? 15 : v [3]) - 8) & 0x0F);
			// EVNT1
			cc1100_wor_von [2] = (cc1100_wor_von [2] & 0x8F) |
				(((v [4] > 7) ? 7 : v [4]) << 4);
#endif
		}

		break;
#endif
	    default:

		syserror (EREQPAR, "cc11 op");

	}

	if (RxST == RCV_STATE_PD)
		// This should work OK also for WOR
		power_down ();

	// ====================================================================
RRet:
	return ret;
RVal:
	if (val != NULL)
		*val = ret;
	goto RRet;
}

// ============================================================================

void phys_cc1100 (int phy, int mbs) {
//
// Note that mbs DOES COVER the checksum!!!
//
// A packet sent over the radio starts with a length byte (called the payload
// length) and its full length is in fact odd. What follows the length byte is
// the TCV packet, i.e., the payload. The maximum payload length is 60 bytes,
// because the FIFO size is 64 bytes, and one of them is used for the payload
// length byte, and two status bytes are appended at the end upon reception.
// Those status bytes have nothing to do with the CRC: they are the Link
// Quality and RSSI bytes calculated by the chip.
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
// length is (always) mbs - 2.
//
#if (RADIO_OPTIONS & RADIO_OPTION_NOCHECKS) == 0
	if (rbuff != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "cc11");
#endif

	if (mbs == 0)
		mbs = CC1100_MAXPLEN;

#if (RADIO_OPTIONS & RADIO_OPTION_NOCHECKS) == 0
	if (mbs < 6 || mbs > CC1100_MAXPLEN)
		syserror (EREQPAR, "cc11 mb");
#endif

#if !defined(__CC430__) && (RADIO_OPTIONS & RADIO_OPTION_CCHIP)
	// Chip connectivity test
	cc1100_csn_down;
	mdelay (2);
	if (cc1100_so_val) {
		// This precludes hangups that would have been caused by stuck
		// cc1100_so_val on our first attempt to talk to the chip
		syserror (EHARDWARE, "cc11 ch");
	}
	// cc1100_csn_up
	CC1100_SPI_END;
#endif

	// Buffer length in bytes including the checksum
	rbuffl = (byte) mbs
#if SOFTWARE_CRC
		+ 2	// Need extra room for the status bytes
#endif
	;
	rbuff = umalloc (rbuffl);
#if (RADIO_OPTIONS & RADIO_OPTION_NOCHECKS) == 0
	if (rbuff == NULL)
		syserror (EMALLOC, "cc11");
#endif
	physid = phy;

	/* Register the phy */
	__pi_v_qevent = tcvphy_reg (phy, option, INFO_PHYS_CC1100);

	LEDI (0, 0);
	LEDI (1, 0);
	LEDI (2, 0);

	// Initialize the device
	cc1100_ini_regs;
	chip_reset ();

#if DIAG_MESSAGES
	diag ("CC1100E: %d, %d.%dMHz, %d/%dkHz=%d.%dMHz", RADIO_BITRATE_INDEX,
		CC1100_BFREQ, CC1100_BFREQ_10, RADIO_DEFAULT_CHANNEL,
			CC1100_CHANSPC_T1000, CC1100_DFREQ, CC1100_DFREQ_10);
#endif

#if (RADIO_OPTIONS & RADIO_OPTION_ENTROPY) && ENTROPY_COLLECTION

	// Collect initial entropy
	{
		word i;

		for (i = 0; i < 8; i++) {
			enter_rx ();
			// The worst case settling time for RSSI after RX is
			// ca. 750 us
			mdelay (1);
			entropy = (entropy << 4) |
				(get_reg (CCxxx0_RSSI) & 0xF);
			enter_idle ();
		}
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
		diag ("CC1100 ENTR: %x%x", (word)entropy, (word)(entropy>>16));
#endif
		power_down ();
	}
#endif

	// Install the backoff timer
	utimer_add (&bckf_timer);

	// Start the driver process
	__pi_v_drvprcs = runthread (cc1100_driver);
#if (RADIO_OPTIONS & RADIO_OPTION_NOCHECKS) == 0
	if (__pi_v_drvprcs == 0)
		syserror (ERESOURCE, "cc11");
#endif
}
