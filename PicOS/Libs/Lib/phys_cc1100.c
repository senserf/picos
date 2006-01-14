/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define	DEFINE_RF_SETTINGS	1

#include "kernel.h"
#include "tcvphys.h"
#include "cc1100.h"

static int option (int, address);

		// Pointer to the static reception buffer
static word	*rbuff = NULL,
		bckf_timer = 0,

		physid,
		statid,
		rnd_seed;		// For the random number generator

word		zzv_drvprcs, zzv_qevent;
byte		zzv_iack,		// To resolve interrupt race
		zzv_gwch;		// Guard watch


static byte	RxOFF,		// Transmitter on/off flags
		TxOFF,
		xpower,
		rbuffl;

#if GUARD_PROCESS

#define	WATCH_RCV	1
#define WATCH_XMT	2

#define		guard_start(f)	_BIS (zzv_gwch, (f))
#define		guard_stop(f)	_BIC (zzv_gwch, (f))

#else

#define		guard_start(f)	do { } while (0)
#define		guard_stop(f)	do { } while (0)

#endif	/* GUARD_PROCESS */

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

static void cc1100_set_power () {

	cc1100_set_reg (CCxxx0_FREND0, FREND0 | xpower);
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

	if (val == CC1100_STATE_IDLE)
		// The status is right, return #bytes in RX FIFO
		return (b & 0x7f);
#if TRACE_DRIVER
	diag ("RX ILL = %x/%x", val, b);
#endif
	return -1;
}

void cc1100_rx_flush () {

	while (1) {
		cc1100_strobe (CCxxx0_SFRX);
		if (cc1100_get_reg (CCxxx0_RXBYTES) == 0)
			return;
#if TRACE_DRIVER
		diag ("RX FLUSH LOOP");
#endif
	}
	enter_rx ();
}

