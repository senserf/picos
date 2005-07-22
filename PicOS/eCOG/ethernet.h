#ifndef	__ethernet_h
#define	__ethernet_h

/* ============================================================================ */
/*                       PicOS                                                  */
/*                                                                              */
/* Ethernet chip driver                                                         */
/*                                                                              */
/*                                                                              */
/* Copyright (C) Olsonet Communications Corporation, 2002, 2003                 */
/*                                                                              */
/* Permission is hereby granted, free of charge, to any person obtaining a copy */
/* of this software and associated documentation files (the "Software"), to     */
/* deal in the Software without restriction, including without limitation the   */
/* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or  */
/* sell copies of the Software, and to permit persons to whom the Software is   */
/* furnished to do so, subject to the following conditions:                     */
/*                                                                              */
/* The above copyright notice and this permission notice shall be included in   */
/* all copies or substantial portions of the Software.                          */
/*                                                                              */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  */
/* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER       */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS */
/* IN THE SOFTWARE.                                                             */
/*                                                                              */
/* ============================================================================ */

//+++ "ethernet.c"
//+++ "gpioirq.c"

#include "sysio.h"

/* Bank 0 */
#define	TCR_REG		0
#define	RCR_REG		2
#define	RPC_REG		5	// Receive/Phy Control Register


/* Bank 1 */
#define CONFIG_REG	0
#define	ADDR0_REG	2
#define	ADDR1_REG	3
#define	ADDR2_REG	4
#define	CTL_REG		6

/* Bank 2 */
#define MMU_CMD_REG	0
#define	PN_REG_B	2
#define PTR_REG		3
#define	DATA_REG	4
#define	DATA_REG_B	8
#define	INT_REG		6		// Interrupt status register
#define	INT_REG_B	12		// Interrupt register (byte)
#define	AR_REG		1
#define	AR_REG_B	3
#define	RXFIFO_REG	2

/* Bank 3 */
#define	REV_REG		5
#define	MII_REG		4
#define MII_MSK_CRS100	0x4000 // Disables CRS100 detection during tx half dup
#define MII_MDOE	0x0008 // MII Output Enable
#define MII_MCLK	0x0004 // MII Clock, pin MDCLK
#define MII_MDI		0x0002 // MII Input, pin MDI
#define MII_MDO		0x0001 // MII Output, pin MDO

// PHY Identifier Registers
#define PHY_ID1_REG		0x02	// PHY Identifier 1
#define PHY_ID2_REG		0x03	// PHY Identifier 2

// PHY Control Register
#define PHY_CNTL_REG		0x00
#define PHY_CNTL_RST		0x8000	// 1=PHY Reset
#define PHY_CNTL_LPBK		0x4000	// 1=PHY Loopback
#define PHY_CNTL_SPEED		0x2000	// 1=100Mbps, 0=10Mpbs
#define PHY_CNTL_ANEG_EN	0x1000  // 1=Enable Auto negotiation
#define PHY_CNTL_PDN		0x0800	// 1=PHY Power Down mode
#define PHY_CNTL_MII_DIS	0x0400	// 1=MII 4 bit interface disabled
#define PHY_CNTL_ANEG_RST	0x0200  // 1=Reset Auto negotiate
#define PHY_CNTL_DPLX		0x0100	// 1=Full Duplex, 0=Half Duplex
#define PHY_CNTL_COLTST		0x0080	// 1=MII Colision Test

// PHY Configuration Register 1
#define	PHY_CFG1_REG		0x10
#define	PHY_CFG1_LNKDIS		0x8000

// PHY Status Output (and Interrupt status) Register
#define PHY_INT_REG		0x12	// Status Output (Interrupt Status)
#define PHY_INT_INT		0x8000	// 1=bits have changed since last read
#define	PHY_INT_LNKFAIL		0x4000	// 1=Link Not detected
#define PHY_INT_LOSSSYNC	0x2000	// 1=Descrambler has lost sync
#define PHY_INT_CWRD		0x1000	// 1=Invalid 4B5B code detected on rx
#define PHY_INT_SSD		0x0800	// 1=No Start Of Stream detected on rx
#define PHY_INT_ESD		0x0400	// 1=No End Of Stream detected on rx
#define PHY_INT_RPOL		0x0200	// 1=Reverse Polarity detected
#define PHY_INT_JAB		0x0100	// 1=Jabber detected
#define PHY_INT_SPDDET		0x0080	// 1=100Base-TX mode, 0=10Base-T mode
#define PHY_INT_DPLXDET		0x0040	// 1=Device in Full Duplex

