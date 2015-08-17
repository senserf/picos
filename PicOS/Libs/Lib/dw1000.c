/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2015                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define	DW1000_DEFINE_RF_SETTINGS
#include "dw1000.h"
#undef DW1000_DEFINE_RF_SETTINGS

// ============================================================================
// Chip access functions ======================================================
// ============================================================================

static void spi_out (byte b) {

	register byte i;

	for (i = 0; i < 8; i++) {
		if (b & 0x80)
			dw1000_si_up;
		else
			dw1000_si_down;
		b <<= 1;
		dw1000_sclk_up;
		DW1000_SPI_WAIT;
		dw1000_sclk_down;
		DW1000_SPI_WAIT;
	}
}

static byte spi_in () {

	register byte i, val;

	for (i = val = 0; i < 8; i++) {
		val <<= 1;
		if (dw1000_so_val)
			val |= 1;
		dw1000_sclk_up;
		DW1000_SPI_WAIT;
		dw1000_sclk_down;
		DW1000_SPI_WAIT;
	}

	return val;
}

static void chip_trans (byte reg, word index) {
//
// Start SPI transaction, reg includes the write flag, if required
//
	DW1000_SPI_START;
	if (index)
		// Indexed
		reg |= 0x40;
	spi_out (reg);
	if (index) {
		if (index < 128) {
			spi_out ((byte)index);
			return;
		}
		spi_out (((byte)index) | 0x80);
		spi_out ((byte)(index >> 7));
	}
}

#if !(DW1000_OPTIONS & 0x0001)
static
#endif
void chip_write (byte reg, word index, word length, byte *stuff) {
//
// Write
//
	chip_trans (reg | 0x80, index);
	while (length--)
		spi_out (*stuff++);
	DW1000_SPI_STOP;
}

#if !(DW1000_OPTIONS & 0x0001)
static
#endif
void chip_read (byte reg, word index, word length, byte *stuff) {
//
// Read
//
	chip_trans (reg, index);
	while (length--)
		*stuff++ = spi_in ();
	DW1000_SPI_STOP;
}

// ============================================================================
// ============================================================================

// Not sure if we need to store ldotune, probably not, but antenna delay must
// be restored after every wakeup; note that antenna delay is separate for
// PRF64 and PRF16
static word	antdelay = 0,
		pan = 0;		// PAN

// This is the default mode; resettable with the RESET physopt
static chconfig_t mode;
static byte flags;

// Location data; it may make sense to allocate it dynamically based on the
// role (later)
static dw1000_locdata_t locdata;

#define	trr_upper	(*((lword*)(locdata.tst + DW1000_TSOFF_TRR + 1)))

// The thread
sint __dw1000_v_drvprcs;

// ============================================================================
// ============================================================================

#if 0
static void reset_rx () {

	byte c;

	c = 0xe0;
	chip_write (DW1000_REG_PMSC, 3, 1, &c);
	c = 0xf0;
	chip_write (DW1000_REG_PMSC, 3, 1, &c);
}
#endif

static lword read_otpm (word addr) {
//
// Reads 4 bytes from OTP memory
//
	lword res;

	chip_write (DW1000_REG_OTP_IF, 4, 2, (byte*)&addr);
	addr = 0x03;	// Manual drive OTP_READ
	chip_write (DW1000_REG_OTP_IF, 6, 1, (byte*)&addr);
	addr = 0x00;
	chip_write (DW1000_REG_OTP_IF, 6, 1, (byte*)&addr);
	chip_read (DW1000_REG_OTP_IF, 10, 4, (byte*)&res);
	return res;
}

static void tosleep () {

	byte b;

	// The manual says: in order to put the DW1000 into the SLEEP state
	// SLEEP_EN needs to be set in AON_CFG0 and then the configuration
	// needs to be uploaded to the AON using the UPL_CFG bit in AON_CTRL.
	// This assumes that SLEEP_EN is already set.
	b = 0x00;
	chip_write (DW1000_REG_AON, 2, 1, &b);
	b = 0x02;
	chip_write (DW1000_REG_AON, 2, 1, &b);
}

