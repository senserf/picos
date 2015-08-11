/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2015                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "kernel.h"
#include "tcvphys.h"

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

static void chip_write (byte reg, word index, word length, byte *stuff) {
//
// Write
//
	chip_trans (reg | 0x80, index);
	while (length--)
		spi_out (*stuff++);
	DW1000_SPI_STOP;
}

static void chip_read (byte reg, word index, word length, byte *stuff) {
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
		netid = 0;		// PAN

// This is the default mode; resettable with the RESET physopt
static chconfig_t mode;
static byte role = DW1000_ROLE_TAG;

// The thread
sint __dw1000_v_drvprcs;

// ============================================================================
// ============================================================================

#if (DW1000_OPTIONS & 0x0001)

static lword dw1000_device_id () {

	lword d;

	chip_read (DW1000_REG_DEVID, 0, 4, (byte*)&d);
	return d;
}

#endif

static void reset_rx () {

	byte c;

	c = 0xe0;
	chip_write (DW1000_REG_PMSC, 3, 1, &c);
	c = 0xf0;
	chip_write (DW1000_REG_PMSC, 3, 1, &c);
}

static lword read_otpm (word addr) {
//
// Reads 4 bytes from OTP memory
//
	lword res;

	chip_write (DW1000_REG_OTPC, 4, 2, (byte*)&addr);
	addr = 0x03;	// Manual drive OTP_READ
	chip_write (DW1000_REG_OTPC, 6, 1, (byte*)&addr);
	addr = 0x00;
	chip_write (DW1000_REG_OTPC, 6, 1, (byte*)&addr);
	chip_read (DW1000_REG_OTPC, 10, 4, (byte*)&res);
	return res;
}

static void enter_sleep () {

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

static void initialize () {
//
// After startup
//
	word w, v;
	byte a, b;
	lword lw, cf;

	if (__dw1000_v_drvprcs) {
		// Kill the previous thread
		kill (__dw1000_v_drvprcs);
		__dw1000_v_drvprcs = 0;
	}

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
	lw = dw1000_device_id ();
	diag ("DWINIT: %x%x", (word)(lw >> 16), (word) lw);
#endif

	// Read and preserve antenna delay; will be set manually on every wake,
	// so we just make sure it is handy
	lw = read_otpm (DW1000_ADDR_ANTDELAY);
	// Determine the actual delay to use
	if ((antdelay = (word)((mode.prf ? (lw >> 16) : lw) & 0xffff)) == 0)
		// Use some default delay as per reference driver
		antdelay = dw1000_def_rfdelay (mode.prf);

	// AON wakeup configuration; L64P is only set for 64B preamble, which
	// we don't use; not sure if PRES_SLEEP is needed at this stage
	w = DW1000_ONW_LDC | DW1000_ONW_LLDE | DW1000_ONW_PRES_SLEEP;
	if ((read_otpm (DW1000_ADDR_LDOTUNE) & 0xff) != 0)
		w |= DW1000_ONW_LLDO;
	chip_write (DW1000_REG_AON, 0, 2, (byte*)w);

	// Enable sleep with wake on CS; not sure if the SLEEP_EN bit should be
	// set now; apparently, it doesn't put the device to sleep; maybe we
	// should move this to the end of initialization?
	b = DW1000_CF0_SLEEP_EN | DW1000_CF0_WAKE_SPI;
	chip_write (DW1000_REG_AON, 6, 1, &b);

	// Crystal trim
	if ((b = (byte) read_otpm (DW1000_ADDR_XTRIM) & 0x1F) == 0)
		// No calibration value stored, use the default mid range, as
		// in the reference driver
		b = 0x10;
	// Write the trim or'red with the stuff that goes to the upper bits of
	// the byte; the RD says: bits 7:5 must always be set to binary “011”.
	// Failure to maintain this value will result in DW1000 malfunction.
	chip_read (DW1000_REG_FS_CTRL, 0x0e, 1, &a);
	b = (a & ~0x1f) | (b & 0x1f);
	chip_write (DW1000_REG_FS_CTRL, 0x0e, 1, &b);

	// Set the config register to the default, then prepare it and write
	// once
	cf = DW1000_CF_HIRQ_POL | DW1000_CF_DIS_DRXB | DW1000_CF_FFEN |
		// Auto RX re-enable, so we won't have to handle incorrect
		// frames by hand
		DW1000_CF_RXAUTR;

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
	lw = ((word)host_id) | (((lword)netid) << 16);
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
	_BIS (cf, role == DW1000_ROLE_TAG ? DW1000_CF_FFAD : DW1000_CF_FFBC);

	// Enable clocks normal; note: the reference driver does some weird
	// things to the apparently unused bits of the PMSC_CTRL0, my shortcut
	// is to write zero to the entire low word
	w = 0x0000;
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
	chip_write (DW1000_REG_FS_CTRL, 7, 5, dw1000_pll2_config);

	b = DW1000_PLL2_CALCFG;
	chip_write (DW1000_REG_FS_CTRL, 0x0e, 1, &b);

	// RF RX blocks
	b = DW1000_RX_CONFIG;
	chip_write (DW1000_REG_RF_CONF, 0x0b, 1, &b);

	lw = dw1000_rf_txctrl;
	chip_write (DW1000_REG_RF_CONF, 0x0c, 4, (byte*)&lw);

	// Baseband parameters
	w = dw1000_sftsh;
	chip_write (DW1000_REG_DRX_CONF, 0x02, 2 (byte*)&w);

	w = dw1000_dtune1;
	chip_write (DW1000_REG_DRX_CONF, 0x04, 2 (byte*)&w);

	// dtune1b
	if (mode.datarate) {
		// 6M8: dtune1b; we don't use preamble length of 64
		b = 0x28;
		chip_write (DW1000_REG_DRX_CONF, 0x26, 1, &b);
		w = 0x20;
	} else {
		w = 0x64;
	}
	chip_write (DW1000_REG_DRX_CONF, 0x06, 2 (byte*)&w);

	// dtune2
	lw = dw1000_dtune2;
	chip_write (DW1000_REG_DRX_CONF, 0x08, 4, (byte*)&lw);

	// dtune3
	w = cc1000_sfdto;
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

	chip_write (DW1000_REG_CTRL, 0, 4, (byte*)&lw);

	// Preamble size, TX PRF, ranging bit
	lw = (((lword)(dw1000_preamble | dw1000_prf)) << 16) | 0x00008000 |
		(((lword)dw1000_datarate) << 13);

	if (role == DW1000_ROLE_PEG)
		// This is the fixed length of the anchor's response; there are
		// two different lengths: 9 and 24 (including FCS) for a Tag
		lw |= 11;

	chip_write (DW1000_REG_TX_FCTRL, 0, 4, (byte*)&lw);

	// Write RX antenna delay, TX delay will be written on wakeup
	chip_write (DW1000_REG_LDE_IF, 0x1804, 2, (byte*)antdelay);

	// ====================================================================

	// Preset the transmit buffer: this is common for both modes (PAN
	// follows the sequence number)
	chip_write (DW1000_REG_TX_BUFFER, 3, 2, (byte*)&netid);
	if (role == DW1000_ROLE_TAG) {
		// Frame control bytes are 01 80 (no destination address,
		// short source address, frame type = data; the buffer
		// layout is: 01 80 SQ PAN PAN SRC SRC ...; the length is
		// 7 (for the initial poll) and 22 (for the fin message which
		// additionally includes 3 * 5 = 15 bytes of timestamps); note
		// that the stored length must include two extra bytes for FCS
		w = 0x8001;
		// Position of source address
		v = 5;
	} else {
		// Frame control bytes are 41 88 (short destination address,
		// short source address, pan compression, frame type = data
		w = 0x8841;
		v = 7;
	}
	chip_write (DW1000_REG_TX_BUFFER, 0, 2, (byte*)&w);
	chip_write (DW1000_REG_TX_BUFFER, v, 2, (byte*)&host_id);

	// ====================================================================
	
	// Enter sleep
	enter_sleep ();
}

static void wakeup () {
//
// Wakes up the chip from lp mode; note: for now we spin like crazy; this may
// have to be reorganized into an FSM later
//
	word w;

	SPI_START;
	udelay (250);
	SPI_STOP;

	for (w = 0; w < 50; w++) {
		mdelay (1);
		if (dw1000_ready)
			goto Ready;
	}

	syserror (EHARDWARE, "dw1");

	// After going through all those pains to detect when the chip gets up,
	// they delay for about this much to "stabilize the crystal"
	mdelay (90);

	// Now we have to reload the antenna delay; this makes me wonder why
	// bother; can't we just account for it in the formula?
	chip_write (DW1000_REG_TX_ANTD, 0, 2, (byte*)&antdelay);

	// Should we put some test here to make sure we are actually alive?
}

static byte getevent () {
//
// Retrieves and clears the interrupt status
//
	lword status;
	byte len;

	chip_read (DW1000_REG_SYS_STATUS, 0, 4, (byte*)&status);

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
		    (DW1000_IRQ_RXPHD | DW1000_IRQ_RXSFDD))
			// Bad frame
			goto Bad;
	}

	// If we check this before TX, then we may be OK for RX forcibly
	// following TX (if we ever go for this option), if the mask ignores
	// TX, but includes RX
	if (status & DW1000_IRQ_RXFCG) {
		// Receiver FCS OK
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
		// This returns the chip to IDLE
		len = 0x40;
		chip_write (DW1000_REG_SYS_CTRL, 0, 1, &len);
		len = DW1000_EVT_BAD;
		goto Rtn;
	}

	// None
	return DW1000_EVT_NIL;
}



		
		
		
	
	