// PHY Status Register
#define PHY_STAT_REG		0x01
#define PHY_STAT_CAP_T4		0x8000	// 1=100Base-T4 capable
#define PHY_STAT_CAP_TXF	0x4000	// 1=100Base-X full duplex capable
#define PHY_STAT_CAP_TXH	0x2000	// 1=100Base-X half duplex capable
#define PHY_STAT_CAP_TF		0x1000	// 1=10Mbps full duplex capable
#define PHY_STAT_CAP_TH		0x0800	// 1=10Mbps half duplex capable
#define PHY_STAT_CAP_SUPR	0x0040	// 1=recv mgmt frames with not preamble
#define PHY_STAT_ANEG_ACK	0x0020	// 1=ANEG has completed
#define PHY_STAT_REM_FLT	0x0010	// 1=Remote Fault detected
#define PHY_STAT_CAP_ANEG	0x0008	// 1=Auto negotiate capable
#define PHY_STAT_LINK		0x0004	// 1=valid link
#define PHY_STAT_JAB		0x0002	// 1=10Mbps jabber condition
#define PHY_STAT_EXREG		0x0001	// 1=extended registers implemented

// PHY Auto-Negotiation Advertisement Register
#define PHY_AD_REG		0x04
#define PHY_AD_NP		0x8000	// 1=PHY requests exchange of Next Page
#define PHY_AD_ACK		0x4000	// 1=got link code word from remote
#define PHY_AD_RF		0x2000	// 1=advertise remote fault
#define PHY_AD_T4		0x0200	// 1=PHY is capable of 100Base-T4
#define PHY_AD_TX_FDX		0x0100	// 1=PHY is capable of 100Base-TX FDPLX
#define PHY_AD_TX_HDX		0x0080	// 1=PHY is capable of 100Base-TX HDPLX
#define PHY_AD_10_FDX		0x0040	// 1=PHY is capable of 10Base-T FDPLX
#define PHY_AD_10_HDX		0x0020	// 1=PHY is capable of 10Base-T HDPLX
#define PHY_AD_CSMA		0x0001	// 1=PHY is capable of 802.3 CMSA

#define CONFIG_DEFAULT	0x8000
#define	TCR_CLEAR	0
#define	TCR_ENABLE	1
#define	RCR_CLEAR	0
#define	RCR_STRIP_CRC	0x0200
#define	RCR_RXEN	0x0100
#define	RCR_SOFTRST	0x8000
#define	RCR_PRMS	0x0002
#define	RCR_ALMUL	0x0004		// Accept all multicast packets
#define CTL_AUTORELEASE	0x0800
#define	MC_RESET	(2<<5)		// Reset MMU to initial state
#define	MC_ALLOC	(1<<5) 		// OR with number of 256 byte packets
#define	MC_ENQUEUE	(6<<5)
#define	MC_RELEASE	(4<<5)
#define	MC_BUSY		1
#define	IM_ALLOC_INT	0x08	 	// Allocation request complete
#define	IM_MDINT	0x80		// PHY MI Register 18 Interrupt
#define	IM_EPH_INT	0x20
#define	IM_RX_OVRN_INT	0x10
#define	IM_RCV_INT	0x01		// Reception
#define AR_FAILED	0x80		// Alocation Failed
#define	PTR_READ	0x2000
#define PTR_RCV		0x8000
#define	PTR_AUTOINC 	0x4000 		// Auto increment the pointer
#define	RXFIFO_EMPTY	0x8000

#define	RS_ALGNERR	0x8000
#define	RS_BADCRC	0x2000
#define	RS_TOOLONG	0x0800
#define	RS_TOOSHORT	0x0400
#define	RS_ERRORS	(RS_ALGNERR | RS_BADCRC | RS_TOOLONG | RS_TOOSHORT)

#define	RPC_SPEED	0x2000	// When 1 PHY is in 100Mbps mode.
#define	RPC_DPLX	0x1000	// When 1 PHY is in Full-Duplex Mode
#define	RPC_ANEG	0x0800	// When 1 PHY is in Auto-Negotiate Mode
#define	RPC_LSXA_SHFT	5	// Bits to shift LS2A,LS1A,LS0A to lsb
#define	RPC_LSXB_SHFT	2	// Bits to get LS2B,LS1B,LS0B to lsb
#define RPC_LED_100_10	(0x00)	// LED = 100Mbps OR's with 10Mbps link detect
#define RPC_LED_RES	(0x01)	// LED = Reserved
#define RPC_LED_10	(0x02)	// LED = 10Mbps link detect
#define RPC_LED_FD	(0x03)	// LED = Full Duplex Mode
#define RPC_LED_TX_RX	(0x04)	// LED = TX or RX packet occurred
#define RPC_LED_100	(0x05)	// LED = 100Mbps link dectect
#define RPC_LED_TX	(0x06)	// LED = TX packet occurred
#define RPC_LED_RX	(0x07)	// LED = RX packet occurred

