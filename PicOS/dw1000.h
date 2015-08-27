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
#define	DW1000_OPTIONS		0x0000
#endif

#define	DW1000_OPT_DEBUG	0x01
#define	DW1000_OPT_SHORT_STAMPS	0x02
#define	DW1000_OPT_NO_ANT_DELAY	0x04
#define	DW1000_OPT_SHORT_RESULT	0x08

// The length of time stamp in bytes
#if (DW1000_OPTIONS & DW1000_OPT_SHORT_STAMPS)
#define	DW1000_TSTAMP_LEN	4
#else
#define	DW1000_TSTAMP_LEN	5
#endif

// ============================================================================
// Registers
// ============================================================================

#define	DW1000_REG_DEVID	0x00
#define	DW1000_REG_EUI		0x01
#define	DW1000_REG_PANADR	0x03
#define	DW1000_REG_SYS_CFG	0x04
#define	DW1000_REG_TX_POWER	0x1e
#define	DW1000_REG_OTP_IF	0x2d		// OTP control
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
#define	DW1000_REG_RX_BUFFER	0x11
#define	DW1000_REG_TX_BUFFER	0x09
#define	DW1000_REG_SYS_STATUS	0x0f
#define	DW1000_REG_SYS_CTRL	0x0d
#define	DW1000_REG_SYS_MASK	0x0e
#define	DW1000_REG_RX_TIME	0x15
#define	DW1000_REG_TX_TIME	0x17
#define	DW1000_REG_DX_TIME	0x0a
#define	DW1000_REG_TX_ANTD	0x18
#define	DW1000_REG_SYS_TIME	0x06

// ============================================================================
// Interrupt flags
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

// These that are not auto-cleared on sane actions
#define	DW1000_IRQ_OTHERS	(DW1000_IRQ_CLKPLL_LL	|\
				 DW1000_IRQ_RFPLL_LL	|\
				 DW1000_IRQ_CPLOCK	|\
				 DW1000_IRQ_ESYNCR	|\
				 DW1000_IRQ_GPIOIRQ	|\
				 DW1000_IRQ_TXBERR)
				 

// IRQs to enable when waiting for reception; I am not sure if the RXAUTR flag
// works as explained in the manual (the reference driver mentions a bug); if
// it does, the single RXFCG IRQ should do, and the getevent function can be
// simplified; otherwise, we have to include more flags here and, probably,
// complicate getevent even more
#define	DW1000_IRQ_RECEIVE	(DW1000_IRQ_RXFCG)

// IRQs to enable when waiting for the end of transmissions
#define	DW1000_IRQ_TRANSMIT	(DW1000_IRQ_TXFRS)

// ============================================================================
// Event types returned by getevent after an interrupt
// ============================================================================

#define	DW1000_EVT_BAD		0x80		// Error
#define	DW1000_EVT_XMT		0x81		// Xmit done
#define	DW1000_EVT_TMO		0x82		// Timeout
#define	DW1000_EVT_NIL		0x00		// Void
// Note: after RX, the function returns frame length

// ============================================================================
// Configuration flags
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
// Some OTP addresses
// ============================================================================

#define	DW1000_ADDR_LDOTUNE	0x04
#define	DW1000_ADDR_ANTDELAY	0x1c
#define	DW1000_ADDR_XTRIM	0x1e
#define	DW1000_ADDR_TXCONF	0x10

// ============================================================================
// AON flags 
// ============================================================================

#define	DW1000_ONW_LDC		0x0040		// AON_WCFG load user config
#define	DW1000_ONW_L64P		0x0080		// AON_WCFG L64P
#define	DW1000_ONW_PRES_SLEEP	0x0100		// AON_WCFG preserve sleep cnf.
#define	DW1000_ONW_LLDE		0x0800		// AON_WCFG load microcode
#define	DW1000_ONW_LLDO		0x1000		// AON_WCFG load LDOTUNE
#define	DW1000_ONW_RX		0x0002		// AON_WCFG start in RX mode

#define	DW1000_CF0_SLEEP_EN	0x01		// AON_CF0 sleep enable
#define	DW1000_CF0_WAKE_SPI	0x04		// AON_CF0 wake on SPI

// ============================================================================
// Constants for selecting the bit rate for data TX (and RX)
// ============================================================================

#define DW1000_BR_110K		0	// UWB bit rate 110 kbits/s
#define DW1000_BR_850K		1	// UWB bit rate 850 kbits/s
#define DW1000_BR_6M8		2	// UWB bit rate 6.8 Mbits/s

// ============================================================================
// Constants for specifying the (nominal) mean Pulse Repetition Frequency
// ============================================================================

#define DW1000_PRF_16M		1	//!< UWB PRF 16 MHz
#define DW1000_PRF_64M		2	//!< UWB PRF 64 MHz

// ============================================================================
// Constants for specifying Preamble Acquisition Chunk (PAC) Size in symbols
// ============================================================================

#define DW1000_PAC8		0	//!< PAC  8 (preamble 128 and below)
#define DW1000_PAC16		1	//!< PAC 16 (256)
#define DW1000_PAC32		2	//!< PAC 32 (512)
#define DW1000_PAC64		3	//!< PAC 64 (1024 and up)

