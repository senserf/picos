#ifndef	__pg_dw1000_h
#define	__pg_dw1000_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2015                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "kernel.h"
#include "dw1000_sys.h"

//+++ "dw1000.c"

// ============================================================================

#ifndef	DW1000_OPTIONS
#define	DW1000_OPTIONS		0x0001
#endif

// ============================================================================

#define	DW1000_ROLE_TAG		0
#define	DW1000_ROLE_PEG		1

// ============================================================================

#define	DW1000_REG_DEVID	0x00
#define	DW1000_REG_EUI		0x01
#define	DW1000_REG_PANADR	0x03
#define	DW1000_REG_SYS_CFG	0x04
#define	DW1000_REG_TX_POWER	0x1e
#define	DW1000_REG_OTPC		0x2d		// OTP control
#define	DW1000_REG_PMSC		0x36
#define	DW1000_REG_RF_CONF	0x28
#define	DW1000_REG_TX_CAL	0x2a
#define	DW1000_REG_AON		0x2c
#define	DW1000_REG_FS_CTRL	0x2b
#define	DW1000_REG_LDE_IF	0x2e
#define	DW1000_REG_DRX_CONF	0x27
#define	DW1000_REG_AGC_CTRL	0x23
#define	DW1000_REG_USR_SFD	0x21
#define	DW1000_REG_CHAN_CTRL	0x1f
#define	DW1000_REG_TX_FCTRL	0x08
#define	DW1000_REG_RX_FINFO	0x10
#define	DW1000_REG_TX_BUFFER	0x09
#define	DW1000_REG_SYS_STATUS	0x0f
#define	DW1000_REG_SYS_CTRL	0x0d

// ============================================================================

#define	DW1000_IRQ_ICRBP	0x80000000
#define	DW1000_IRQ_HSRBP	0x40000000
#define	DW1000_IRQ_AFFREJ	0x20000000
#define	DW1000_IRQ_TXBERR	0x10000000
#define	DW1000_IRQ_HPDWARN	0x08000000
#define	DW1000_IRQ_RXSFDTO	0x04000000
#define	DW1000_IRQ_CLKPLL_LL	0x02000000
#define	DW1000_IRQ_RFPLL_LL	0x01000000
#define	DW1000_IRQ_SLP2INIT	0x00800000
#define	DW1000_IRQ_GPIOIRQ	0x00400000
#define	DW1000_IRQ_RXPTO	0x00200000
#define	DW1000_IRQ_RXOVRR	0x00100000
#define	DW1000_IRQ_LDEERR	0x00040000
#define	DW1000_IRQ_RXRFTO	0x00020000
#define	DW1000_IRQ_RXRFSL	0x00010000
#define	DW1000_IRQ_RXFCE	0x00008000
#define	DW1000_IRQ_RXFCG	0x00004000
#define	DW1000_IRQ_RXDFR	0x00002000
#define	DW1000_IRQ_RXPHE	0x00001000
#define	DW1000_IRQ_RXPHD	0x00000800
#define	DW1000_IRQ_LDEDONE	0x00000400
#define	DW1000_IRQ_RXSFDD	0x00000200
#define	DW1000_IRQ_RXPRD	0x00000100
#define	DW1000_IRQ_TXFRS	0x00000080
#define	DW1000_IRQ_TXPHS	0x00000040
#define	DW1000_IRQ_TXPRS	0x00000020
#define	DW1000_IRQ_TXFRB	0x00000010
#define	DW1000_IRQ_AAT		0x00000008
#define	DW1000_IRQ_ESYNCR	0x00000004
#define	DW1000_IRQ_CPLOCK	0x00000002
#define	DW1000_IRQ_IRQS		0x00000001

#define DW1000_IRQ_RXERRORS	(DW1000_IRQ_RXPHE 	|\
				 DW1000_IRQ_RXFCE 	|\
				 DW1000_IRQ_RXRFSL	|\
 	 	 	 	 DW1000_IRQ_RXSFDTO	|\
				 DW1000_IRQ_RXRFTO	|\
				 DW1000_IRQ_RXPTO	|\
                                 DW1000_IRQ_AFFREJ	|\
				 DW1000_IRQ_LDEERR) 

#define DW1000_IRQ_RXFINE  	(DW1000_IRQ_RXDFR 	|\
				 DW1000_IRQ_RXFCG 	|\
				 DW1000_IRQ_RXPRD 	|\
				 DW1000_IRQ_RXSFDD	|\
				 DW1000_IRQ_RXPHD 	|\
				 DW1000_IRQ_LDEDONE) 

#define DW1000_IRQ_ALLTX	(DW1000_IRQ_AAT 	|\
				 DW1000_IRQ_TXFRB 	|\
				 DW1000_IRQ_TXPRS 	|\
				 DW1000_IRQ_TXPHS 	|\
				 DW1000_IRQ_TXFRS) 

#define	DW1000_IRQ_ALLSANE	(DW1000_IRQ_ALLTX 	|\
				 DW1000_IRQ_RXFINE	|\
				 DW1000_IRQ_RXERRORS)

