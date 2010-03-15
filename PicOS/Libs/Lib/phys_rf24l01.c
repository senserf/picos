/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "tcvphys.h"
#include "rf24l01.h"

static int option (int, address);

static word	*rbuff = NULL,
		bckf_timer = 0,

		physid,
		statid;

word		zzv_drvprcs, zzv_qevent;


static byte	RxOFF,			// Transmitter on/off flags
		TxOFF;

/* ========================================= */

static const byte config [] = RF24L01_PRECONFIG;
static const byte netaddr [] = NETWORK_ADDRESS;
static const byte adtfill [] = { REG_RX_ADDR_P1, REG_TX_ADDR };

#if MULTIPLE_PIPES
static const byte pipe_length [] = {
					PIPE1_LENGTH,
					PIPE2_LENGTH,
					PIPE3_LENGTH,
					PIPE4_LENGTH,
					PIPE5_LENGTH
				    };
#endif

static void put_byte (byte val) {
/*
 * Send a byte over SPI
 */
	word i;

	for (i = 0; i < 8; i++) {
		if (val & 0x80)
			data_up;
		else
			data_down;
		val <<= 1;
		sck_up;
		sck_down;
	}
}

static byte get_byte () {
/*
 * Retrieve a byte from SPI
 */
	byte i, res;

	for (res = 0, i = 0; i < 8; i++) {
		res <<= 1;
		if (data_val)
			res |= 1;
		sck_up;
		sck_down;
	}

	return res;
}

static void put_reg (byte reg, byte val) {

	csn_down;
	put_byte (CMD_W_REGISTER + reg);
	put_byte (val);
	csn_up;
	// Let's keep data consistently down when nothing happens
	data_down;
}

static byte get_reg (byte reg) {

	byte res;

	csn_down;
	put_byte (CMD_R_REGISTER + reg);
	data_down;
	res = get_byte ();
	csn_up;

	return res;
}

static byte get_stat () {
/*
 * Retrieve the status byte
 */
	byte i, stat;

	data_up;	// CMD_NOP is 'all ones'
	csn_down;

	for (stat = 0, i = 0; i < 8; i++) {
		stat <<= 1;
		if (data_val)
			stat |= 1;
		sck_up;
		sck_down;
	}
	csn_up;
	data_down;

	return stat;
}

static byte chip_config () {
/*
 * Configure registers
 */
	byte i, j;

	i = 0;
	while (config [i] != 255) {
		put_reg (config [i], config [i+1]);
#if 0
		diag ("REG %d -> %x", config [i], config [i+1]);
#endif
		i += 2;
	}

	// Addresses
	for (i = 0; i < sizeof (adtfill); i++) {
#if 0
		diag ("REG %d -> %x %x %x %x %x", adtfill [i],
			netaddr [0],
			netaddr [1],
			netaddr [2],
			netaddr [3],
			netaddr [4]
		);
#endif
		csn_down;
		put_byte (CMD_W_REGISTER + adtfill [i]);
		for (j = 0; j < 5; j++)
			put_byte (netaddr [j]);
		csn_up;
	}
	data_down;
}

static byte chip_verify () {

	byte i, j, reg [5];

	i = 0;
	while (config [i] != 255) {
		reg [0] = get_reg (config [i]);
#if 0
		diag ("REG %d == %x", config [i], reg [0]);
#endif
		if (config [i] != REG_STATUS && reg [0] != config [i+1]) {
			diag ("Short register check failed: [%d] == %x != %x",
				config [i], reg [0], config [i+1]);
			syserror (EHARDWARE, "RF24L01 reg");
		}
		i += 2;
	}

	for (i = 0; i < sizeof (adtfill); i++) {
		csn_down;
		put_byte (CMD_R_REGISTER + adtfill [i]);
		data_down;
		for (j = 0; j < 5; j++)
			reg [j] = get_byte ();
		csn_up;
#if 0
		diag ("REG %d == %x %x %x %x %x", adtfill [i],
			reg [0],
			reg [1],
			reg [2],
			reg [3],
			reg [4]
		);
#endif
		for (j = 0; j < 5; j++) {
			if (reg [j] != netaddr [j]) {
				diag ("Long register check failed: "
					"[%d](%d) == %x != %x",
						adtfill [i], j, reg [j],
							netaddr [j]);
			}
		}
	}
}