static void wakeitup () {
//
// Wakes up the chip from lp mode; note: for now we spin like crazy; this may
// have to be reorganized into an FSM later
//
	word w;

	DW1000_SPI_START;
	udelay (250);
	DW1000_SPI_STOP;

	for (w = 0; w < 50; w++) {
		mdelay (1);
		if (dw1000_ready)
			goto Ready;
	}

	syserror (EHARDWARE, "dw1");

	// After going through all those pains to detect when the chip gets up,
	// they delay for about this much to "stabilize the crystal"
Ready:
	mdelay (90);

	// Now we have to reload the antenna delay; this makes me wonder why
	// bother; can't we just account for it in the formula?
	chip_write (DW1000_REG_TX_ANTD, 0, 2, (byte*)&antdelay);

	// Should we put some test here to make sure we are actually alive?
}

static void initialize () {
//
// After startup
//
	word w, v;
	byte a, b;
	lword lw, cf;

	// There is no way to easily and authoritatively reset the chip; RST
	// is down when in sleep, so to make the pull down meaningful, you
	// have to bring it up first; this presumes that wake (by CS) has been
	// configured; other than that, it is power down and up again
	wakeitup ();
	DW1000_RESET;	// Hard reset

	// Before configuring PLL, the SPI rate is not supposed to exceed 3M
	// I am not sure how fast we can ever get, probably not much faster
	// than that, so let us just ignore this little issue and keep our
	// fingers crossed

	// The reference driver sets the clocks to XTI for reading OTP, it says
	// that otherwise the operation is unreliable; I think that this
	// amounts to setting the LSB of PMSC CTRL0 to 0x01
	b = 0x01;
	chip_write (DW1000_REG_PMSC, 0, 1, &b);

#if (DW1000_OPTIONS & 0x0001)
	chip_read (DW1000_REG_DEVID, 0, 4, (byte*)&lw);
	diag ("DWINIT: %x%x", (word)(lw >> 16), (word) lw);
#endif

	// Read and preserve antenna delay; will be set manually on every wake,
	// so we just make sure it is handy
	lw = read_otpm (DW1000_ADDR_ANTDELAY);
	antdelay = (word)((mode.prf ? (lw >> 16) : lw) & 0xffff);

#if (DW1000_OPTIONS & 0x0001)
	diag ("OTPAD: %u", antdelay);
#endif
	// Determine the actual delay to use
	if (antdelay == 0)
		// Use some default delay as per reference driver
		antdelay = dw1000_def_rfdelay (mode.prf);

	// AON wakeup configuration; L64P is only set for 64B preamble, which
	// we don't use; not sure if PRES_SLEEP is needed at this stage
	w = DW1000_ONW_LDC | DW1000_ONW_LLDE | DW1000_ONW_PRES_SLEEP;
	if ((read_otpm (DW1000_ADDR_LDOTUNE) & 0xff) != 0)
		w |= DW1000_ONW_LLDO;
	chip_write (DW1000_REG_AON, 0, 2, (byte*)&w);

	// Enable sleep with wake on CS; not sure if the SLEEP_EN bit should be
	// set now; apparently, it doesn't put the device to sleep; maybe we
	// should move this to the end of initialization?
	b = DW1000_CF0_SLEEP_EN | DW1000_CF0_WAKE_SPI;
	chip_write (DW1000_REG_AON, 6, 1, &b);

	// Crystal trim
	if ((b = (byte) read_otpm (DW1000_ADDR_XTRIM) & 0x1F) == 0)
		// No calibration value stored, use the default mid range, as
		// in the reference driver
		b = DW1000_PLL2_CALCFG;
	else
		// Write the trim or'red with the stuff that goes to the upper
		// bits of the byte; the manual says: bits 7:5 must always be
		// set to binary “011”. Failure to maintain this value will
		// result in DW1000 malfunction.
		b = (DW1000_PLL2_CALCFG & ~0x1f) | (b & 0x1f);

	chip_write (DW1000_REG_FS_CTRL, 0x0e, 1, &b);

	// Set the config register to the default, then prepare it and write
	// once
	cf = DW1000_CF_HIRQ_POL | DW1000_CF_DIS_DRXB ;
// PGTEST
#if 0
		DW1000_CF_FFEN | DW1000_CF_FFAD |
		// Auto RX re-enable, so we won't have to handle incorrect
		// frames by hand
		DW1000_CF_RXAUTR;
#endif

	// No, we don't touch EUI as we will be using short addresses only
#if 0
	// Set EUI to be all ones except for the last word, which is
	// equal to Host Id
	lw = 0;
	chip_write (DW1000_REG_EUI, 0, 4, (byte*)&lw);
	lw = (word)host_id;
	chip_write (DW1000_REG_EUI, 4, 4, (byte*)&lw);
#endif
	// Set PAN and short address to network Id and Host Id; note: we should
	// reset this whenever the Host Id changes!
	lw = ((word)host_id) | (((lword)pan) << 16);
	chip_write (DW1000_REG_PANADR, 0, 4, (byte*)&lw);

	// TX config (power); this is an array of lw power entries, two entries
	// per channel starting at 1, first entry for PRF 16M, the other for
	// 64M
	lw = read_otpm (DW1000_ADDR_TXCONF + (mode.channel ? 8 : 2) + mode.prf);
	if (lw == 0 || lw == MAX_LWORD)
		// Absent
		lw = dw1000_def_txpower;

	if (!dw1000_use_smartpower) {
		// In the reference driver, if smartpower is used, then the
		// tx power value is applied "as-is"; otherwise, its LS byte
		// is replicated over all the remaining bytes (how is this
		// for black magic?)
		memset (&lw, (byte)lw, 4);
		// Disable smart power (enabled by default)
		_BIS (cf, DW1000_CF_DIS_STPX);
		// This (no smartpower) implies (just in our case) 110K rate
		_BIS (cf, DW1000_CF_RXM110K);
	}

	// Pulse generator calibration + TX power
	b = dw1000_def_pgdelay;
	chip_write (DW1000_REG_TX_CAL, 0x0b, 1, &b);
	chip_write (DW1000_REG_TX_POWER, 0, 4, (byte*)&lw);

	// Note that we always use filtering (FFEN is set by default - see
	// above); a Tag only accepts frames addressed to itself, while a Peg
	// behaves as a "coordinator", i.e., accepts destination-less frames
	// where PAN ID matches its PAN ID (i.e., frames restricted to our
	// Network Id); we do not use ACK frames at all (do we have to?)
// PGTEST
#if 0
	if (flags & DW1000_FLG_ANCHOR)
		_BIS (cf, DW1000_CF_FFBC);
#endif

	// LDELOAD, note: this is probably not needed if LLDE is set in AON
	// (as it is)
	w = 0x8000;
	chip_write (DW1000_REG_OTP_IF, 6, 2, (byte*)&w);
	udelay (200);

	// Enable clocks normal; note: the reference driver does some weird
	// things to the apparently unused bits of the PMSC_CTRL0, my shortcut
	// is to write there what is needed
	w = 0x0200;
	chip_write (DW1000_REG_PMSC, 0, 2, (byte*)&w);

	// Write the configuration register
	chip_write (DW1000_REG_SYS_CFG, 0, 4, (byte*)&cf);

	// ====================================================================
	// Not sure if this block belongs here, probably makes no difference;
	// Configure LDE
	w = dw1000_lde_coeff;
	chip_write (DW1000_REG_LDE_IF, 0x2804, 2, (byte*)&w);

	b = DW1000_LDE_PARAM1;
	chip_write (DW1000_REG_LDE_IF, 0x0806, 1, &b);

	w = dw1000_lde_param3;
	chip_write (DW1000_REG_LDE_IF, 0x1806, 2, (byte*)&w);

	// Configure RF PLL
	chip_write (DW1000_REG_FS_CTRL, 7, 5, (byte*)dw1000_pll2_config);

#if 0
	// Already done above
	b = DW1000_PLL2_CALCFG;
	chip_write (DW1000_REG_FS_CTRL, 0x0e, 1, &b);
#endif

	// RF RX blocks
	b = DW1000_RX_CONFIG;
	chip_write (DW1000_REG_RF_CONF, 0x0b, 1, &b);

	lw = dw1000_rf_txctrl;
	// Note: the most significant byte ends up to DE no matter what we
	// write; some undocumented feature
	chip_write (DW1000_REG_RF_CONF, 0x0c, 4, (byte*)&lw);

	// Baseband parameters
	w = dw1000_sftsh;
	chip_write (DW1000_REG_DRX_CONF, 0x02, 2, (byte*)&w);

	w = dw1000_dtune1;
	chip_write (DW1000_REG_DRX_CONF, 0x04, 2, (byte*)&w);

	// dtune1b
	if (mode.datarate) {
		// 6M8: dtune1b; we don't use preamble length of 64
		w = 0x20;
	} else {
		w = 0x64;
	}
	chip_write (DW1000_REG_DRX_CONF, 0x06, 2, (byte*)&w);

	b = 0x28;
	chip_write (DW1000_REG_DRX_CONF, 0x26, 1, &b);

	// dtune2
	lw = dw1000_dtune2;
	chip_write (DW1000_REG_DRX_CONF, 0x08, 4, (byte*)&lw);

	// dtune3
	w = dw1000_sfdto;
	chip_write (DW1000_REG_DRX_CONF, 0x20, 2, (byte*)&w);

	// AGC
	lw = DW1000_AGCTUNE2;
	chip_write (DW1000_REG_AGC_CTRL, 0x0c, 4, (byte*)&lw);
	w = dw1000_agctune1;
	chip_write (DW1000_REG_AGC_CTRL, 0x04, 2, (byte*)&w);

	w = dw1000_channel;
	lw = w | (w << 4) | (((lword)dw1000_prf) << 18);
	w = dw1000_precode;
	lw |= ((lword)w << 22) | ((lword)w << 27);

	// Non standard SFD
	if (mode.nssfd) {
		b = dw1000_dwnssfdl;
		chip_write (DW1000_REG_USR_SFD, 0, 1, &b);
		lw |= 0x00320000;
	}

	chip_write (DW1000_REG_CHAN_CTRL, 0, 4, (byte*)&lw);

	// Preamble size, TX PRF, ranging bit
	lw = (((lword)(dw1000_preamble | dw1000_prf)) << 16) | 0x00008000 |
		(((lword)dw1000_datarate) << 13);

	if (flags & DW1000_FLG_ANCHOR)
		// This is the fixed length of the anchor's response; there are
		// two different lengths: 9 and 24 (including FCS) for a Tag
		lw |= 11;

	chip_write (DW1000_REG_TX_FCTRL, 0, 4, (byte*)&lw);

	// Write RX antenna delay, TX delay will be written on wakeup
	chip_write (DW1000_REG_LDE_IF, 0x1804, 2, (byte*)&antdelay);

	// ====================================================================

	// Preset the transmit buffer: this is common for both modes (PAN
	// follows the sequence number)
	chip_write (DW1000_REG_TX_BUFFER, 3, 2, (byte*)&pan);

	if (flags & DW1000_FLG_ANCHOR) {
		// Frame control bytes are 41 88 (short destination address,
		// short source address, pan compression, frame type = data
		w = 0x8841;
		v = 7;
	} else {
		// Frame control bytes are 01 80 (no destination address,
		// short source address, frame type = data; the buffer
		// layout is: 01 80 SQ PAN PAN SRC SRC ...; the length is
		// 7 (for the initial poll) and 22 (for the fin message which
		// additionally includes 3 * 5 = 15 bytes of timestamps); note
		// that the stored length must include two extra bytes for FCS
		w = 0x8001;
		// Position of source address
		v = 5;
	}
	chip_write (DW1000_REG_TX_BUFFER, 0, 2, (byte*)&w);
	chip_write (DW1000_REG_TX_BUFFER, v, 2, (byte*)&host_id);

	// ====================================================================

	// Enter sleep
	tosleep ();
#if (DW1000_OPTIONS & 0x0001)
	diag ("INIT OK");
#endif
}