// ============================================================================

#define	DW1000_EVT_BAD		0x80
#define	DW1000_EVT_XMT		0x81
#define	DW1000_EVT_TMO		0x82
#define	DW1000_EVT_NIL		0x00

// ============================================================================

#define	DW1000_CF_FFEN		0x00000001
#define	DW1000_CF_FFBC		0x00000002
#define	DW1000_CF_FFAD		0x00000008
#define	DW1000_CF_FFAA		0x00000010
#define	DW1000_CF_FFMASK	0x000001FF	// All filtering bits
#define	DW1000_CF_HIRQ_POL	0x00000200	// IRQ polarity is high
#define	DW1000_CF_DIS_DRXB	0x00001000	// Disable dual buffering
#define	DW1000_CF_DIS_STPX	0x00040000	// Disable smart TX power
#define	DW1000_CF_RXM110K	0x00400000	// Data rate 110K
#define	DW1000_CF_RXAUTR	0x20000000	// RX auto re-enable

// ============================================================================

#define	DW1000_ADDR_LDOTUNE	0x04
#define	DW1000_ADDR_ANTDELAY	0x1c
#define	DW1000_ADDR_XTRIM	0x1e
#define	DW1000_ADDR_TXCONF	0x10

#define	DW1000_ONW_LDC		0x0040		// AON_WCFG load user config
#define	DW1000_ONW_L64P		0x0080		// AON_WCFG L64P
#define	DW1000_ONW_PRES_SLEEP	0x0100		// AON_WCFG preserve sleep cnf.
#define	DW1000_ONW_LLDE		0x0800		// AON_WCFG load microcode
#define	DW1000_ONW_LLDO		0x1000		// AON_WCFG load LDOTUNE
#define	DW1000_ONW_RX		0x0002		// AON_WCFG start in RX mode

#define	DW1000_CF0_SLEEP_EN	0x01		// AON_CF0 sleep enable
#define	DW1000_CF0_WAKE_SPI	0x04		// AON_CF0 wake on SPI

//! Constants for selecting the bit rate for data TX (and RX)
//! These are defined for write (with just a shift) the TX_FCTRL register
#define DW1000_BR_110K		0	//!< UWB bit rate 110 kbits/s
#define DW1000_BR_850K		1	//!< UWB bit rate 850 kbits/s
#define DW1000_BR_6M8		2	//!< UWB bit rate 6.8 Mbits/s

//! Constants for specifying the (Nominal) mean Pulse Repetition Frequency
//! These are defined for direct write (with a shift if necessary) to
//! CHAN_CTRL and TX_FCTRL regs
#define DW1000_PRF_16M		1	//!< UWB PRF 16 MHz
#define DW1000_PRF_64M		2	//!< UWB PRF 64 MHz

//! Constants for specifying Preamble Acquisition Chunk (PAC) Size in symbols
#define DW1000_PAC8		0	//!< PAC  8 (preamble 128 and below)
#define DW1000_PAC16		1	//!< PAC 16 (256)
#define DW1000_PAC32		2	//!< PAC 32 (512)
#define DW1000_PAC64		3	//!< PAC 64 (1024 and up)

//! constants for specifying TX Preamble length in symbols defined to allow
//! them be directly written into byte 2 of the TX_FCTRL register (i.e. a four
// bit value destined for bits 20..18 but shifted left by 2 for byte alignment)
#define DW1000_PLEN_4096 0x0C	//! Standard preamble length 4096 symbols
#define DW1000_PLEN_2048 0x28	//! Non-standard preamble length 2048 symbols
#define DW1000_PLEN_1536 0x18	//! Non-standard preamble length 1536 symbols
#define DW1000_PLEN_1024 0x08	//! Standard preamble length 1024 symbols
#define DW1000_PLEN_512	 0x34	//! Non-standard preamble length 512 symbols
#define DW1000_PLEN_256	 0x24	//! Non-standard preamble length 256 symbols
#define DW1000_PLEN_128	 0x14	//! Non-standard preamble length 128 symbols
#define DW1000_PLEN_64	 0x04	//! Standard preamble length 64 symbols

typedef struct {
	byte	channel:1,	// Boolean: 0 - 2, 1 - 5
		prf:1,		// PRF code, also Boolean: 0 - 16M, 1 - 64M
		precode:1,	// Preamble code: 0 - 3, 1 - 9
		nssfd:1,	// Boolean: non-standard SFD
		datarate:1,	// Boolean: 0 - 110K, 1 - 6M8
		preamble:1,	// Boolean: 0 - 128, 1 - 1024
		pacsize:2;	// PAC selection: 0 through 3
} chconfig_t;

// Preamble selection determines SFD timeout: 0 - 129, 1 - 1057
#define	dw1000_sfdto		(mode.prf ? 1057 : 129)
#define	dw1000_channel		(mode.channel ? 5 : 2)
#define	dw1000_precode		(mode.precode ? 9 : 3)
#define	dw1000_prf		(mode.prf ? DW1000_PRF_64M : DW1000_PRF_16M)
#define	dw1000_preamble		(mode.preamble ? DW1000_PLEN_1024 : \
					DW1000_PLEN_128)