static void power_down () {

	put_reg (REG_CONFIG, config [REG_CONFIG]);
#if 0
	diag ("Power down");
#endif
}

static void rcv_enable () {

	// Clear previous int
	put_reg (REG_STATUS, REG_STATUS_RX_DR);
	// Enable interrupts
	set_rcv_int;
	put_reg (REG_CONFIG, config [REG_CONFIG] + REG_CONFIG_PWR_UP +
		REG_CONFIG_PRIM_RX);
	ce_up;
#if 0
	{ 
		byte reg;
		diag ("RX_ENABLED:");
		for (reg = 0; reg < 24; reg++)
			diag ("R[%d] = %x", reg, get_reg (reg));
	}
#endif
}

static void xmt_enable () {

	put_reg (REG_CONFIG, config [REG_CONFIG] + REG_CONFIG_PWR_UP +
		REG_CONFIG_PRIM_TX);
#if 0
	diag ("XM_ENABLED: %x %d", get_reg (REG_CONFIG), ce_val);
#endif
}

static void rx_flush () {

	csn_down;
	put_byte (CMD_FLUSH_RX);
	csn_up;
	data_down;
}

static void tx_flush () {

	csn_down;
	put_byte (CMD_FLUSH_TX);
	csn_up;
	data_down;
}

static void mod_reg (byte reg, byte val) {
/*
 * Modify a register without disturbing the state
 */
	byte ceval;

	// Wait while TX FIFO is busy
	while ((get_reg (REG_FIFO_STATUS) & REG_FIFO_STATUS_TX_E) != 0);

	// Hopefully, we are consistent here and never leave TX FIFO full
	// while TX is disabled. If we ever get stuck in here, FIXME: if
	// TX is disabled and TX FIFO is nonempty, kill the FIFO.

	// Preserve previous CE
	ceval = ce_val;

	// Force CE down to deactivate the chip
	ce_down;

	put_reg (reg, val);
	if (ceval)
		ce_up;
}

static void setpower (byte p) {

	if (p > RADIO_MAX_POWER)
		p = RADIO_MAX_POWER;

	mod_reg (REG_RF_SETUP, (get_reg (REG_RF_SETUP) & 0xf9) | (p << 1));
}

static byte getpower () {

	return (get_reg (REG_RF_SETUP) >> 1) & 0x3;
}

static void setchannel (byte c) {

	if (c > RADIO_MAX_CHANNEL)
		c = RADIO_MAX_CHANNEL;

	mod_reg (REG_RF_CH, c);
}

static byte getchannel () {

	return get_reg (REG_RF_CH);
}

static void setparam (byte *pa) {

	if (pa == NULL) {
		// Reset the chip completely
		mdelay (1);
		ce_down;
		mdelay (1);
		tx_flush ();	
		chip_config ();
		// Kick the process; hopefully, it will sort things out
		trigger (zzv_qevent);
	}

	while (*pa != 255) {
		put_reg (*pa, *(pa+1));
		pa += 2;
	}
}

static void ini_rf24l01 () {

	byte stat;

	ini_regs;
	ce_down;
	csn_up;
	mdelay (2);
	chip_config ();
	stat = chip_verify ();

	diag ("RF24L01 initialized: %x", ((word) get_reg (REG_CONFIG) << 8) |
		stat);
}

#if MULTIPLE_PIPES

