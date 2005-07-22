#ifndef	__pg_rf24g_h
#define	__pg_rf24g_h	1

/*
 * With RF24G, in the ShockBurst mode (which we use in this driver), the
 * maximum length of the data buffer is equal to 32 bytes - address length
 * - CRC length. The CRC can be 1 or two bytes. We make these hardwired,
 * as there isn't really a lot of useful flexibility from TARP's point of
 * view. The raw frame layout is this:
 *
 *           ADR LEN .... DATA .... CRC
 *
 * where ADR is 1 byte (and represents the "application class"). All stations
 * in the same setup will be tuned to the same address to receive all traffic,
 * which by the very nature of TARP is boradcast. LEN is 1 byte and stores the
 * actual length of DATA, which can be anything from 0 to 28 bytes. CRC is 2
 * bytes. All sent/received packets are always 28 bytes long with a filler
 * used for shorter packets.
 *
 */
#define	RADIO_DEF_ADR_LEN	1
#define RADIO_DEF_CRC_LEN	2
#define	RADIO_DEF_BUF_LEN	(32-RADIO_DEF_ADR_LEN-RADIO_DEF_CRC_LEN-1)
/*
 * These values are merely defaults changeable with tcv_control
 */
#define	RADIO_DEF_MNBACKOFF	32	/* Minimum backoff */
#define	RADIO_DEF_XMITSPACE	8	/* Space between xmitted packets */
#define	RADIO_DEF_BSBACKOFF	0xff	/* Randomized component */
/*
 * Four levels are available (0-3). Anything >= 3 is equivalent to 3.
 */
#define	RADIO_DEF_XPOWER	1	/* Default transmit power */
/*
 * Default channel (0-127)
 */
#define RADIO_DEF_CHANNEL	0x0
/*
 * Default bit rate (0 = 250, 1 = 1000 kbps)
 */
#define	RADIO_DEF_BITRATE	0

/*
 * Serial strobe delay (usec)
 */
#define	TINY_DELAY		10


/* ================================================================== */

/*
 * Pin assignment:
 *
 *      eCOG                       RF24G                     GPIO
 *   ===================================================================
 *   J11-08 (A0) + LED             CE                        00   out
 *   J11-09 (A1) + LED             CS                        01   out
 *   J12-07 (L4)                   DATA                      12   in/out
 *   J12-08 (L5)                   DR1                       13   in (RCV int)
 *   J12-09 (L6)                   CLK(1)                    14   in/out
 *   J12-11 VCC                    3V                        --
 *   J12-12 GND                    GND                       --
 *   ===================================================================
 *
 */

/*
 * Configuration registers
 */
#define	PARBLOCK_SIZE		15	// 15 bytes of parameters


/*
 * A0/A1 (GPIO 0/1) permanently set as out, L5 (GPIO 13) permanently set as
 * in; the remaining registers set temporarily as out and low.
 */
#define	ini_regs		do { \
				    rg.io.gp0_3_out = \
					IO_GP0_3_OUT_CLR0_MASK | \
					IO_GP0_3_OUT_CLR1_MASK | \
					IO_GP0_3_OUT_EN0_MASK | \
					IO_GP0_3_OUT_EN1_MASK ; \
				    rg.io.gp12_15_out = \
					IO_GP12_15_OUT_CLR12_MASK | \
					IO_GP12_15_OUT_EN12_MASK | \
					IO_GP12_15_OUT_CLR13_MASK | \
					IO_GP12_15_OUT_DIS13_MASK | \
					IO_GP12_15_OUT_CLR14_MASK | \
					IO_GP12_15_OUT_EN14_MASK ; \
				} while (0)

// Use 0x00d0 for rising edge. We prefer high status to avoid packet loss
// on the race at the end of reception.
#define	GP_INT_RCV	0x00b0	// High signal on GPIO13
#define	GP_INT_MSK	0x00f0	// Mask out

#define	clr_rcv_int	(rg.io.gp12_15_cfg &= ~GP_INT_MSK)
#define	set_rcv_int	(rg.io.gp12_15_cfg |= GP_INT_RCV)
/*
 * Check for our interrupt
 */
#define	rf24g_int	(rg.io.gp8_15_sts & 0x0800)


/*
 * Signal operations
 */
#define	rf24g_csup	rg.io.gp0_3_out = IO_GP0_3_OUT_SET1_MASK
#define	rf24g_csdown	rg.io.gp0_3_out = IO_GP0_3_OUT_CLR1_MASK
#define	rf24g_clk0up	rg.io.gp12_15_out = IO_GP12_15_OUT_SET14_MASK
#define	rf24g_clk0down	rg.io.gp12_15_out = IO_GP12_15_OUT_CLR14_MASK
#define	rf24g_ceup	rg.io.gp0_3_out = IO_GP0_3_OUT_SET0_MASK
#define	rf24g_cedown	rg.io.gp0_3_out = IO_GP0_3_OUT_CLR0_MASK
#define	rf24g_dataout	rg.io.gp12_15_out = \
					IO_GP12_15_OUT_CLR12_MASK | \
					IO_GP12_15_OUT_EN12_MASK
#define	rf24g_datain	rg.io.gp12_15_out = \
					IO_GP12_15_OUT_CLR12_MASK | \
					IO_GP12_15_OUT_DIS12_MASK
#define	rf24g_dataup	rg.io.gp12_15_out = IO_GP12_15_OUT_SET12_MASK
#define	rf24g_datadown	rg.io.gp12_15_out = IO_GP12_15_OUT_CLR12_MASK
#define	rf24g_getdbit	(rg.io.gp8_15_sts & 0x0100)

#define	HSTAT_RCV	0x80	// Receiver enabled
#define	HSTAT_XMT	0x00	// Transmitter enabled
#define HSTAT_IDLE	0x01

#define	gbackoff	do { \
				zzv_rdbk->seed = (zzv_rdbk->seed + 1) * 6789; \
				zzv_rdbk->backoff = zzv_rdbk->delmnbkf + \
					(zzv_rdbk->seed & zzv_rdbk->delbsbkf); \
			} while (0)

#define	LEDI(n,s) 	(rg.io.gp0_3_out = ((1 << (s)) << 4 * (n)))

void phys_rf24g (int, int, int, int, int);

#define	rxevent	((word)&zzv_rdbk)

typedef	struct {

	word	*rbuffer;
	byte 	rxoff, txoff, rxwait, channel, group, bitrate;
	word	qevent, physid, statid;
	/* ================== */
	/* Control parameters */
	/* ================== */
	word 	delmnbkf,		// Minimum backoff
		delbsbkf,		// Backof mask for random component
		delxmspc,		// Minimum xmit packet space
		seed,			// For the random number generator
		backoff;		// Calculated backoff for xmitter
} radioblock_t;

extern radioblock_t *zzv_rdbk;

#endif
