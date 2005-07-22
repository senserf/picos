#ifndef	__radio_h
#define	__radio_h

/* ============================================================================ */
/*                       PicOS                                                  */
/*                                                                              */
/* Radio driver                                                                 */
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

#include "sysio.h"

#if	RADIO_TYPE == RADIO_RFMI
/*
 * RFMI
 *
 *      eCOG-J12              BOARD                     GPIO
 *   ================================================================
 *      07 (L4)               PTT (XMT disable Grey)    12   out
 *      08 (L5)               XMT (Red)                 13   out
 *      09 (L6)               RCV (Yellow)              14   in
 *      12 (GND)              GND (Green)
 *
 */

#define	ini_regs		rg.io.gp12_15_out = \
				IO_GP12_15_OUT_SET12_MASK | \
				IO_GP12_15_OUT_EN12_MASK | \
				IO_GP12_15_OUT_CLR13_MASK | \
				IO_GP12_15_OUT_EN13_MASK | \
				IO_GP12_15_OUT_CLR14_MASK | \
				IO_GP12_15_OUT_DIS14_MASK

/* Transmit enable */
#define	xmt_enable_cold()	do { \
					rg.io.gp12_15_out = \
						IO_GP12_15_OUT_CLR12_MASK; \
					udelay (256); \
				} while (0)

#define	xmt_enable_warm()	xmt_enable_cold ()

#define	rcv_enable_cold()	do { \
					rg.io.gp12_15_out = \
						IO_GP12_15_OUT_SET12_MASK; \
				} while (0)

#define	rcv_enable_warm()	rcv_enable_cold()

#define	xcv_sleep()		rcv_enable_cold()

#define	xmt(b)			rg.io.gp12_15_out = (b) ? \
					IO_GP12_15_OUT_SET13_MASK : \
						IO_GP12_15_OUT_CLR13_MASK

#define	rcv			fd.io.gp8_15_sts.sts14
#define	rcvhigh			(rcv == 0)	// 0 means receiving signal

#define	XMS_LOW			0	// Transmitted LOW
#define	XMS_HIGH		1
#define	RCS_LOW			1	// Received LOW (reversed)
#define	RCS_HIGH		0

#define	ini_params()

/* Interrupt locks (interrupts simulated by timer) */
#define	cli_radio		cli_tim
#define	sti_radio		sti_tim

#define	TXL_DEFPRLEN		32	// Double swaps in a sent preamble
#define	DRVDATA_SIZE		7	// Dynamic driver data table size

#define	RADIO_DEF_BITRATE	18000	// Default bit rate (bps)
#define	RCL_DEFPRTRIES		4	// Number of retries for rcvd preamble
#define	RCL_DEFPRWAIT		4	// Wait for preamble high (msec)
#define	RCL_MINPR		2	// Minimum swaps in a received preamble

#endif

#if	RADIO_TYPE == RADIO_XEMICS
/*
 *      eCOG                        XEMICS                   GPIO
 *   ================================================================
 *   J12-06 (L3)                   3,11 (SI+M1)              11   out
 *   J12-07 (L4)                   1,13 (SCK+M2)             12   out
 *   J11-08 (A0) + LED             19,9 (DatIn+M0)           00   out
 *   J12-09 (L6)                   17   (DatOut)             14   in
 *   J12-11 (VCC)                  2    (VDD)                --
 *   J12-12 (GND)                  4    (GND)                --
 *   J11-10 (A2)                   6    (TXAnt)              02   out
 *   J11-11 (A3)                   8    (RXAnt)              03   out
 *   J12-10 (L7)                   15   (DCLK)               15   in
 *   J12-08 (L5)                   5    (SO)                 13   in
 *   J12-01 (D2)                   12   (PATTERN)            21   in
 *   J12-02 (D3)                   7    (EN)                 22   out
 *   ================================================================
 *
 *   J11-09 (A1)   LED             used as reverse receive indicator
 *
 */