static word clear_to_send () {

#if 0
	byte stat;

	stat = cc1100_status ();
	diag ("OLD STATUS: %x", stat);
#else
	// Make sure our status is sane (FIXME: try removing this)
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

static void power_down () {

#if TRACE_DRIVER
	diag ("CC1100 POWER DOWN");
#endif
	enter_idle ();
	cc1100_strobe (CCxxx0_SPWD);
	STROBE_WAIT;
	cc1100_strobe (CCxxx0_SPWD);
}

static void chip_reset () {

	full_reset;
	init_cc_regs ();
}

static void ini_cc1100 () {

	// Initialize the requisite pins
	ini_regs;

	chip_reset ();
	power_down ();

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
}

#if CRC_MODE > 1
#include "checksum.h"
#endif

#define	DR_LOOP		0
#define	DR_SWAIT	1

process (cc1100_driver, void)

  address xbuff;
  int paylen, len;
  byte b, *eptr;

  entry (DR_LOOP)

RedoAll:
	if (TxOFF) {
		if (TxOFF == 3) {
			// Make sure to drain the XMIT queue each time you get
			// here
			tcvphy_erase (physid);
			if (RxOFF == 1) {
				power_down ();
				RxOFF = 2;
			}
		} else if (TxOFF == 1 && RxOFF) {
			power_down ();
			wait (zzv_qevent, DR_LOOP);
			release;
		}
	}

	if (RX_FIFO_READY) {
RedoRX:
    		LEDI (3, 1);

		// A packet has been received and is waiting in RX FIFO
		guard_stop (WATCH_RCV);
		if (RxOFF) {
			cc1100_rx_flush ();
			// Ignore it
			LEDI (3, 0);
			goto DoXMT;
		}

		if ((len = cc1100_rx_status ()) < 0) {
			// Error: typically FIFO overrun (shouldn't happen)
#if TRACE_DRIVER
			diag ("RX BAD STATUS");
#endif
			cc1100_rx_flush ();
			goto DoSkp;
		}

		if ((len & 1) == 0 || len < 7) {
			// Actual payload length must be even
#if TRACE_DRIVER
			diag ("RX BAD PACKET: %d", len);
#endif
			cc1100_rx_flush ();
			goto DoSkp;
		}

		paylen = cc1100_get_reg (CCxxx0_RXFIFO);
		if ((paylen & 1) || paylen != len - 3) {
#if TRACE_DRIVER
			diag ("RX PAYLOAD LENGTH MISMATCH: %d/%d", len, paylen);
#endif
			cc1100_rx_flush ();
			goto DoSkp;
		}

		if (paylen > rbuffl) {
#if TRACE_DRIVER
			diag ("RX PACKET TOO LONG: %d", paylen);
#endif
			cc1100_rx_flush ();
			goto DoSkp;
		}

		cc1100_get_reg_burst (CCxxx0_RXFIFO, (byte*)rbuff,
			(byte) paylen + 2);

		// We have extracted the packet, so we can start RCV for
		// another one

		// A precaution: make sure the FIFO is truly empty
		while (RX_FIFO_READY) {
#if TRACE_DRIVER
			diag ("RX FIFO SHOULD BE EMPTY");
#endif
			cc1100_rx_flush ();
		}

		enter_rx ();

		// Check the station ID before doing anything else
		if (statid != 0 && rbuff [0] != 0 &&
			rbuff [0] != statid) {
#if TRACE_DRIVER
			diag ("RX BAD STATID: %x", rbuff [0]);
#endif
			add_entropy (rbuff [3]);
			goto DoSkp;
		}


#if CRC_MODE > 1
		// Verify CRC
		len = paylen >> 1;
		if (w_chk (rbuff, len, 0)) {
			// Bad checksum
#if TRACE_DRIVER
			diag ("BAD CHECKSUM %x %x %x",
				(word*)(rbuff) [0],
				(word*)(rbuff) [1],
				(word*)(rbuff) [2]);
#endif
			// Ignore
			goto DoSkp;
		}

		eptr = (byte*)rbuff + paylen;

		((byte*)rbuff) [paylen - 2] = (*(eptr + 1) & 0x7f);
		((byte*)rbuff) [paylen - 1] = *eptr;
		add_entropy (rbuff [len-1]);

#else	/* CRC_MODE */

		// Status bytes
		eptr = (byte*)rbuff + paylen;
		b = *(eptr+1);
		add_entropy (*eptr ^ b);
		paylen += 2;

		if (b & 0x80) {
			// CRC OK: we will change this to software checksum
			*(eptr+1) = *eptr;
			// Swap these two
			*eptr = (b & 0x7f);
		} else {
			// Bad checksum
#if TRACE_DRIVER
			diag ("BAD CHECKSUM %x %x %x",
				(word*)(rbuff) [0],
				(word*)(rbuff) [1],
				(word*)(rbuff) [2]);
#endif
			// Ignore
			goto DoSkp;
		}
#endif	/* CRC_MODE */

		tcvphy_rcv (physid, rbuff, paylen);
DoSkp:
		LEDI (3, 0);
		// Receiver is on, we don't know about transmitter

		if ((TxOFF & 1)) {
			// It is off solid - just wait for the RX FIFO to
			// become ready
CWaitRX:
			if (RX_FIFO_READY)
				goto RedoRX;
WaitRX:
			wait (zzv_qevent, DR_LOOP);
			if (RxOFF == 0)
				rcv_enable_int;
			release;
		}
	}
DoXMT:
	// One more check: RX FIFO may have become ready in the meantime
	if (RX_FIFO_READY)
		// Reception has priority
		goto RedoRX;

	if (bckf_timer) {
		delay (bckf_timer, DR_LOOP);
		goto WaitRX;
	}

RetryXM:
	// Check for transmission
	if ((xbuff = tcvphy_get (physid, &paylen)) == NULL) {
		if (TxOFF == 2) {
			// Draining: stop xmt
			TxOFF = 3;
			goto RedoAll;
		}
		goto CWaitRX;
	}

	if (
#if CRC_MODE > 1
		paylen >= rbuffl	/* This includes 2 extra bytes */
#else
		paylen > rbuffl
#endif
			|| paylen < 6 || (paylen & 1)) {
		// Sanity check
#if TRACE_DRIVER
		diag ("TX ILLEGAL PACKET LENGTH: %d", paylen);
#endif
		tcvphy_end (xbuff);
		goto RetryXM;
	}

	xbuff [0] = statid;

	if (RX_FIFO_READY)
		// Last time to check
		goto RedoRX;

	if (clear_to_send () == NO) {
		// We have to wait
		gbackoff;
		proceed (DR_LOOP);
	}

	LEDI (2, 1);


#if CRC_MODE > 1
	// Calculate CRC
	len = (paylen >> 1) - 1;
	((word*)xbuff) [len] = w_chk ((word*)xbuff, len, 0);
#else
	paylen -= 2;		// Ignore the checksum bytes
#endif
	// Send the length byte
	cc1100_set_reg (CCxxx0_TXFIFO, paylen);

	cc1100_set_reg_burst (CCxxx0_TXFIFO, (byte*)xbuff, (byte) paylen);

	tcvphy_end (xbuff);

	// Note: this is crude. We should wait (roughly):
	// ((paylen + 6) * 8 / 10) * (1024/1000) ticks (assuming 0.1 msec per
	// bit. For a 32-bit packet, the above formula yields 31.12, so ...
	// you see ...
	delay (paylen, DR_SWAIT);
	release;

  entry (DR_SWAIT)

	if ((len = cc1100_status ()) == CC1100_STATE_TX) {
		delay (TXEND_POLL_DELAY, DR_SWAIT);
		release;
	}

	guard_stop (WATCH_XMT);
	LEDI (2, 0);

	bckf_timer = XMIT_SPACE;
	proceed (DR_LOOP);

endprocess (1)

#if	GUARD_PROCESS

#define	GU_ACTION	0

process (cc1100_guard, void)

  word stat;

  entry (GU_ACTION)

	if (zzv_gwch) {
#if TRACE_DRIVER
		diag ("GUARD WATCHDOG RESET: %x", zzv_gwch);
#endif
Reset:
		zzv_gwch = 0;
		chip_reset ();
		enter_rx ();
		p_trigger (zzv_drvprcs, ETYPE_USER, zzv_qevent);
		ldelay (GUARD_SHORT_DELAY, GU_ACTION);
		release;
	}

	if ((TxOFF & 1) && RxOFF) {
		// The chip is powered down, so don't bother
		ldelay (GUARD_LONG_DELAY, GU_ACTION);
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
		release;
	}

	if (stat != CC1100_STATE_RX) {
		// Something is wrong: note that stat == IDLE implies
		// RX_FIFO_READY
#if TRACE_DRIVER
		diag ("GUARD BAD STATE: %d", stat);
#endif
		goto Reset;
	}

	// Do not break reception in progress
	stat = cc1100_get_reg (CCxxx0_RXBYTES);

	if (stat & 0x80) {
		// Overflow
#if TRACE_DRIVER
		diag ("GUARD HUNG FIFO OVERFLOW");
#endif
		goto Reset;
	}

	if (stat == 0) {
		// Recalibrate
		enter_idle ();
		enter_rx ();
		ldelay (GUARD_LONG_DELAY, GU_ACTION);
	} else {
		guard_start (WATCH_RCV);
		delay (GUARD_SHORT_DELAY, GU_ACTION);
	}

endprocess (1)

#endif	/* GUARD_PROCESS */

void phys_cc1100 (int phy, int mbs) {
/*
 * mbs includes two bytes for the checksum
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

	rbuffl = (byte) mbs;	// buffer length in bytes, including checksum
#if CRC_MODE > 1
	rbuffl += 2;
#endif
	if ((rbuff = umalloc (rbuffl)) == NULL)
		syserror (EMALLOC, "phys_cc1100");

	statid = 0;
	physid = phy;
	rnd_seed = 12345;

	/* Register the phy */
	zzv_qevent = tcvphy_reg (phy, option, INFO_PHYS_CC1100);

	/* Both parts are initially active */
	LEDI (1, 0);
	LEDI (2, 0);
	LEDI (3, 0);

	// Things start in the off state
	RxOFF = TxOFF = 1;
	// Default power corresponds to the center == 0dBm
	xpower = 3;
	/* Initialize the device */
	ini_cc1100 ();

	/* Install the backoff timer */
	utimer (&bckf_timer, YES);
	bckf_timer = 0;

	/* Start the processes */
	zzv_drvprcs = fork (cc1100_driver, NULL);

#if GUARD_PROCESS
	fork (cc1100_guard, NULL);
#endif

}

static int option (int opt, address val) {
/*
 * Option processing
 */
	int ret = 0;

	switch (opt) {

	    case PHYSOPT_STATUS:

		ret = ((TxOFF == 0) << 1) | (RxOFF == 0);
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_TXON:

		if (RxOFF) {
			// Start up
			chip_reset ();
			// Even if RX is off, this is our default mode
			enter_rx ();
		}

		TxOFF = 0;

		if (RxOFF)
			LEDI (1, 1);
		else
			LEDI (1, 2);

		trigger (zzv_qevent);
		break;

	    case PHYSOPT_RXON:

		if (TxOFF) {
			// Start up
			chip_reset ();
			enter_rx ();
		}

		RxOFF = 0;

		if (TxOFF)
			LEDI (1, 1);
		else
			LEDI (1, 2);
		trigger (zzv_qevent);
		break;

	    case PHYSOPT_TXOFF:

		/* Drain */
		TxOFF = 2;
		if (RxOFF)
			LEDI (1, 0);
		else
			LEDI (1, 1);
		trigger (zzv_qevent);
		break;

	    case PHYSOPT_TXHOLD:

		TxOFF = 1;
		if (RxOFF)
			LEDI (1, 0);
		else
			LEDI (1, 1);
		trigger (zzv_qevent);
		break;

	    case PHYSOPT_RXOFF:

		RxOFF = 1;
		if (TxOFF)
			LEDI (1, 0);
		else
			LEDI (1, 1);
		trigger (zzv_qevent);
		break;

	    case PHYSOPT_CAV:

		/* Force an explicit backoff */
		if (val == NULL)
			bckf_timer = 0;
		else
			bckf_timer = *val;
		trigger (zzv_qevent);
		break;

	    case PHYSOPT_SETPOWER:

		if (val == NULL)
			// Default
			xpower = 3;
		else if (*val > 7)
			xpower = 7;
		else
			xpower = *val;
		cc1100_set_power ();

		break;

	    case PHYSOPT_GETPOWER:

		ret = (int) xpower;
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_SETSID:

		statid = (val == NULL) ? 0 : *val;
		break;

            case PHYSOPT_GETSID:

		ret = (int) statid;
		if (val != NULL)
			*val = ret;
		break;

	    default:

		syserror (EREQPAR, "phys_cc1100 option");

	}
	return ret;
}