static void do_rx_fifo (byte PIP_NUM) {

#define	PIP_LENGTH	(pipe_length [PIP_NUM - 1])
#define	XMT_PAYLEN	pn

#else	/* SINGLE PIPE */

#define	PIP_NUM	1
#define	PIP_LENGTH	(MAX_PAYLOAD_LENGTH - 1)
#define	XMT_PAYLEN	(MAX_PAYLOAD_LENGTH - 2)

static void do_rx_fifo () {
#endif	/* MULTIPLE_PIPES */

/*
 * Extract the payload from FIFO 1. This is the only FIFO we use at the moment.
 */

	byte n, len;
	byte *bptr;

	if (RxOFF) {
		// The receiver is off; clean the FIFO and return
		rx_flush ();
		goto Rtn;
	}

	len = get_reg (REG_RX_PW_P0 + PIP_NUM);
	if (len != PIP_LENGTH) {
		// Something wrong
#if 0
		diag ("RX FIFO illegal length [%d]: %d", PIP_NUM, len);
#endif
		rx_flush ();
		goto Rtn;
	}

	csn_down;
	put_byte (CMD_R_RX_PAYLOAD);
	data_down;

	len = get_byte ();
	for (n = PIP_LENGTH - 1, bptr = (byte*) rbuff; n--; )
		*bptr++ = get_byte ();
	csn_up;
#if 0
	diag ("RX PKT [%d]: %x %x %x", len, rbuff [0], rbuff [1], rbuff [2]);
#endif

	if ((len & 1) != 0 || len < 4 || len >= PIP_LENGTH) {
		// Illegal length
#if 0
		diag ("RX illegal payload length [%d]: %d", PIP_LENGTH, len);
#endif
		goto Rtn;
	}

	// Note: we assume that the buffer length is fixed at 32; no need
	// to check if the buffer can accommodate the packet
	if (statid != 0 && rbuff [0] != 0 && rbuff [0] != statid) {
		// Illegal ID
#if 0
		diag ("RX statid mismatch: %x %x", statid, rbuff [0]);
#endif
		goto Rtn;
	}

	// Store two zero bytes at the end - for compatibility with other
	// drivers. Will get rid of that (as well as the network ID), if this
	// ever makes it to serious production.

	rbuff [len << 1] = 0;
	tcvphy_rcv (physid, rbuff, len + 2);
Rtn:
#if backoff_after_receive
	gbackoff;
#else
	NOP;
#endif
}

#define	DR_LOOP		0
#define	DR_XWAIT	1

#if MULTIPLE_PIPES

#define	TRY_RECEIVE \
	while ((pn = get_stat () & 0xe) < 0xc) { \
		LEDI (2, 1); \
		do_rx_fifo (pn >> 1); \
		LEDI (2, 0); \
	} \

#else	/* SINGLE PIPE */

#define	TRY_RECEIVE	\
	while ((get_stat () & 0xe) < 0xc) { \
		LEDI (2, 1); \
		do_rx_fifo (); \
		LEDI (2, 0); \
	} \

#endif	/* MULTIPLE_PIPES */

thread (rf24l01_driver)

  byte pn, st;
  address xbuff;
  word paylen;

  entry (DR_LOOP)

	ce_down;

	if ((TxOFF & 1)) {
		// The transmitter is OFF solid
		if (TxOFF == 3) {
			// Make sure to drain the XMIT queue each time you get
			// here
			tcvphy_erase (physid);
			if (RxOFF == 1) {
				power_down ();
				// Don't do this more than once
				RxOFF = 2;
			}
		} else if (TxOFF == 1 && RxOFF) {
			// We can suspend ourselves completely until something
			// happens
			power_down ();
			wait (zzv_qevent, DR_LOOP);
			release;
		}

		// Keep receiving
		TRY_RECEIVE;

		wait (zzv_qevent, DR_LOOP);
		if (RxOFF == 0)
			rcv_enable ();
		release;
	}

	TRY_RECEIVE;

	if (bckf_timer) {
		delay (bckf_timer, DR_LOOP);
		wait (zzv_qevent, DR_LOOP);
		if (RxOFF == 0)
			rcv_enable ();
		release;
	}

	if (tcvphy_top (physid) == NULL) {
		// Nothing to transmit
		if (TxOFF) {
			// TxOFF == 2 -> draining: stop xmt
#if 0
			diag ("END TX DRAINING");
#endif
			TxOFF = 3;
			proceed (DR_LOOP);
		}
		// Wait
		wait (zzv_qevent, DR_LOOP);
		if (RxOFF == 0)
			rcv_enable ();
		release;
	}

	// FIXME: do LBT later. Perhaps it is not needed considering that
	// the actual transmission of the packet takes much less than 1ms.

	if ((xbuff = tcvphy_get (physid, &paylen)) == NULL)
		// Sanity check
		proceed (DR_LOOP);

	sysassert (paylen <= MAX_PAYLOAD_LENGTH && paylen >= 6 &&
	    (paylen & 1) == 0,
		"phys_rf24l01 xmt pktl");

	LEDI (1, 1);

	paylen -= 2;	// The chip provides its own checksum

	xbuff [0] = statid;

#if MULTIPLE_PIPES
	// Select the right receipt address and pipe width
	for (pn = 1; pn < 6; pn++)
		if (pipe_length [pn - 1] > paylen)
			break;

	// Modify the LSB of the XMT address
	put_reg (REG_TX_ADDR, pn);
	// Transmit payload length
	pn = pipe_length [pn - 1] - 1;
#endif
	xmt_enable ();

	csn_down;
	put_byte (CMD_W_TX_PAYLOAD);

	put_byte ((byte)paylen);

	for (st = 0; st < XMT_PAYLEN; st++)
		put_byte (((byte*)xbuff) [st]);
	csn_up;
	data_down;

	// This will start the transmission
	ce_up;
	// Now wait until done

	tcvphy_end (xbuff);
#if 0
	if ((get_stat () & REG_STATUS_TX_DS) != 0) {
		diag ("TX EARLY!!!");
	}
#endif

#if BUSY_WAIT_ETX

	// Busy-wait for the end of packet < 0.5 msec
	while ((get_stat () & REG_STATUS_TX_DS) == 0);
	// Remove the TX done status. Chip bug: despite the TX done interrupt
	// being formally masked, the IRQ line goes low when this is not done
	// explicitly.
	put_reg (REG_STATUS, REG_STATUS_TX_DS);

	LEDI (1, 0);
	bckf_timer = XMIT_SPACE;
	proceed (DR_LOOP);
#else
	// This much will do, it is an overkill actually
	delay (1, DR_XWAIT);
	release;

  entry (DR_XWAIT)

	pn = get_stat ();
	if ((pn & REG_STATUS_TX_DS) == 0) {
		// Not likely - just in case
		delay (1, DR_XWAIT);
		release;
	}
	// Done
	put_reg (REG_STATUS, REG_STATUS_TX_DS);
	LEDI (1, 0);
	bckf_timer = XMIT_SPACE;
	proceed (DR_LOOP);
#endif

endthread

void phys_rf24l01 (int phy, int mbs) {

	if (rbuff != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "phys_rf24l01");

	/*
	 * We book the maximum-size buffer, i.e., 34 bytes. The checksum/
	 * RSSI part is not used on this chip, but we leave it for compatibility
	 * with other drivers.
	 */

	if (mbs != MAX_PAYLOAD_LENGTH)
		syserror (EREQPAR, "phys_rf24l01 mbs != 32");

	if ((rbuff = umalloc (MAX_PAYLOAD_LENGTH)) == NULL)
		syserror (EMALLOC, "phys_rf24l01");

	statid = 0;
	physid = phy;

	/* Register the phy */
	zzv_qevent = tcvphy_reg (phy, option, INFO_PHYS_RF24L01);

	/* Both parts are initially active */
	LEDI (0, 0);
	LEDI (1, 0);
	LEDI (2, 0);

	// Things start in the off state
	RxOFF = TxOFF = 1;

	ini_rf24l01 ();

	/* Install the backoff timer */
	utimer (&bckf_timer, YES);
	bckf_timer = 0;

	/* Start the driver process */
	zzv_drvprcs = runthread (rf24l01_driver);
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

		TxOFF = 0;

		if (RxOFF)
			LEDI (0, 1);
		else
			LEDI (0, 2);

		trigger (zzv_qevent);
		break;

	    case PHYSOPT_RXON:

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
			setpower (RADIO_DEF_XPOWER);
		else
			setpower ((byte)(*val));
		break;

	    case PHYSOPT_GETPOWER:

		ret = (int) getpower ();
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

	    case PHYSOPT_SETCHANNEL:

		if (val == NULL)
			// Default, immediate
			setchannel (RADIO_DEF_CHANNEL);
		else
			setchannel ((byte) (*val));
		break;

	    case PHYSOPT_GETCHANNEL:

		ret = getchannel ();
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_SETPARAM:

		setparam ((byte*)val);
		break;

	    default:

		syserror (EREQPAR, "phys_rf24l01 option");

	}
	return ret;
}