static void toidle () {
//
// Put the chip into IDLE state
//
	byte b;

	b = 0x40;
	chip_write (DW1000_REG_SYS_CTRL, 0, 1, &b);
}

static byte getevent () {
//
// Retrieves and clears the interrupt status
//
	lword status;
	byte len;

Redo:
	chip_read (DW1000_REG_SYS_STATUS, 0, 4, (byte*)&status);
diag ("GE: %x%x", (word)(status >> 16), (word)status);

	if (status & DW1000_IRQ_OTHERS) {
		// These are not auto-cleared
#if (DW1000_OPTIONS & 0x0001)
		if (status & (DW1000_IRQ_CLKPLL_LL | DW1000_IRQ_RFPLL_LL))
			diag ("LOCK! %x", (word)(status >> 16));
#endif
		status = DW1000_IRQ_OTHERS;
		chip_write (DW1000_REG_SYS_STATUS, 0, 4, (byte*)&status);
		goto Redo;
	}

	if (status & DW1000_IRQ_LDEDONE) {
		// This is a walkaround for a bug (creatively copied from the
		// reference driver). I wish I understood what I am doing.
		// My plan is to base the operation solely on receive events.
		// Say, the Tag starts a handshake by sending the first packet.
		// It sets up RX after TX with RX timeout. We will set up the
		// interrupt service for RX events only, assuming that the the
		// TX event is uninteresting. After RX, the Tag sends the final
		// packet of the handshake, and after TX it goes down until the
		// next try (we may add n ACK later). The Peg waits for RX (no
		// timeout). After RX, it does TX + RX and timeout. After the
		// final RX, it calculated the distance.
		if ((status & (DW1000_IRQ_RXPHD | DW1000_IRQ_RXSFDD)) !=
		    (DW1000_IRQ_RXPHD | DW1000_IRQ_RXSFDD)) {
			// Bad frame
diag ("DBUG");
			goto Bad;
		}
	}

	// If we check this before TX, then we may be OK for RX forcibly
	// following TX (if we ever go for this option), if the mask ignores
	// TX, but includes RX
	if (status & DW1000_IRQ_RXFCG) {
		// Receiver FCS OK
diag ("RXOK");
		if ((status & DW1000_IRQ_LDEDONE) == 0)
			goto Bad;
		if ((status & DW1000_IRQ_RXOVRR))
			goto Bad;
		// Receive
		chip_read (DW1000_REG_RX_FINFO, 0, 1, &len);
		// This will mean "reception"
		len &= 0x7f;
Rtn:
		// The device status should now be OK, so there is no need
		// to disable RXTX, because we manually initiate transmission
		// and reception, and do nothing else
		status = DW1000_IRQ_ALLSANE;
		chip_write (DW1000_REG_SYS_STATUS, 0, 4, (byte*)&status);
diag ("GE: %d", len);
		return len;
	}

	if (status & DW1000_IRQ_TXFRS) {
		// Xmit done
		len = DW1000_EVT_XMT;
		goto Rtn;
	}

	if (status & DW1000_IRQ_RXRFTO) {
		// Receive timeout; not sure if we are going to use this
		// feature; it may be handy in the Tag waiting for a Peg
		// response
		len = DW1000_EVT_TMO;
		// RX doesn't auto re-enable after frame wait timeout
		goto Rtn;
	}

	// Do we need that?
	if (status & DW1000_IRQ_RXERRORS) {
Bad:
		toidle ();
		len = DW1000_EVT_BAD;
		goto Rtn;
	}

	// None
diag ("GE: v");
	return DW1000_EVT_NIL;
}

