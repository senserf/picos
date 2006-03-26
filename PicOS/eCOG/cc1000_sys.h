#ifndef	__pg_cc1000_sys_h
#define	__pg_cc1000_sys_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ "gpioirq.c"

/* ================================================================== */

/*
 * Pin assignment:
 *
 *      eCOG                        CC1000                  GPIO
 *   ===================================================================
 *   J12-06 (L3)                   PALE                      11   out
 *   J12-07 (L4)                   DIO                       12   in/out
 *   J12-08 (L5)                   DCLK                      13   in
 *   J12-09 (L6)                   PCLK                      14   out
 *   J12-10 (L7)                   PDATA                     15   in/out
 *   J12-11 VCC                    3V                        --
 *   J12-12 GND                    GND                       --
 *   J23                           RSSI                      --
 *   ===================================================================
 *
 */

#define	ini_regs		do { \
				     rg.io.gp8_11_out = \
					IO_GP8_11_OUT_CLR11_MASK | \
					IO_GP8_11_OUT_EN11_MASK; \
				     rg.io.gp12_15_out = \
					IO_GP12_15_OUT_CLR12_MASK | \
					IO_GP12_15_OUT_CLR15_MASK | \
					IO_GP12_15_OUT_CLR13_MASK | \
					IO_GP12_15_OUT_DIS13_MASK | \
					IO_GP12_15_OUT_CLR14_MASK | \
					IO_GP12_15_OUT_EN14_MASK; \
				} while (0)

#define	GP_INT_RCV	0x00d0	// Rising edge on GPIO13
#define	GP_INT_XMT	0x00c0	// Falling edge on GPIO13
#define	GP_INT_MSK	0x00f0

#define	clr_xcv_int		(rg.io.gp12_15_cfg &= ~GP_INT_MSK)
#define	set_rcv_int		(rg.io.gp12_15_cfg |= GP_INT_RCV)
#define	set_xmt_int		(rg.io.gp12_15_cfg |= GP_INT_XMT)
#define	cc1000_int		(rg.io.gp8_15_sts & 0x0800)
#define	clear_cc1000_int	do { } while (0)

/*
 * CC1000 signal operations
 */
#define	chp_paledown	rg.io.gp8_11_out = IO_GP8_11_OUT_CLR11_MASK
#define	chp_paleup	rg.io.gp8_11_out = IO_GP8_11_OUT_SET11_MASK
#define	chp_pclkdown	rg.io.gp12_15_out = IO_GP12_15_OUT_CLR14_MASK
#define	chp_pclkup	rg.io.gp12_15_out = IO_GP12_15_OUT_SET14_MASK
#define chp_pdirout	rg.io.gp12_15_out = IO_GP12_15_OUT_EN15_MASK
#define chp_pdirin	rg.io.gp12_15_out = IO_GP12_15_OUT_DIS15_MASK

#define chp_pdioout	do { \
				rg.io.gp12_15_out = IO_GP12_15_OUT_CLR12_MASK; \
				rg.io.gp12_15_out = IO_GP12_15_OUT_EN12_MASK; \
			} while (0)

#define chp_pdioin	rg.io.gp12_15_out = IO_GP12_15_OUT_DIS12_MASK

#define chp_getpbit	fd.io.gp8_15_sts.sts15
#define	chp_outpbit(b)	rg.io.gp12_15_out = (b) ? \
				IO_GP12_15_OUT_SET15_MASK : \
					IO_GP12_15_OUT_CLR15_MASK

#define	chp_getdbit	(rg.io.gp8_15_sts & 0x0100)
#define	chp_setdbit	(rg.io.gp12_15_out = IO_GP12_15_OUT_SET12_MASK)
#define	chp_clrdbit	(rg.io.gp12_15_out = IO_GP12_15_OUT_CLR12_MASK)

#define	hard_lock	clr_xcv_int
#define	hard_drop	rg.io.gp12_15_cfg |= zzv_status

/*
 * 0xc mean ANA0 - ANA1. ANA1 is used for calibration.
 */
#define	adc_config	do { \
				fd.ssm.ex_ctrl.adc_rst_set = 1; \
				rg.adc.cfg = 0xc; \
				fd.ssm.ex_ctrl.adc_rst_clr = 1; \
			} while (0)

#define	adc_start	(fd.ssm.cfg.adc_en = 1)
#define	adc_stop	do { } while (0)
#define	adc_disable	(fd.ssm.cfg.adc_en = 0)
#define	adc_value	fd.adc.sts.data
#define	adc_wait	do { } while (0)

#define	RSSI_MIN	0xf800	// Minimum and maximum RSSI values (for scaling)
#define	RSSI_MAX	0x07ff
#define	RSSI_SHF	4	// Shift bits to fit into a (unsigned) byte

#define	disable_xcv_timer	do { } while (0)

#endif
