/*
 * RF pins CC1000 connected to George's board
 */

/*
 * Pin assignment:
 *
 *      eCOG                        CC1000                  GPIO
 *   ===================================================================
 *   (L3)                   PALE                      11   out
 *   (L4)                   DIO                       12   in/out
 *   (L5)                   DCLK                      13   in
 *   (L6)                   PCLK                      14   out
 *   (L7)                   PDATA                     15   in/out
 *   ANA0                   RSSI                      --
 *   ===================================================================
 *
 */

// Note: for low power operation, you should avoid dangling input pins.
// My tests indicate that as such pins float, current drain jumps to about
// 80-130uA, which effect disappears when the pins are grounded, even via
// a relatively large resistor (like 220K). Thus, I am assuming that the
// default setting of bidirectional pins is OUTPUT, supposing that the
// RF chip will turn their ends to high impedance in PD mode (so I don't
// have to /shouldn't/ worry about their settings). Also, I am assuming
// that the chip will make sure to set all of its output pins to a definite
// low-impedance state, such that they won't float.

// Upon init, DIO and PDATA become output low
#define	ini_regs		do { \
				     rg.io.gp8_11_out = \
					IO_GP8_11_OUT_CLR11_MASK | \
					IO_GP8_11_OUT_EN11_MASK; \
				     rg.io.gp12_15_out = \
					IO_GP12_15_OUT_CLR12_MASK | \
					IO_GP12_15_OUT_EN12_MASK  | \
					IO_GP12_15_OUT_CLR15_MASK | \
					IO_GP12_15_OUT_EN15_MASK  | \
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

#define	hard_drop	rg.io.gp12_15_cfg |= zzv_status

// This is the 'cfg' field, i.e., ANA0 - VRef
#define	PIN_ADC_RSSI	0x00

// George has recalibrated the RSSI, such that the maximum voltage sets the
// value to 4095 and the MIN is around 2630, so this is the new scale. It used
// to be RSSI_MIN = 0, RSSI_MAX = 1490

#define	RSSI_MIN	(2800-2048)
#define	RSSI_MAX	(3500-2048)
#define	RSSI_SHF	4	// Shift bits to fit into a (unsigned) byte
