/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2015                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "kernel.h"
#include "tcvphys.h"

#define	DW1000_DEFINE_RF_SETTINGS
#include "dw1000.h"
#endif

#ifndef DW1000_SPI_WAIT
#define	DW1000_SPI_WAIT	CNOP
#endif

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
			return
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

// This is the default mode; it is resettable with the RESET physopt
static chconfig_t mode = chconfig [0];
static byte role = DW1000_ROLE_TAG;

// ============================================================================
// ============================================================================

#if 0

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

static void dw1000_init () {
//
// After startup
//
	word w;
	lword lw, cf;

	DW1000_RESET;	// Hard reset

	// Before configuring PLL, the SPI rate is not supposed to exceed 3M
	// I am not sure how fast we can ever get, probably not much faster
	// than that, so let us just ignore this little issue and keep our
	// fingers crossed

	// The reference driver sets the clocks to XTI for reading OTP, it says
	// that otherwise the operation is unreliable; I think that this
	// amounts to setting the LSB of PMSC CTRL0 to 0x01
	w = 0x01;
	chip_write (DW1000_REG_PMSC, 0, 1, (byte*)&w);

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
		w |= DW1000_LLDO;
	chip_write (DW1000_REG_AON, 0, 2, (byte*)w);

	// Enable sleep with wake on CS; not sure if the SLEEP_EN bit should be
	// set now; apparently, it doesn't put the device to sleep; maybe we
	// should move this to the end of initialization?
	w = DW1000_CF0_SLEEP_EN | DW1000_CF0_WAKE_SPI;
	chip_write (DW1000_REG_AON, 6, 1, (byte*)&w);

	// Crystal trim
	if ((w = (word) read_otpm (DW1000_ADDR_XTRIM) & 0x1F) == 0)
		// No calibration value stored, use the default mid range, as
		// in the reference driver
		w = 0x10;
	// Write the trim or'red with the stuff that goes to the upper bits of
	// the byte; the RD says: bits 7:5 must always be set to binary “011”.
	// Failure to maintain this value will result in DW1000 malfunction.
	chip_write (DW1000_REG_FS_CTRL, 0x0e, 1, (byte*)&w);

	// Set the config register to the default, then prepare it and write
	// once
	cf = DW1000_CF_HIRQ_POL | DW1000_CF_DIS_DRXB;

	// Set EUI to be all ones except for the last word, which is
	// equal to Host Id
	lw = 0;
	chip_write (DW1000_REG_EUI, 0, 4, (byte*)&lw);
	lw = (word)host_id;
	chip_write (DW1000_REG_EUI, 4, 4, (byte*)&lw);

	// Set PAN to network Id
	


	if (role == DW1000_ROLE_TAG)

	// TX config (power); this is an array of lw power entries, two entries
	// per channel starting at 1, first entry for PRF 16M, the other for
	// 64M
	lw = read_otpm (DW1000_ADDR_TXCONF + (mode.channel ? 8 : 2) + mode.prf);
	if (lw == 0 || lw == MAX_LWORD)
		// Absent
		lw = dw1000_def_txpower;

	if (dw1000_use_smartpower) {


###here shouldn't we first figure out how to set the whole 32-bit SYS_CFG
###register?
###peg should run without frame filtering, because the tags don't know it;
###for now we do something simple, the actual algorithm is going to be
###complicate (if we get there at all)
	
	
	

	// XTRIM, TX Config ...
		








	// Configuration notes:
	//
	//	Set WAKE_SPI in AON_CFG0 to enable wake by SPI CS down (500us)
	//	Set WAKE_PIN to enable pin wake (no need to use it)
	//	Clear WAKE_CNT to disable autowake on internal timer
	//
	//	Set ONW_LDC i AON_WCFG to make sure that configuration is
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

	
	



}

