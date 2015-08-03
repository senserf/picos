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
#define	DW1000_REG_TX_CAL	0x2a
#define	DW1000_REG_AON		0x2c
#define	DW1000_REG_FS_CTRL	0x2b

// ============================================================================

#define	DW1000_CF_FFEN		0x00000001
#define	DW1000_CF_FFBC		0x00000002
#define	DW1000_CF_FFAD		0x00000008
#define	DW1000_CF_FFAA		0x00000010
#define	DW1000_CF_FFMASK	0x000001FF	// All filtering bits
#define	DW1000_CF_HIRQ_POL	0x00000200	// IRQ polarity is high
#define	DW1000_CF_DIS_DRXB	0x00001000	// Disable dual buffering
#define	DW1000_CF_DIS_STPX	0x00040000	// Disable smart TX power

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

// According to the RD, smart power is enabled when data rate is 6M8
#define	dw1000_use_smartpower	(mode.datarate)

###############################################################################
###############################################################################

// Room for the (tentative) API; for now, I propose to treat this as a sensor
// generating events and returning readings consisting of pairs: Node Id,
// distance (maybe some more, like a timestamp). Probably, after each reading,
// we should reset it for the next measurement.




// Temporary
void dw1000_start (byte, word);

#endif