#define	BANK_SELECT	7

/* ================= */
/* PHY Configuration */
/* ================= */
#define	PHY_MANUAL	0	// 10 Mbps, half-duplex
#define RPC_AUTONEG \
    (RPC_ANEG | (RPC_LED_100 << RPC_LSXA_SHFT) | \
	 (RPC_LED_FD << RPC_LSXB_SHFT) | RPC_SPEED | RPC_DPLX)
#define RPC_MANUAL \
	((RPC_LED_10 << RPC_LSXA_SHFT) | (RPC_LED_TX_RX << RPC_LSXB_SHFT))
#define	RPC_DEFAULT	RPC_MANUAL
#define	PHY_DEFAULT	PHY_MANUAL
#define	RCR_DEFAULT	(RCR_STRIP_CRC | RCR_RXEN | RCR_ALMUL)
/* ================== */

#define	ioaddr		(ETHER_ADDR + 0x0180)
#define	outw(w,a)	(*((volatile word*)(ioaddr + (a))) = (w))
#define	outb(b,a)	(*(((volatile byte*)\
				((word*)ioaddr))+(a)) = (b))
#define inw(a)		(*((volatile word*)(ioaddr + (a))))
#define	inb(a)		(*(((volatile byte*)\
				((word*)ioaddr)) + (a)))
#define	select(x)	outw (x, BANK_SELECT)

#define	macaddr		(zzd_data->h_source)

#define	waitmmu(w)	do { w = inw (MMU_CMD_REG); } while ((w) & MC_BUSY)

#define	swab(w)		{ w = (((w) >> 8) & 0xff) | (((w) & 0xff) << 8); }
#define f_byte(w)	((char)(byte)(w))
#define s_byte(w)	((char)(byte)((w) >> 8))
#define	f_byte_set(w,b)	((w) = (word)(byte)(b))
#define	s_byte_set(w,b)	((w) |= ((word)(byte)(b)) << 8)

/* ========================================================================== */
/* Disable interrupts.   I have to do it this way because the GPIO7 interrupt */
/* line from the chip appears to get stuck at high level after each interrupt */
/* ========================================================================== */
#define	tcv_hard_din		do {\
	fd.io.gp4_7_cfg.int7 =\
	    IO_GP4_7_CFG_INT7_NONE;\
	fd.io.gp4_7_out.en7 = 1;\
	fd.io.gp4_7_out.clr7 = 1;\
				} while (0)
/* ================= */
/* Enable interrupts */
/* ================= */
#define	tcv_hard_eni		do {\
	fd.io.gp4_7_out.dis7 = 1;\
	fd.io.gp4_7_cfg.int7 =\
    	    IO_GP4_7_CFG_INT7_HIGH;\
				} while (0)

#define	tcv_interrupt	fd.io.gp0_7_sts.sts7

#define	ETH_PTYPE	0x6006	/* Packet type */
#define	ETH_MCAST	{0xE1AA,0xBBCC,0xDDEE}
#define	ETH_DEST	ETH_MCAST
#define	ETH_ZLEN	60	/* Minimum octets/frame - FCS */
#define	ETH_MXLEN	1514	/* Maximum octets/frame - FCS */

#define	FLG_ENRCV	0x0001	/* Enable RCV interrupt */
#define	FLG_ENXMT	0x0008	/* Enable buffer memory available interrupt */
#define	FLG_DSTOK	0x8000	/* Dest (server) address valid */
#define	FLG_RCOOKED	0x4000	/* Last read packet was cooked */

#define	PTYPE_UP	0x8000	/* Uplink flag */
#define	PTYPE_PTP	0x7f00	/* Packet type mask */
#define	PTYPE_CNT	0x00ff	/* ID count */

#define	ERR_FORMAT	0xffff	/* Packet format error in cooked mode */

typedef struct {
/*
 * Driver data.
 */
	word	cardid;			/* Card Id for cooked mode */
	word	h_source [3];		/* our MAC address  */
	word	h_dest [3];		/* server address (cooked mode) */
	word 	flags;			/* Which interrupts to enable */
	byte	readmode, writemode;
	word	error;			/* Receive error status */
	word	scratch [3];
} ddata_t;

extern ddata_t  *zzd_data;

#define soft_din	do { outw (0, INT_REG); tcv_hard_din; } while (0)
#define soft_eni	do { \
				tcv_hard_eni; \
				outw (zzd_data->flags << 8, INT_REG); \
			} while (0)
#endif
