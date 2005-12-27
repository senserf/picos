/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define	DEFINE_RF_SETTINGS	1

#include "kernel.h"
#include "tcvphys.h"
#include "cc1100.h"

static int option (int, address);

		// Pointer to the static reception buffer
word		*zzr_buffer = NULL,

		// ... and to the dynamic transmission buffer
		*zzx_buffer = NULL,

		zzv_qevent,
		zzv_physid,
		zzv_statid,
		zzx_seed,		// For the random number generator
		zzx_backoff;		// Calculated backoff for xmitter

byte		*zzx_bptr,
		*zzr_bptr;

byte		zzv_rxoff,		// Transmitter on/off flags
		zzv_txoff,
		zzx_paylen,		// Output payload length in bytes
		zzr_buffl,		// Input buffer length (-2)
		zzr_state,		// Receiver state
		zzx_power,
		zzx_left,
		zzr_left;

/* ========================================= */

static byte patable [] = PATABLE;

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

byte cc1100_get_reg (byte addr) {

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

void cc1100_get_reg_burst (byte addr, byte *buffer, word count) {

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

static void cc1100_set_power () {

	cc1100_set_reg (CCxxx0_FREND0, FREND0 | zzx_power);
}

static void init_cc_regs () {

	const byte	*cur;

	for (cur = cc1100_rfsettings; *cur != 255; cur += 2)
		cc1100_set_reg (cur [0], cur [1]);

	// Power setting is handled separately
	cc1100_set_reg_burst (CCxxx0_PATABLE, patable, sizeof(patable));
	cc1100_set_power ();
}

static byte cc1100_status () {

	register byte val;

	while (1) {

		SPI_START;
		val = cc1100_spi_out_stat (CCxxx0_SNOP | 0x80);
		SPI_END;

		switch (val) {

			// Clean up hanging overflow/underflow states

			case CC1100_STATE_TX_UNDERFLOW:

				cc1100_strobe (CCxxx0_SFTX);
#if 0
				diag ("Hung TX_UNF");
#endif
				continue;

			case CC1100_STATE_RX_OVERFLOW:

				cc1100_strobe (CCxxx0_SFRX);
#if 0
				diag ("Hung RX_OVF");
#endif
			// Loop on transitional states until they go away

			case CC1100_STATE_CALIBRATE:
			case CC1100_STATE_SETTLING:

				continue;

			default:
				return val;
		}
	}
}

static void enter_idle () {

	while (cc1100_status () != CC1100_STATE_IDLE)
		cc1100_strobe (CCxxx0_SIDLE);
}

static void enter_rx () {

#if STAY_IN_RX

	// Recalibration is needed every once in a while, if we are allowed
	// to stay indefinitely in RX

	if (rx_rcnt_recalibrate) {
		// Time to recalibrate
		enter_idle ();
#if 0
		diag ("RECAL");
#endif
		// Reset recalibration counter
		rx_rcnt_res;
	} else {
		// Update recalibration counter
		rx_rcnt_upd;
	}
#endif	/* STAY_IN_RX */
		
	while (cc1100_status () != CC1100_STATE_RX)
		cc1100_strobe (CCxxx0_SRX);
}

int cc1100_rx_status () {

	register byte b, val;

	SPI_START;

	val = cc1100_spi_out_stat (CCxxx0_RXBYTES);

	// Get RXBYTES
	b = cc1100_spi_in ();

	SPI_END;

#if STAY_IN_RX
	if (val == CC1100_STATE_RX)
#else
	if (val == CC1100_STATE_IDLE || val == CC1100_STATE_RX)
#endif
		// The status is right, return #bytes in RX FIFO
		return (b & 0x7f);

	// Bring the status back to decency
	cc1100_status ();
#if 0
	diag ("RX ILL = %x/%x", val, b);
#endif
	return -1;
}

int cc1100_tx_status () {

	register byte b, val;

	SPI_START;

	val = cc1100_spi_out_stat (CCxxx0_TXBYTES);

	// Get TXBYTES
	b = cc1100_spi_in ();

	SPI_END;

	if (val == CC1100_STATE_TX)
		// We need the number of free bytes
		return 64 - (b & 0x7f);

	cc1100_status ();
#if 0
	diag ("TX ILL = %x/%x", val, b);
#endif
	return -1;
}

static void cc1100_power_down () {

#if 0
	diag ("CC1100 POWER DOWN");
#endif
	enter_idle ();
	cc1100_strobe (CCxxx0_SPWD);
	STROBE_WAIT;
	cc1100_strobe (CCxxx0_SPWD);
}

static void cc1100_reset () {

	full_reset;
	init_cc_regs ();
	enter_idle ();
}

static void ini_cc1100 () {

	// Initialize the requisite pins
	ini_regs;

	cc1100_reset ();

	// Read the chip number reg and write a message to the UART

#if DIAG_MESSAGES

	diag ("CC1100 initialized: [%x] %x",
		(cc1100_get_reg (CCxxx0_SYNC1)   << 8) |
		 cc1100_get_reg (CCxxx0_SYNC0)		,
		(cc1100_get_reg (CCxxx0_VERSION) << 8) |
		 cc1100_get_reg (CCxxx0_MCSM1)		);
#endif

#if 1
	// Validate registers
	{
		const byte *cur;
		byte val;

		// Validate register settings
		for (cur = cc1100_rfsettings; *cur != 255; cur += 2) {
			val = cc1100_get_reg (cur [0]);
			if (val != cur [1]) {
				diag ("Register check failed: [%d] == %x != %x",
					cur [0], val, cur [1]);
				syserror (EHARDWARE, "CC1100 reg");
			}
		}
	}
#endif
	// We start in the OFF state
	cc1100_power_down ();
}

void phys_cc1100 (int phy, int mbs) {

	if (zzr_buffer != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "phys_cc1100");

	if (mbs < 4 || mbs > 255) {
		if (mbs == 0)
			mbs = RADIO_DEF_BUF_LEN;
		else
			syserror (EREQPAR, "phys_cc1100 mbs");
	}

	// Account for the two status bytes

	if ((zzr_buffer = umalloc (mbs + 2)) == NULL)
		syserror (EMALLOC, "phys_cc1100");

	zzr_buffl = (byte) mbs;		// buffer length in bytes (-2)
	zzv_statid = 0;
	zzv_physid = phy;
	zzx_backoff = 0;
	zzx_seed = 12345;
	zzr_state = 0;

	/* Register the phy */
	zzv_qevent = tcvphy_reg (phy, option, INFO_PHYS_CC1100);

	/* Both parts are initially active */
	LEDI (1, 0);
	LEDI (2, 0);
	LEDI (3, 0);

	// Things start in the off state
	zzv_rxoff = zzv_txoff = 1;
	// Default power corresponds to the center == 0dBm
	zzx_power = 3;
	/* Initialize the device */
	ini_cc1100 ();
}

static word clear_to_send () {

	// I am inclined to redo this (in the next version of the driver) and
	// handle everything by hand. That is, unless I figure out how to set
	// the RSSI threshold.
#if 0
	byte stat;

	stat = cc1100_status ();
	diag ("OLD STATUS: %x", stat);
#else
	// Make sure our status is sane
	cc1100_status ();
#endif
	// CCA will make this command ineffective if we are receiving a packet
	// or RSSI is above threshold. How to set this threshold, I still have
	// no clue. So I have disabled that part, as it appears to be overly
	// sensitive. See CHANNEL_CLEAR_ON_RSSI.

	cc1100_strobe (CCxxx0_STX);
#if 0
	stat = cc1100_status ();
	diag ("NEW STATUS: %x", stat);
	return (stat == CC1100_STATE_TX);
#else
	// We succeed if we have entered TX
	return (cc1100_status () == CC1100_STATE_TX);
#endif

}

procname (cc1100_receiver);

#define	XM_LOOP		0
#define	XM_LBT		1
#define	XM_SEND		2
#define	XM_WAIT		3

process (cc1100_xmitter, void)

    int stat, stln; 

    entry (XM_LOOP)

	if (zzv_txoff) {
		/* We are off */
		if (zzv_txoff == 3) {
Drain:
			tcvphy_erase (zzv_physid);
			wait (zzv_qevent, XM_LOOP);
			release;
		} else if (zzv_txoff == 1) {
			/* Queue held, transmitter off */
			zzx_backoff = 0;
			if (!running (cc1100_receiver))
				cc1100_power_down ();
			finish;
		}
	}

	if ((stln = tcvphy_top (zzv_physid)) == 0) {
		/* Packet queue is empty */
		if (zzv_txoff == 2) {
			/* Draining; stop xmt if the output queue is empty */
			if (!running (cc1100_receiver))
				cc1100_power_down ();
			zzv_txoff = 3;
			/* Redo */
			goto Drain;
		}
		wait (zzv_qevent, XM_LOOP);
		release;
	}

	if (zzx_backoff && stln < 2) {
		/* We have to wait and the packet is not urgent */
		delay (zzx_backoff, XM_LOOP);
		zzx_backoff = 0;
		wait (zzv_qevent, XM_LOOP);
		release;
	}

	if ((zzx_buffer = tcvphy_get (zzv_physid, &stln)) == NULL)
		// Last time to check
		proceed (XM_LOOP);

	if (stln > zzr_buffl) {
		// A precaution
#if 0
		diag ("TCV packet too long: %d", stln);
#endif
		tcvphy_end (zzx_buffer);
		zzx_buffer = NULL;
		proceed (XM_LOOP);
	}

	// Set the station Id
	zzx_buffer [0] = zzv_statid;

	zzx_paylen = (byte) stln;

	// zzx_buffer being set enables the receiver to detect that a received
	// packet matches the outgoing one. We will implement this later, but
	// the hooks are here already. To cancel the transmission, the receiver
	// will do tcvphy_end, set zzx_buffer to NULL, and trigger qevent.

    entry (XM_LBT)

	if (zzx_buffer == NULL)
		// Released by the receiver
		proceed (XM_LOOP);

	if (zzv_txoff) {
		tcvphy_end (zzx_buffer);
		zzx_buffer = NULL;
		proceed (XM_LOOP);
	}

	// Check if the receiver isn't holding the device
	rcv_disable_int;
	if (rcv_busy) {
		rcv_restore_int;
#if 0
		diag ("XMT LCK");
#endif
		gbackoff;
		delay (zzx_backoff, XM_LBT);
		zzx_backoff = 0;
		wait (zzv_qevent, XM_LBT);
		release;
	}

	lock_xmt;

	if (clear_to_send () == NO) {
		// CCA says NO
		unlock_xmt;
		rcv_restore_int;
#if 0
		diag ("LBT BACKOFF");
#endif
		gbackoff;
		delay (zzx_backoff, XM_LBT);
		zzx_backoff = 0;
		wait (zzv_qevent, XM_LBT);
		release;
	}

	// We have the channel and XMT is running
	LEDI (2, 1);

	// Send the length byte
	cc1100_set_reg (CCxxx0_TXFIFO, zzx_paylen);

	zzx_left = zzx_paylen;
	zzx_bptr = (byte*) zzx_buffer;

    entry (XM_SEND)

	while (zzx_left) {

		stat = cc1100_tx_status ();
		if (stat < 0) {
			// Underflow - we will redo the same packet
			enter_idle ();
#if 0
			diag ("Xmit UNDERFLOW");
#endif
			goto SkipForNow;
		}

		if (stat == 0) {
#if BUSY_TRANSMIT
			continue;
#else
			// Retry in a couple of msec
			delay (SKIP_XMT_DELAY, XM_SEND);
			release;
#endif
		}

		if (stat > zzx_left)
			stat = zzx_left;

		cc1100_set_reg_burst (CCxxx0_TXFIFO, zzx_bptr, stat);

		zzx_bptr += stat;
		zzx_left -= stat;
	}

	// We are done. Just hang on until the transmission ends.
	tcvphy_end (zzx_buffer);

SkipForNow:

	zzx_buffer = NULL;

    entry (XM_WAIT)
	
	if ((stat = cc1100_status ()) == CC1100_STATE_TX) {
#if 0
		diag ("STILL SENDING");
#endif
		delay (TXEND_POLL_DELAY, XM_WAIT);
		release;
	}
#if 0
	diag ("EOT STATUS: %x", stat);
#endif

	LEDI (2, 0);

	// Before unlocking, check if the receiver is still around
	if (zzv_rxoff) {
		// Return to idle
		if (stat != CC1100_STATE_IDLE)
			enter_idle ();
	} else {

#if STAY_IN_RX
		// Reset counter to recalibration
		rx_rcnt_res;
		// We are set to recalibrate on this transtition (we should be
		// IDLE at this moment)
		enter_rx ();
		if (rcv_istate != RCV_IGN) {
			// Receiver waiting for a packet: reset it
			set_rcv_istate (RCV_STA);
			rcv_enable_int;
		}
#else
		if (rcv_istate != RCV_IGN) {
			enter_rx ();
			set_rcv_istate (RCV_STA);
			rcv_enable_int;
		} else {
			enter_idle ();
		}
#endif
	}

	unlock_xmt;
	trigger (rxunlock);

	delay (MIN_BACKOFF, XM_LOOP);
	release;

    nodata;

endprocess (1)

#define	RCV_GETIT		0
#define	RCV_CHECKIT		1
#define	RCV_RECEIVE		2

process (cc1100_receiver, void)

    int len;

    entry (RCV_GETIT)

	if (xmt_busy) {
#if 0
		diag ("RC locked");
#endif
		wait (rxunlock, RCV_GETIT);
		release;
	}

	if (zzv_rxoff) {
#if 0
		diag ("RX closing");
#endif
		set_rcv_istate (RCV_IGN);

		if (zzv_txoff)
			cc1100_power_down ();
		else
			enter_idle ();
		finish;
	}

	zzr_bptr = NULL;

	wait (rxevent, RCV_CHECKIT);
	set_rcv_istate (RCV_STA);

#if STAY_IN_RX == 0
	// Need to enter RX explicitly
	enter_rx ();
#endif
	rcv_enable_int;
	release;

    entry (RCV_CHECKIT)

	rcv_disable_int;
	set_rcv_istate (RCV_IGN);

	if (zzv_rxoff) {
		unlock_rcv;
		LEDI (3, 0);
		proceed (RCV_GETIT);
	}

	if (zzr_bptr == NULL) {
		unlock_rcv;
		LEDI (3, 0);
#if 0
		diag ("RCV failure %d", zzr_left);
#endif

#if STAY_IN_RX
		if (!xmt_busy)
			enter_rx ();
#endif
		proceed (RCV_GETIT);
	}

	unlock_rcv;

	len = zzr_bptr - (byte*) zzr_buffer;

	add_entropy (*(zzr_bptr-1) ^ *(zzr_bptr-2));

	// Check if the packet is OK

	zzr_left = *(zzr_bptr-1);
	if (zzr_left & 0x80) {
		// CRC OK
		*(zzr_bptr-1) = *(zzr_bptr-2);
		// Reverse the location of RSSI to be compatible with DM2100
		*(zzr_bptr-2) = (zzr_left & 0x7f);
	} else {
		// Ignore
		proceed (RCV_GETIT);
	}

	gbackoff;

#if 0
	diag ("RCV: %d %x %x %x %x %x %x", len,
		zzr_buffer [0],
		zzr_buffer [1],
		zzr_buffer [2],
		zzr_buffer [3],
		zzr_buffer [4],
		zzr_buffer [5]);
#endif
	/* Check the station Id */
	if (zzv_statid != 0 && zzr_buffer [0] != 0 &&
	    zzr_buffer [0] != zzv_statid)
		/* Wrong packet */
		proceed (RCV_GETIT);

	tcvphy_rcv (zzv_physid, zzr_buffer, len);

#if STAY_IN_RX
	if (rx_rcnt_recalibrate) 
		enter_rx ();
	else
		rx_rcnt_upd;
#endif
	proceed (RCV_GETIT);

    nodata;

endprocess (1)

static int option (int opt, address val) {
/*
 * Option processing
 */
	int ret = 0;

	switch (opt) {

	    case PHYSOPT_STATUS:

		ret = ((zzv_txoff == 0) << 1) | (zzv_rxoff == 0);
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_TXON:

		if (zzv_rxoff)
			// Start up
			cc1100_reset ();

		zzv_txoff = 0;

		if (zzv_rxoff)
			LEDI (1, 1);
		else
			LEDI (1, 2);

		if (!running (cc1100_xmitter))
			fork (cc1100_xmitter, NULL);
		trigger (zzv_qevent);
		break;

	    case PHYSOPT_RXON:

		if (zzv_txoff)
			// Start up
			cc1100_reset ();

		zzv_rxoff = 0;

		if (zzv_txoff)
			LEDI (1, 1);
		else
			LEDI (1, 2);

		if (!running (cc1100_receiver))
			fork (cc1100_receiver, NULL);
#if STAY_IN_RX
		if (!xmt_busy)
			enter_rx ();
#endif
		break;

	    case PHYSOPT_TXOFF:

		/* Drain */
		zzv_txoff = 2;
		if (zzv_rxoff)
			LEDI (1, 0);
		else
			LEDI (1, 1);
		trigger (zzv_qevent);
		break;

	    case PHYSOPT_TXHOLD:

		zzv_txoff = 1;
		if (zzv_rxoff)
			LEDI (1, 0);
		else
			LEDI (1, 1);
		trigger (zzv_qevent);
		break;

	    case PHYSOPT_RXOFF:

		zzv_rxoff = 1;
		if (zzv_txoff)
			LEDI (1, 0);
		else
			LEDI (1, 1);
		trigger (rxevent);
		break;

	    case PHYSOPT_CAV:

		/* Force an explicit backoff */
		if (val == NULL)
			zzx_backoff = 0;
		else
			zzx_backoff = *val;
		trigger (zzv_qevent);
		break;

	    case PHYSOPT_SETPOWER:

		if (val == NULL)
			// Default
			zzx_power = 3;
		else if (*val > 7)
			zzx_power = 7;
		else
			zzx_power = *val;
		cc1100_set_power ();

		break;

	    case PHYSOPT_GETPOWER:

		ret = (int) zzx_power;
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_SETSID:

		zzv_statid = (val == NULL) ? 0 : *val;
		break;

            case PHYSOPT_GETSID:

		ret = (int) zzv_statid;
		if (val != NULL)
			*val = ret;
		break;

	    default:

		syserror (EREQPAR, "phys_cc1100 option");

	}
	return ret;
}