// ============================================================================
// Constants for specifying TX Preamble length in symbols defined to allow
// them be directly written into byte 2 of the TX_FCTRL register (i.e. a four
// bit value destined for bits 20..18 but shifted left by 2 for byte alignment)
// ============================================================================

#define DW1000_PLEN_4096 0x0C	// Standard preamble length 4096 symbols
#define DW1000_PLEN_2048 0x28	// Non-standard preamble length 2048 symbols
#define DW1000_PLEN_1536 0x18	// Non-standard preamble length 1536 symbols
#define DW1000_PLEN_1024 0x08	// Standard preamble length 1024 symbols
#define DW1000_PLEN_512	 0x34	// Non-standard preamble length 512 symbols
#define DW1000_PLEN_256	 0x24	// Non-standard preamble length 256 symbols
#define DW1000_PLEN_128	 0x14	// Non-standard preamble length 128 symbols
#define DW1000_PLEN_64	 0x04	// Standard preamble length 64 symbols

// ============================================================================
// Compressed (to one byte) data structure representing the various RF configs
// which I've basically copied (the configs, not the structure) from the
// reference driver. As the paramaters of those configurations come from a
// reduced set, I was able to use bits and one short field instead of full
// values
// ============================================================================
typedef struct {
	byte	channel:1,	// Boolean: 0 - 2, 1 - 5
		prf:1,		// PRF code, also Boolean: 0 - 16M, 1 - 64M
		precode:1,	// Preamble code: 0 - 3, 1 - 9
		nssfd:1,	// Boolean: non-standard SFD
		datarate:1,	// Boolean: 0 - 110K, 1 - 6M8
		preamble:1,	// Boolean: 0 - 128, 1 - 1024
		pacsize:2;	// PAC selection: 0 through 3
} chconfig_t;

// Preamble selection determines SFD timeout: 0 - 129, 1 - 1057, so there is no
// need to store it (sfdto) in the structure
#define	dw1000_sfdto		(mode.preamble ? 1057 : 129)

#define	dw1000_channel		(mode.channel ? 5 : 2)
#define	dw1000_precode		(mode.precode ? 9 : 3)
#define	dw1000_prf		(mode.prf ? DW1000_PRF_64M : DW1000_PRF_16M)
#define	dw1000_preamble		(mode.preamble ? DW1000_PLEN_1024 : \
					DW1000_PLEN_128)
#define	dw1000_datarate		(mode.datarate ? DW1000_BR_6M8 : DW1000_BR_110K)

// ============================================================================

#ifdef	DW1000_DEFINE_RF_SETTINGS

// ============================================================================
// The configuration options to choose from; I am copying them creatively
// from Deca driver (8 options). We may settle later on something fixed.
// ============================================================================

// Options: 0, 2-7 work, 2 produces funny TRP sometimes, as if the reception
// of POLL at anchor couldn't be properly timed

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

// RF PLL configuration values: five values per channel
static const byte __pll2_config [2][5] = {
	{ 0x08, 0x05, 0x40, 0x08, 0x26 },	// 4 Ghz, channel 2
	{ 0x1d, 0x04, 0x00, 0x08, 0xa6 }	// 6.5 Ghz, channel 5
};

// dtune2 values, one value for each PRF/PAC pair
static const lword __digital_bb_config [2][4] = {
        { 0x311A002D, 0x331A0052, 0x351A009A, 0x371A011D },
        { 0x313B006B, 0x333B00BE, 0x353B015E, 0x373B0296 }
};

#endif

// ============================================================================
// Default antenna delays (calculation copied from reference driver)
// ============================================================================

#define	DW1000_64M_RFDELAY	514.462
#define	DW1000_16M_RFDELAY	513.9067

// Time resolution for packet flight timing
#define DW1000_TIME_UNIT	(1.0/499.2e6/128.0)	// = 15.65e-12 s

#define	__dw1000_rfdelay(b)	((word)((((b) / 2.0) * 1e-9) / \
					DW1000_TIME_UNIT))
#define	dw1000_def_rfdelay(prf)	((prf) ? \
				      __dw1000_rfdelay (DW1000_64M_RFDELAY) : \
				      __dw1000_rfdelay (DW1000_16M_RFDELAY) )

// Pulse generator calibration
#define dw1000_def_pgdelay	(mode.channel ? 0xc0 : 0xc2)

// Default TX power
#define	dw1000_def_txpower	(mode.channel ? (mode.prf ? 0x25456585 : \
					0x0E082848) : (mode.prf ? 0x07274767 : \
						0x15355575))

// According to the RD, smart power is enabled when data rate is 6M8
#define	dw1000_use_smartpower	(mode.datarate)

// ============================================================================
// LDE configuration parameters
// ============================================================================

#define	__dw1000_lde_coeff	((word)((mode.precode ? (0.16 * 65536) : \
					(0.32 * 65536))))