#define	ini_regs		do { \
				     rg.io.gp0_3_out = \
					IO_GP0_3_OUT_CLR0_MASK | \
					IO_GP0_3_OUT_EN0_MASK | \
					IO_GP0_3_OUT_CLR1_MASK | \
					IO_GP0_3_OUT_EN1_MASK | \
					IO_GP0_3_OUT_CLR2_MASK | \
					IO_GP0_3_OUT_EN2_MASK | \
					IO_GP0_3_OUT_CLR3_MASK | \
					IO_GP0_3_OUT_EN3_MASK; \
				     rg.io.gp8_11_out = \
					IO_GP8_11_OUT_CLR11_MASK | \
					IO_GP8_11_OUT_EN11_MASK; \
				     rg.io.gp12_15_out = \
					IO_GP12_15_OUT_CLR12_MASK | \
					IO_GP12_15_OUT_EN12_MASK | \
					IO_GP12_15_OUT_CLR13_MASK | \
					IO_GP12_15_OUT_DIS13_MASK | \
					IO_GP12_15_OUT_CLR14_MASK | \
					IO_GP12_15_OUT_DIS14_MASK | \
					IO_GP12_15_OUT_CLR15_MASK | \
					IO_GP12_15_OUT_DIS15_MASK; \
				     rg.io.gp20_23_out = \
					IO_GP20_23_OUT_CLR21_MASK | \
					IO_GP20_23_OUT_DIS21_MASK | \
					IO_GP20_23_OUT_CLR22_MASK | \
					IO_GP20_23_OUT_EN22_MASK ; \
				} while (0)

#define	xem_setant_xmt		rg.io.gp0_3_out = IO_GP0_3_OUT_SET2_MASK
#define	xem_clrant_xmt		rg.io.gp0_3_out = IO_GP0_3_OUT_CLR2_MASK
#define	xem_setant_rcv		rg.io.gp0_3_out = IO_GP0_3_OUT_SET3_MASK
#define	xem_clrant_rcv		rg.io.gp0_3_out = IO_GP0_3_OUT_CLR3_MASK

#define	xem_getso		fd.io.gp8_15_sts.sts13

#define	xmt0			rg.io.gp0_3_out = IO_GP0_3_OUT_CLR0_MASK
#define xmt1			rg.io.gp0_3_out = IO_GP0_3_OUT_SET0_MASK

#define	xmt(b)			rg.io.gp0_3_out = (b) ? \
					IO_GP0_3_OUT_SET0_MASK : \
						IO_GP0_3_OUT_CLR0_MASK

#define	rcv			fd.io.gp8_15_sts.sts14
#define	rcvhigh			(rcv == 0)

#define	XMS_LOW			0	// Transmitted LOW
#define	XMS_HIGH		1
#define	RCS_LOW			1	// Received LOW
#define	RCS_HIGH		0

#define	xem_enlow		do { \
					rg.io.gp20_23_out = \
						IO_GP20_23_OUT_CLR22_MASK; \
					udelay (128); \
				} while (0)

#define	xem_enhigh		do { \
					rg.io.gp20_23_out = \
						IO_GP20_23_OUT_SET22_MASK; \
					udelay (128); \
				} while (0)

#define	xem_setrcvi		rg.io.gp0_3_out = IO_GP0_3_OUT_SET1_MASK
#define	xem_clrrcvi		rg.io.gp0_3_out = IO_GP0_3_OUT_CLR1_MASK

#define	xem_setmode0		rg.io.gp0_3_out = IO_GP0_3_OUT_SET0_MASK
#define	xem_clrmode0		rg.io.gp0_3_out = IO_GP0_3_OUT_CLR0_MASK
#define	xem_setmode1		rg.io.gp8_11_out = IO_GP8_11_OUT_SET11_MASK
#define	xem_clrmode1		rg.io.gp8_11_out = IO_GP8_11_OUT_CLR11_MASK
#define	xem_setmode2		rg.io.gp12_15_out = IO_GP12_15_OUT_SET12_MASK
#define	xem_clrmode2		rg.io.gp12_15_out = IO_GP12_15_OUT_CLR12_MASK

#define	xem_dismodes		do { \
					xem_setsi; \
					xem_setsck; \
					xmt (0); \
				} while (0)

#define	xem_setsi		rg.io.gp8_11_out = IO_GP8_11_OUT_SET11_MASK
#define	xem_clrsi		rg.io.gp8_11_out = IO_GP8_11_OUT_CLR11_MASK
#define	xem_setsck		rg.io.gp12_15_out = IO_GP12_15_OUT_SET12_MASK
#define	xem_clrsck		rg.io.gp12_15_out = IO_GP12_15_OUT_CLR12_MASK

