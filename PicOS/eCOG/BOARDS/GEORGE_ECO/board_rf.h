/*
 * RF pins CC1000 connected to George's board
 */

/*
 * Pin assignment:
 *
 *      eCOG                        CC1000                  GPIO
 *   ===================================================================
 *   (A0)                   LNA                        0   out
 *   (L0)                   RXR                        8   out
 *   (L1)                   TXR                        9   out
 *   (L2)                   CHP_OUT                   10   in (unused)
 *   (L3)                   PALE                      11   out
 *   (L4)                   DIO                       12   in/out
 *   (L5)                   DCLK                      13   in
 *   (L6)                   PCLK                      14   out
 *   (L7)                   PDATA                     15   in/out
 *   ANA0                   RSSI                      --
 *   ===================================================================
 *
 */
#define	ini_regs		do { \
				     rg.io.gp0_3_out = \
					IO_GP0_3_OUT_SET0_MASK | \
					IO_GP0_3_OUT_EN0_MASK; \
				     rg.io.gp8_11_out = \
					IO_GP8_11_OUT_CLR8_MASK | \
					IO_GP8_11_OUT_CLR9_MASK | \
					IO_GP8_11_OUT_CLR11_MASK | \
					IO_GP8_11_OUT_EN8_MASK | \
					IO_GP8_11_OUT_EN9_MASK | \
					IO_GP8_11_OUT_DIS10_MASK | \
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

#define	lna_on 			(rg.io.gp0_3_out = IO_GP0_3_OUT_CLR0_MASK)
#define	lna_off			(rg.io.gp0_3_out = IO_GP0_3_OUT_SET0_MASK)

#define	pins_rf_enable_rcv	do { \
				  lna_on; \
				  rg.io.gp8_11_out = IO_GP8_11_OUT_SET8_MASK | \
				  		     IO_GP8_11_OUT_CLR9_MASK; \
				} while (0)

#define	pins_rf_enable_xmt	do { \
				  rg.io.gp8_11_out = IO_GP8_11_OUT_CLR8_MASK | \
				  		     IO_GP8_11_OUT_SET9_MASK; \
				} while (0)

#define	pins_rf_disable		do { lna_off; \
				  rg.io.gp8_11_out = IO_GP8_11_OUT_CLR8_MASK | \
				  		     IO_GP8_11_OUT_CLR9_MASK; \
				} while (0)



#define	clr_xcv_int		(rg.io.gp12_15_cfg &= ~GP_INT_MSK)
#define	set_rcv_int		(rg.io.gp12_15_cfg |= GP_INT_RCV)
#define	set_xmt_int		(rg.io.gp12_15_cfg |= GP_INT_XMT)
#define	cc1000_int		(rg.io.gp8_15_sts & 0x0800)

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

#define	hard_drop	rg.io.gp12_15_cfg |= __pi_v_status

/*
 * It used to be 0xc (for adc.cgf) meaning ANA0 - ANA1, with ANA1 used for
 * calibration. George uses Vref for the reference.
 */

// This is the 'cfg' field, i.e., ANA0 - VRef
#define	PIN_ADC_RSSI	0x00

// George has recalibrated the RSSI, such that the maximum voltage sets the
// value to 4095 and the MIN is around 2630, so this is the new scale. It used
// to be RSSI_MIN = 0, RSSI_MAX = 1490

#define	RSSI_MIN	(2800-2048)
#define	RSSI_MAX	(3500-2048)
#define	RSSI_SHF	4	// Shift bits to fit into a (unsigned) byte
