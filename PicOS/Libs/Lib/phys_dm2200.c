/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "tcvphys.h"
#include "dm2200.h"

static int option (int, address);

word		*zzr_buffer = NULL,
		*zzr_buffp,	// Pointer to next buffer word; also used to
				// indicate that a reception is pending
		*zzr_buffl,	// Pointer to LWA+1 of buffer area
		*zzx_buffer = NULL, // Pointer to dynamic transmission buffer
		*zzx_buffp,	// Next buffer word
		*zzx_buffl,	// LWA+1 of xmit buffer
		zzv_qevent,
		zzv_physid,
		zzv_statid,
		zzx_backoff;	// Calculated backoff for xmitter

byte		zzv_curbit,	// Current bit index
		zzv_prmble,	// Preamble counter
		zzv_status,
		zzr_length,	// Length of received packet in words; this
				// is set after a complete reception
		zzv_curnib,	// Current nibble
		zzv_cursym,	// Symbol buffer
		zzv_rxoff,	// Transmitter on/off flags
		zzv_txoff,
		zzr_rcvmode = DM2200_DEF_RCVMODE,
 		zzv_istate = IRQ_OFF;

const byte zzv_symtable [] = {
		255,		//  000000
		255,		//  000001
		255,		//  000010
		255,		//  000011
		255,		//  000100
		255,		//  000101
		255,		//  000110
		255,		//  000111
		255,		//  001000
		255,		//  001001
		255,		//  001010
		128,		//  001011	(SV2, EOP)
		255,		//  001100
		  0,		//  001101
		  1,		//  001110
		255,		//  001111
		255,		//  010000
		255,		//  010001
		255,		//  010010
		  2,		//  010011
		255,		//  010100
		  3,            //  010101
		  4,            //  010110
		255,		//  010111
		255,		//  011000
		  5,		//  011001
		  6,		//  011010
		255,		//  011011
		  7,		//  011100
		255,		//  011101
		255,		//  011110
		255,		//  011111
		255,		//  100000
		255,		//  100001
		129,		//  100010	(SV3)
		  8,		//  100011
		255,		//  100100
		  9,		//  100101
		 10,		//  100110
		255,		//  100111
		255,		//  101000
		 11,		//  101001
		 12,		//  101010
		255,		//  101011
		 13,		//  101100
		255,		//  101101
		130,		//  101110	(SV1)
		255,		//  101111
		255,		//  110000
		255,		//  110001
		 14,		//  110010
		255,		//  110011
		 15,		//  110100
		255,		//  110101
		255,		//  110110
		255,		//  110111
		255,		//  111000
		255,		//  111001
		255,		//  111010
		255,		//  111011
		255,		//  111100
		255,		//  111101
		255,		//  111110
		255		//  111111
	};

const byte zzv_nibtable [] = {
		0x05,		// 001101 ->   1100 -> 000000000101
		0x09,		// 001110 ->    120 -> 000000001001
		0x50,		// 010011 ->   0011 -> 000001010000
		0x00,		// 010101 -> 000000 -> 000000000000
		0x40,		// 010110 ->  00010 -> 000001000000
		0x14,		// 011001 ->   0110 -> 000000010100
		0x04,		// 011010 ->  01000 -> 000000000100
		0x18,		// 011100 ->    021 -> 000000011000
		0x18,		// 100011 ->    021 -> 000000011000
		0x04,		// 100101 ->  01000 -> 000000000100
		0x14,		// 100110 ->   0110 -> 000000010100
		0x40,		// 101001 ->  00010 -> 000001000000
		0x00,		// 101010 -> 000000 -> 000000000000
		0x50,		// 101100 ->   0011 -> 000001010000
		0x05,		// 110010 ->   1100 -> 000000000101
		0x41		// 110100 ->   1001 -> 000001000001
	};

const byte zzv_srntable [] = {
		3, 2, 3, 5, 4, 3, 4, 2, 2, 4, 3, 4, 5, 3, 3, 3
	};

/* ========================================= */

#include "checksum.h"

static void dm2200_wreg (byte reg, byte val) {

	word i;

	ser_out;
	cfg_up;

	ser_down;	// Write bit
	ser_clk_up;
	ser_clk_down;

	if ((reg & 2))	// First bit of register address
		ser_up;
	ser_clk_up;
	ser_clk_down;

	if ((reg & 1))	// Second bit of register address
		ser_up;
	else
		ser_down;
	ser_clk_up;	
	ser_clk_down;

	i = 7;
	while (1) {
		if ((val & (1 << i)))
			ser_up;
		else
			ser_down;
		ser_clk_up;	
		ser_clk_down;
		if (i == 0)
			break;
		i--;
	};

	cfg_down;
	ser_down;
}

static byte dm2200_rreg (byte reg) {

	word i;
	byte v;

	ser_out;
	cfg_up;

	ser_up;		// Read bit
	ser_clk_up;
	ser_clk_down;

	if ((reg & 2) == 0)	// First bit of register address
		ser_down;
	ser_clk_up;
	ser_clk_down;

	if ((reg & 1))		// Second bit of register address
		ser_up;
	else
		ser_down;
	ser_clk_up;	
	ser_clk_down;

	ser_in;

	i = 7;
	v = 0;

	while (1) {
		ser_clk_up;	
		ser_clk_down;
		if (ser_data)
			v |= (1 << i);
		if (i == 0)
			break;
		i--;
	}

	cfg_down;
	return v;
}