static void irqenable (lword mask) {
	chip_write (DW1000_REG_SYS_MASK, 0, 4, (byte*)&mask);
}

static void rxenable_n_release (word st) {
//
// Enable RX
//
	word w;

	wait (&__dw1000_v_drvprcs, st);

// PGTEST
	irqenable (DW1000_IRQ_RXFINE | DW1000_IRQ_RXERRORS);
#if 0
	irqenable (DW1000_IRQ_RECEIVE);
#endif

	// RXENAB
	w = 0x100;
	chip_write (DW1000_REG_SYS_CTRL, 0, 2, (byte*)&w);

	dw1000_int_enable;

	release;
}

static void starttx_n_release (word st) {
//
// Start TX
//
	byte b;

	wait (&__dw1000_v_drvprcs, st);

	irqenable (DW1000_IRQ_TRANSMIT);

	// TXSTRT
	b = 0x02;
	chip_write (DW1000_REG_SYS_CTRL, 0, 1, &b);

	dw1000_int_enable;

	release;
}
	
// ============================================================================
// The anchor process
// ============================================================================

#define	DWA_INIT	0
#define	DWA_TPOLL	1
#define	DWA_RESP_DONE	2
#define	DWA_TFIN	3
#define	DWA_RESET	4

thread (dw1000_anchor)

    entry (DWA_INIT)

	wakeitup ();
	_BIC (flags, DW1000_FLG_LDREADY);