// For 110K, it must be divided by 8
#define	dw1000_lde_coeff	(mode.datarate ? __dw1000_lde_coeff : \
					__dw1000_lde_coeff >> 3)

#define	DW1000_LDE_PARAM1	(0x60 | 13)

#define	dw1000_lde_param3	(mode.prf ? 0x0607 : 0x1607)

#define	dw1000_pll2_config	__pll2_config [mode.channel]

#define	DW1000_PLL2_CALCFG	(0x60 | 0x10)

// ============================================================================
// Receiver configuration params
// ============================================================================

#define	DW1000_RX_CONFIG	0xd8

#define	dw1000_rf_txctrl	(mode.channel ? 0x001e3fe0 : 0x00045ca0)

#define	dw1000_sftsh		(mode.datarate ? \
					(mode.nssfd ? 0x02 : 0x01) : \
					(mode.nssfd ? 0x16 : 0x0a))

#define	dw1000_dtune1		(mode.prf ? 0x008d : 0x0087)

#define	dw1000_dtune2		__digital_bb_config [mode.prf][mode.pacsize]

#define	DW1000_AGCTUNE2		0x2502a907

#define	dw1000_agctune1		(mode.prf ? 0x889b : 0x8870)

#define	dw1000_dwnssfdl		(mode.datarate ? 0x08 : 0x40)

// ============================================================================
// Handshake frame lengths (including FCS)
// ============================================================================

#define	DW1000_FRLEN_TPOLL	9
#define	DW1000_FRLEN_ARESP	11
#define	DW1000_FRLEN_TFIN	24

// ============================================================================

extern sint __dw1000_v_drvprcs;

// ============================================================================
// Flags
// ============================================================================

#define	DW1000_FLG_ANCHOR	0x01	// Anchor mode
#define	DW1000_FLG_LDREADY	0x02	// Location data ready
#define	DW1000_FLG_ACTIVE	0x04	// Module active
#define	DW1000_FLG_REVERTPD	0x08	// Revert to powerdown when done

typedef struct {
//
// The things are arranged such that the second significant byte of
// TRR, which we need as the base for calculating the transmit time of FIN,
// is aligned at 2-byte boundary, so we can use it as a longword in direct
// arithmetic
//
	word tag;				// Source
	byte tst [6*DW1000_TSTAMP_LEN];		// Six time stamps
	byte seq;				// Sequence number

} dw1000_locdata_t;

// Delay after formal wakeup until we can say that the clock has stabilized
// (in mdelay milliseconds); note: the reference driver uses some long delay
// of order 80 milliseconds or so; I see that things appear to work with 0,
// but am not sure if this doesn't affect the accuracy or something
#define	DW1000_WAKEUP_TIME	0

// Offsets into time stamps
#define	DW1000_TSOFF_TSP	(DW1000_TSTAMP_LEN * 0)
#define	DW1000_TSOFF_TRR	(DW1000_TSTAMP_LEN * 1)
#define	DW1000_TSOFF_TSF	(DW1000_TSTAMP_LEN * 2)
#define	DW1000_TSOFF_TRP	(DW1000_TSTAMP_LEN * 3)
#define	DW1000_TSOFF_TSR	(DW1000_TSTAMP_LEN * 4)
#define	DW1000_TSOFF_TRF	(DW1000_TSTAMP_LEN * 5)

// PicOS timeouts: anchor waiting for FIN following sending RESP
#define	DW1000_TMOUT_FIN	100

// This cannot be shortened any more
#define	DW1000_TMOUT_ARESP	7

// The resolution of time stamps is 1/(128*499.2*10^6) seconds, which means
// 1/63897600000 seconds, or 1.565 * 10^-11 seconds. A 40-bit timer covers
// about 17.2 seconds (this is the wrap around time). I have considered using
// only 4 least significant bytes, which is about 67ms. Considering that a
// handshake takes ca. 10ms, this may work, so perhaps we should try.

// Fixed offset from RESP reception to FIN TX. This seems to be at the minimum
// (my attempts to reduce below these values haven't succeeded). When we remove
// the least significant byte (for FIN time calculation), we get ca. 4 * 10^-9,
// i.e., 4 nanoseconds. This is the unit of the processing delay: here the
// first part is in microseconds, the 250 is to bring the 4ns to a millisecond.
// DW1000_FIN_DELAY can be reduced to 2500 when the data rate is 6.8M.
#if DW1000_USE_SPI
#define	DW1000_FIN_DELAY	(3000L * 250L)
#else
#define	DW1000_FIN_DELAY	(4000L * 250L)
#endif

// The maximum number of tries for Tag polls until the successful transmission
// of FIN
#define	DW1000_MAX_TTRIES	4

// Inter try delay (PicOS milliseconds)
#define	DW1000_TPOLL_DELAY	10

// The (tentative) API

void dw1000_start (byte, byte, word);
void dw1000_stop ();
void dw1000_read (word, const byte*, address);
void dw1000_write (word, const byte*, address);

#if (DW1000_OPTIONS & 0x0001)
void chip_read (byte, word, word, byte*);
void chip_write (byte, word, word, byte*);
#endif

#endif