sprocname (rcvradio);

static void ini_dm2200 (void) {
/*
 * Initialize the device
 */
	byte r0, r1, r2;

	ini_regs;
	timer_init;

	dm2200_wreg (0, CFG0_OFF);
	dm2200_wreg (1, CFG1);
	dm2200_wreg (2, LOSYN);

	// Sanity check
	r0 = dm2200_rreg (0);
	r1 = dm2200_rreg (1);
	r2 = dm2200_rreg (2);
	if (r0 != CFG0_OFF || r2 != LOSYN) {
		// r1 has a bunch of R/O bits with random values
		diag ("DM2200 initialization failed: %x, %x, %x", r0, r1, r2);
		syserror (EHARDWARE, "phys_dm2200 init");
	}

	diag ("DM2200 initialized: %x, %x, %x", r0, r1, r2);

#if FCC_TEST_MODE
	if (fcc_test_send) {
		diag ("DM2200 FCC test mode, sending 0101010101010 ....");
		LEDI (0, 2);
		hstat (HSTAT_XMT);
		start_xmt;
	} else {
		diag ("DM2200 FCC test mode, listening .....");
		zzv_rxoff = 0;
		runthread (rcvradio);
		trigger (rxevent);
		LEDI (1, 2);
		LEDI (2, 0);
	}
#endif

}

#if FCC_TEST_MODE == 0 && GLACIER == 0
static
#endif
void hstat (word status) {
/*
 * Change chip status: xmt, rcv, off
 */
	switch (status) {

		case HSTAT_SLEEP:

			rssi_off;
			adc_disable;
			dm2200_wreg (0, CFG0_OFF);
			break;

		case HSTAT_RCV:

			rssi_on;
			dm2200_wreg (0, zzr_rcvmode & 0x7f);
			break;

		default:
			rssi_off;
			adc_disable;
			dm2200_wreg (0, CFG0_XMT);
			udelay (30);
	}
}
		
static byte rssi_cnv (word v) {
/*
 * Converts the RSSI to a single byte 0-255
 */
	add_entropy (v);

#if RSSI_MIN >= 0x8000
	// RSSI is signed
	if ((v & RSSI_MIN))
		v |= RSSI_MIN;
#endif

#if RSSI_MIN != 0
	v -= RSSI_MIN;
#endif
	return (byte) (((int) v - (int) RSSI_MIN) >> RSSI_SHF);
}

#include "xcvcommon.h"

void phys_dm2200 (int phy, int mbs) {
/*
 * phy  - interface number
 * mbs  - maximum packet length (including checksum, must be divisible by 4)
 */
	if (zzr_buffer != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "phys_dm2200");

	if (mbs < 8 || mbs & 0x3) {
		if (mbs == 0)
			mbs = RADIO_DEF_BUF_LEN;
		else
			syserror (EREQPAR, "phys_dm2200 mbs");
	}

	/* For reading RSSI */
	adc_config_rssi;

	if ((zzr_buffer = umalloc (mbs)) == NULL)
		syserror (EMALLOC, "phys_dm2200");

	/* This is static and will never change */
	zzr_buffl = zzr_buffer + (mbs >> 1);
	/* This also indicates that there's no pending reception */
	zzr_buffp = 0;

	zzv_status = 0;

	zzv_statid = 0;
	zzv_physid = phy;
	zzx_backoff = 0;

	/* Register the phy */
	zzv_qevent = tcvphy_reg (phy, option, INFO_PHYS_DM2200);

	/* Both parts are initially active */
	zzv_rxoff = zzv_txoff = 1;
	LEDI (0, 0);
	LEDI (1, 0);
	LEDI (2, 0);

	/* Initialize the device */
	ini_dm2200 ();

#if ENTROPY_COLLECTION
#if FCC_TEST_MODE == 0
	{
		int i;
		/* Use the RSSI to initialize entropy */
		hstat (HSTAT_RCV);
		for (i = 0; i < 8; i++) {
			adc_start;
			mdelay (8);
			adc_stop;
			adc_wait;
			add_entropy (adc_value & 0x0f);
			mdelay (1);
		}
		adc_disable;
		rssi_off;
	}
#endif
#endif
}

static int option (int opt, address val) {
/*
 * Option processing
 */
	int ret = 0;

	switch (opt) {

#include "xcvcommop.h"

	    case PHYSOPT_SENSE:

		ret = receiver_busy;
		break;

	    case PHYSOPT_GETPOWER:

		ret = ((int) rssi_cnv (adc_value)) & 0xff;

		if (val != NULL)
			*val = ret;

		break;

	    case PHYSOPT_SETMODE:

		ret = (zzr_rcvmode & 0x0E) >> 1; 
		if (val == NULL)
			zzr_rcvmode = DM2200_DEF_RCVMODE;
		else if (*val <= DM2200_N_RF_OPTIONS)
			zzr_rcvmode = (zzr_rcvmode & 0xf1) |
				(((byte) (*val)) << 1);
		else
			syserror (EREQPAR, "phys_dm2200 option rfmode");
		break;

	    case PHYSOPT_GETMODE:

		ret = (zzr_rcvmode & 0x0E) >> 1; 
		if (val != NULL)
			*val = ret;
		break;

	    default:

		syserror (EREQPAR, "phys_dm2200 option");

	}
	return ret;
}