// Initial cleanup
getevent ();

retry_rxp:

	rxenable_n_release (DWA_TPOLL);

    entry (DWA_TPOLL)

	// IRQ wakeup

// PGTEST
#if 0
	if (getevent () != DW1000_FRLEN_TPOLL)
		// This must be the TPOLL packet length
		goto retry_rxp;
#endif
goto retry_rxp;

	// The chip should be IDLE at this point

	_BIC (flags, DW1000_FLG_LDREADY);
tpoll:

	// It looks like we have got a POLL; to save on time, we assume it is
	// correct, i.e., we do not verify anything (which we may want to do
	// later) only grab the sender Id and time the packet
	chip_read (DW1000_REG_RX_BUFFER, 3, 1, &(locdata.seq));
	// FIXME: to simplify things (reduce chip communication) we can ignore
	// the official sequence number in the frame and use custom one
	// adjacent to src/dst
	chip_read (DW1000_REG_RX_BUFFER, 5, 2, (byte*)&(locdata.tag));
	chip_read (DW1000_REG_RX_TIME, 0, DW1000_TSTAMP_LEN,
		locdata.tst + DW1000_TSOFF_TRP);

	// Insert the source Tag ID, now dst, into the outgoing message
	chip_write (DW1000_REG_TX_BUFFER, 5, 2, (byte*)&(locdata.tag));
	// Insert the sequence number
	chip_write (DW1000_REG_TX_BUFFER, 3, 1, &(locdata.seq));