#define	xem_getdclk 		fd.io.gp8_15_sts.sts15

/* Interrupt locks (interrupts go via GPIO 21) */

#define	cli_radio		do { \
					fd.io.gp20_23_cfg.int21 = \
	    					IO_GP20_23_CFG_INT21_NONE; \
				} while (0)

#define	sti_radio		do { \
					fd.io.gp20_23_cfg.int21 = \
    	    					IO_GP20_23_CFG_INT21_EDGE; \
				} while (0)

#define int_radio		fd.io.gp16_23_sts.int21

#define	SERCLOCK		64	/* Serial clock pulse in us */
#define	xem_pulse		udelay (SERCLOCK)
#define	xem_wser0		do { \
					xem_clrsi; xem_setsck; xem_pulse; \
					xem_clrsck; xem_pulse; \
				} while (0)
#define	xem_wser1		do { \
					xem_setsi; xem_setsck; xem_pulse; \
					xem_clrsck; xem_pulse; \
				} while (0)
#define	xem_rser(b,i)		do { \
					xem_setsck; xem_pulse; xem_clrsck; \
					xem_pulse; (b) |= (xem_getso << (i)); \
				} while (0)


/*
 * This number is equal to 1,200,000 / baud_rate, and gives the setting of
 * cnt1 (driven from low-ref PLL with 0 divider) to strobe a single bit.
 */
#define	XMT_DEF_BAUD_COUNT	62

/* ======================= */
/* Configuration registers */
/* ======================= */
#define	XEM_CFG_RTPARAM0	( \
				 (0  << 7) | /* Sensitivity prefered */ \
				 (1  << 6) | /* Bit synchronizer on */ \
				 (1  << 5) | /* RSSI on */ \
				 (0  << 4) | /* FEI off */ \
				 (3  << 2) | /* BB filter bandwidth 200kHz */ \
				 (3  << 0)   /* Transmit power */ \
				)

#define	XEM_CFG_RTPARAM1	( \
				 (0 << 7) | /* Internal freq. ref. */ \
				 (0 << 6) | /* Boost receiver power up */ \
				 (0 << 5) | /* No prefiltering */ \
				 (1 << 4) | /* FEI block (irrelevant) */ \
				 (0 << 3) | /* Rise/fall (irrelevant) */ \
				 (0 << 2) | /* Modulation */ \
				 (0 << 1) | /* Range of RSSI (low) */ \
				 (0 << 0)   /* Clock signal out (for test) */ \
				)

#define	XEM_CFG_FSPARAM0	( \
				 (3 << 6) | /* 902 - 928 MHz */ \
				 (4 << 3) | /* 100kHz deviation */ \
				 (2 << 0)   /* Bit rate = 19.2 */ \
				)

#define	XEM_CFG_FSPARAM1	0	/* F0 = middle of the range */
#define	XEM_CFG_FSPARAM2	0	/* F0 = middle of the range */

#define	XEM_CFG_ADPARAM0	( \
				 (1 << 7) | /* Pattern recognition on */ \
				 (1 << 5) | /* Pattern size = 16 bits */ \
				 (0 << 3) | /* Pattern error tol. */ \
				 (0 << 1) | /* Clock out freq. */ \
				 (0 << 0)   /* IQ amplifiers off */ \
				)

#define	XEM_CFG_ADPARAM1	( \
				 (0 << 7) | /* Reserved */ \
				 (0 << 6) | /* No inversion at receiver */ \
				 (0 << 5) | /* BB filtering off ? */ \
				 (0 << 4) | /* BB filter periodicity */ \
				 (0 << 3) | /* BB filter stuff ? */ \
				 (0 << 2) | /* More */ \
				 (0 << 1) | /* XOSC mode ? */ \
				 (0 << 0)   /* Reserved */ \
				)

#define	XEM_CFG_PATTERN0	0x55
#define	XEM_CFG_PATTERN1	0x55
#define	XEM_CFG_PATTERN2	0x55
#define	XEM_CFG_PATTERN3	0x55

#define	TXL_DEFPRLEN		24	// Double swaps in a sent preamble
/*
 * Driver data size: 11 registers (byte) + rd_bcount + rd_prlen + rd_flags
 */
#define	DRVDATA_SIZE		9

#endif

#define	RF_CHECK		1	// Checksum flag

extern	word	zzz_radiostat;

#endif