#define	dw1000_datarate		(mode.datarate ? DW1000_BR_6M8 : DW1000_BR_110K)

// ============================================================================

#ifdef	DW1000_DEFINE_RF_SETTINGS

// The configuration options to choose from; I am copying them creatively
// from Deca driver (8 options). We may settle later on something fixed.

static const chconfig_t chconfig [] = {
//         CH PRF PCO NSF RAT PRE PAC
	{  0,  0,  0,  1,  0,  1,  DW1000_PAC32  },
	{  0,  0,  0,  0,  1,  0,  DW1000_PAC8   },
	{  0,  1,  1,  1,  0,  1,  DW1000_PAC32  },
	{  0,  1,  1,  0,  1 , 0,  DW1000_PAC8   },
	{  1,  0,  0,  1,  0 , 1,  DW1000_PAC32  },
	{  1,  0,  0,  0,  1 , 0,  DW1000_PAC8   },
	{  1,  1,  1,  1,  0 , 1,  DW1000_PAC32  },
	{  1,  1,  1,  0,  1 , 0,  DW1000_PAC8   }
};

static const byte __pll2_config [2][5] = {
	{ 0x08, 0x05, 0x40, 0x08, 0x26 },	// 4 Ghz, channel 2
	{ 0x1d, 0x04, 0x00, 0x08, 0xa6 }	// 6.5 Ghz, channel 5
};

static const lword __digital_bb_config [2][4] = {
        { 0x311A002D, 0x331A0052, 0x351A009A, 0x371A011D },
        { 0x313B006B, 0x333B00BE, 0x353B015E, 0x373B0296 }
};

#endif

// Default antenna delays (calculation copied from reference driver)
#define	DW1000_64M_RFDELAY	514.462
#define	DW1000_16M_RFDELAY	513.9067

#define DW1000_TIME_UNIT	(1.0/499.2e6/128.0)	// !< = 15.65e-12 s

#define	__dw1000_rfdelay(b)	((word)((((b) / 2.0) * 1e-9) / \
					DW1000_TIME_UNIT))
#define	dw1000_def_rfdelay(prf)	((prf) ? \
				      __dw1000_rfdelay (DW1000_64M_RFDELAY) : \
				      __dw1000_rfdelay (DW1000_16M_RFDELAY) )

// Pulse generator calibration (see txSpectrumConfig in the reference driver)
#define dw1000_def_pgdelay	(mode.channel ? 0xc0 : 0xc2)

// Default TX power (see txSpectrumConfig in the reference driver)
#define	dw1000_def_txpower	(mode.channel ? (mode.prf ? 0x25456585 : \
					0x0E082848) : (mode.prf ? 0x07274767 : \
						0x15355575))

// Copied from lde_replicaCoeff in the reference driver (for preamble codes 9
// and 3
#define	__dw1000_lde_coeff	((word)((mode.precode ? (0.16 * 65536) : \
					(0.32 * 65536))))
// For 110K, it must be divided by 8
#define	dw1000_lde_coeff	(mode.datarate ? __dw1000_lde_coeff : \
					__dw1000_lde_coeff >> 3)

// According to the RD, smart power is enabled when data rate is 6M8
#define	dw1000_use_smartpower	(mode.datarate)

#define	DW1000_LDE_PARAM1	(0x60 | 13)

#define	dw1000_lde_param3	(mode.prf ? 0x0607 : 0x1607)

#define	dw1000_pll2_config	__pll2_config [mode.channel]

#define	DW1000_PLL2_CALCFG	(0x60 | 0x10)

// This is for narrow bandwitdh, which is our case
#define	DW1000_RX_CONFIG	0xd8

#define	dw1000_rf_txctrl	(mode.channel ? 0x001e3fe0 : 0x00045ca0)

#define	dw1000_sftsh		(mode.datarate ? \
					(mode.nssfd ? 0x02 : 0x01) : \
					(mode.nssfd ? 0x16 : 0x0a))

#define	dw1000_dtune1		(mode.prf ? 0x008d : 0x0087)

#define	dw1000_dtune2		__digital_bb_config [mode.prf][mode.pac]

#define	DW1000_AGCTUNE2		0x2502a907

#define	dw1000_agctune1		(mode.prf ? 0x889b : 0x8870)

#define	dw1000_dwnssfdl		(mode.datarate ? 0x08 : 0x40)


###############################################################################
###############################################################################

extern sint __dw1000_v_drvprcs;


// Room for the (tentative) API; for now, I propose to treat this as a sensor
// generating events and returning readings consisting of pairs: Node Id,
// distance (maybe some more, like a timestamp). Probably, after each reading,
// we should reset it for the next measurement.




// Temporary
void dw1000_start (byte, byte, word);

#endif