retry_tx:
	// Transmit
	starttx_n_release (DWA_RESP_DONE);

    entry (DWA_RESP_DONE)

	if (getevent () != DW1000_EVT_XMT)
		// Do we really have to check? Probably, if anything, it makes
		// better sense to assume that any event out of the ordinary
		// breaks the handshake
		goto retry_tx;

	// Transmit time
	chip_read (DW1000_REG_TX_TIME, 0, DW1000_TSTAMP_LEN,
		locdata.tst + DW1000_TSOFF_TSR);

	// Receive the fin packet
	delay (DW1000_TMOUT_FIN, DWA_RESET);
	rxenable_n_release (DWA_TFIN);

    entry (DWA_TFIN)

	{
		byte b;

		if ((b = getevent ()) == DW1000_FRLEN_TPOLL)
			// In case we receive another poll: then we restart the
			// whole thing
			goto tpoll;

		if (b != DW1000_FRLEN_TFIN)
			goto retry_rxp;

		// Check the sequence number
		chip_read (DW1000_REG_RX_BUFFER, 3, 1, &b);
		if (b != locdata.seq)
			goto retry_rxp;
	}

	{
		word s;

		// Check the src
		chip_read (DW1000_REG_RX_BUFFER, 5, 2, (byte*)&s);
		if (s != locdata.tag)
			goto retry_rxp;
	}

	// Copy the time stamps
	chip_read (DW1000_REG_RX_BUFFER, 7, 3 * DW1000_TSTAMP_LEN,
		locdata.tst + DW1000_TSOFF_TSP);

	// Done, the data is ready
	_BIS (flags, DW1000_FLG_LDREADY);
	trigger (&locdata);
	goto retry_rxp;

    entry (DWA_RESET)

	toidle ();
	goto retry_rxp;