static void irqenable (lword mask, word st) {

	

static void rxenable (word rxst, word timeout) {
//
// Enable RX with optional timeout
//
	

	



// ============================================================================
// The anchor process
// ============================================================================

#define	DWA_INIT	0

thread (dw1000_anchor)

    entry (DWA_INIT)

	wakeup ();

    entry (DWA_WAITEVENT)

	// race or anything like that
	wait (&__dw1000_v_drvprcs, DWA_EVENT);
	// This also sets the IRQ mask
	rxenable ();
	dw1000_int_enable;
	release;

    entry (DWA_EVENT)

	





	

	st = getevent ();

	if (st == DW1000_EVT_NIL || st == DW1000_EVT_BAD) {
		// Nothing; we are in IDLE when we get here, so there is no interrupt



	// For now, we operate in a single-buffer mode, so RX must be
	// explicitly enabled on every turn
	enter_rx ();

search rxenable









endthread

// ============================================================================

void dw1000_start (byte md, byte rl, word ni) {
//
// This one is user-visible
//
	if (md >= sizeof (chconfig) / sizeof (chconfig_t))
		syserror (EREQPAR, "dw1");

	mode = chconfig [md];
	netid = ni;

	if (rl) {
		// The role is PEG (aka ANCHOR)
		role = DW1000_ROLE_PEG;
		__dw1000_v_drvprcs = runthread (dw1000_anchor);
	}

	initialize ();
}

void dw1000_listen () {
//
// Sets the driver in "anchor" mode, as they call it, i.e., the device will
// listen for a ranging packet from any tag and follow up with the proper
// handshake
//





}

void dw1000_range () {


}



#if 0

- enable interrupts (formally) NO, later, as needed
- write CF OK
- set clocks to normal? OK
- enter sleep (after sleep TX for Tag and RX for Peg) NO, manually for now






	//
	//	Set WAKE_SPI in AON_CFG0 to enable wake by SPI CS down (500us)
	//	Set WAKE_PIN to enable pin wake (no need to use it)
	//	Clear WAKE_CNT to disable autowake on internal timer
	//
	//	Set ONW_LDC in AON_WCFG to make sure that configuration is
	//	saved and restored of sleep/wake
	//
	//	Set ONW_LLDO in AON_WCF only if LDOTUNE_CAL from OTP reads
	//	nonzero

	//








	// Now, we start the device in (regular, non-deep) sleep. We will be
	// keeping it in this state when idle. No need to use the deep sleep,
	// because, at least for now, we can live with the 1uA (or so) current
	// drain.

	conf = chconfig + mode;

	// Bits to be set in AON_WCFG
	w = DW1000_ONW_LDC;
	if ((read_otpm (DW1000_ADDR_LDOTUNE) & 0xff) != 0)
		w |= DW1000_ONW_LLDO;

#endif

void phys_dw1000 (int phy, int mbs) {

	// The default mode
}