endthread

// ============================================================================
// The range (poll) process
// ============================================================================

#define	RAN_INIT	0
#define	RAN_TPOLL	1
#define	RAN_POLL_DONE	2
#define	RAN_ANCHOR	3
#define	RAN_FIN		4
#define	RAN_FAILURE	5

thread (dw1000_range)

    entry (RAN_INIT)

	// We wake it up for a single episode; this will change later
	wakeitup ();

	// Use this as the try counter
	locdata.tag = DW1000_MAX_TTRIES;

    entry (RAN_TPOLL)

	// Insert the sequence number into the outgoing packet
	chip_write (DW1000_REG_TX_BUFFER, 3, 1, &(locdata.seq));
	// Set the length
	{
		byte e = 9;

		chip_write (DW1000_REG_TX_FCTRL, 0, 1, &e);
	}
	starttx_n_release (RAN_POLL_DONE);

    entry (RAN_POLL_DONE)

// PGTEST
getevent ();
toidle ();
locdata.seq++;
delay (1024, RAN_TPOLL);
release;
goto tpoll_exit;

	if (getevent () != DW1000_EVT_XMT) {

tpoll_more:
		toidle ();
		if (locdata.tag <= 1)
			goto tpoll_exit;

		locdata.tag--;
		delay (DW1000_TPOLL_DELAY, RAN_TPOLL);
		release;
	}
// FIXME: use WAIT4RESP

	// Get the packet's transmit time
	chip_read (DW1000_REG_TX_TIME, 0, DW1000_TSTAMP_LEN,
		locdata.tst + DW1000_TSOFF_TSP);

	// Preset the length for the next outgoing packet (FIN) now, to
	// balance the load between the two stages; note that SEQ is OK
	{
		byte e = 24;

		chip_write (DW1000_REG_TX_FCTRL, 0, 1, &e);
	}

	// Read the anchor's response
	delay (DW1000_TMOUT_ARESP, RAN_FAILURE);
	rxenable_n_release (RAN_ANCHOR);

    entry (RAN_ANCHOR)

	if (getevent () != DW1000_FRLEN_ARESP)
		goto tpoll_more;

	{
		byte e;
		chip_read (DW1000_REG_RX_BUFFER, 3, 1, &e);
		if (e != locdata.seq)
			// Pointless for sure
			goto tpoll_more;
	}

	// Save the time stamp
	chip_read (DW1000_REG_RX_TIME, 0, DW1000_TSTAMP_LEN,
		locdata.tst + DW1000_TSOFF_TRR);

	// Calculate the transmit time of FIN packet
	{
		lword t = (trr_upper + DW1000_FIN_DELAY) & ~1;
		chip_write (DW1000_REG_DX_TIME, 1, 4, (byte*)&t);
		// Now for the adjusted time to insert into the packet
		t += *(((byte*)&antdelay) + 1);
		memcpy (locdata.tst + DW1000_TSOFF_TSF + 1, &t, 4);
		*(locdata.tst + DW1000_TSOFF_TSF) = (byte)antdelay;
	}

	chip_write (DW1000_REG_TX_BUFFER, 7, 3 * DW1000_TSTAMP_LEN,
		locdata.tst + DW1000_TSOFF_TSP);

	wait (&__dw1000_v_drvprcs, RAN_FIN);
	irqenable (DW1000_IRQ_TRANSMIT);

	{
		// TXSTRT + TXDLYS
		byte b = (0x02 | 0x04);
		chip_write (DW1000_REG_SYS_CTRL, 0, 1, &b);
		// Verify not late
		chip_read (DW1000_REG_SYS_STATUS, 3, 1, &b);
		if (b & 0x08)
			goto tpoll_more;
	}

	dw1000_int_enable;

	release;
			
    entry (RAN_FIN)

	// Stop it
	if (getevent () == DW1000_EVT_XMT) {

		toidle ();
tpoll_exit:
		tosleep ();

		__dw1000_v_drvprcs = 0;
		trigger (&locdata);
		finish;
	}

    entry (RAN_FAILURE)

	goto tpoll_more;

endthread

// ============================================================================

void dw1000_start (byte md, byte rl, word ni) {
//
// This one is user-visible
//
	if (flags & DW1000_FLG_ACTIVE)
		syserror (ETOOMANY, "dw1");
	if (md >= sizeof (chconfig) / sizeof (chconfig_t))
		syserror (EREQPAR, "dw1");
	_BIS (flags, DW1000_FLG_ACTIVE);

	mode = chconfig [md];
	pan = ni;

	if (rl) {
		// The role is PEG (aka ANCHOR); FIXME: perhaps the roles
		// should be precompiled?
		_BIS (flags, DW1000_FLG_ANCHOR);
		if ((__dw1000_v_drvprcs = runthread (dw1000_anchor)) == 0)
			syserror (ERESOURCE, "dw1");
	} else
		_BIC (flags, DW1000_FLG_ANCHOR);

	initialize ();
}

void dw1000_stop () {
//
// Put the chip into sleep state
//
	if ((flags & DW1000_FLG_ACTIVE) == 0)
		return;

	_BIS (flags, DW1000_FLG_ACTIVE);

	if (__dw1000_v_drvprcs) {
		kill (__dw1000_v_drvprcs);
		__dw1000_v_drvprcs = 0;
	}

	flags = 0;

	toidle ();
	// Not sure if this will do
	tosleep ();
}

// ============================================================================

void dw1000_read (word st, const byte *junk, address val) {
//
// Sensor interface function
//
	if (val == NULL) {
		// Events, good for Tag as well
		if (flags & DW1000_FLG_LDREADY)
			proceed (st);

		// Wait for data ready
		when (&locdata, st);
			release;
	}

	if (flags & DW1000_FLG_ANCHOR) {
		if (flags & DW1000_FLG_LDREADY) {
			memcpy (val, &locdata, sizeof (locdata));
			_BIC (flags, DW1000_FLG_LDREADY);
			return;
		}
	}

	bzero (val, 2);
}

void dw1000_write (word st, const byte *junk, address val) {
//
// Actuator interface function: Tag poll
//
	if (flags & DW1000_FLG_ANCHOR)
		// Should we just ignore, or maybe hang, or error?
		return;

	if (__dw1000_v_drvprcs) {
		// Currently in progress, wait for completion 
		when (&locdata, st);
		release;
	}

	locdata.seq = (byte)(*val);
	__dw1000_v_drvprcs = runthread (dw1000_range);
}
