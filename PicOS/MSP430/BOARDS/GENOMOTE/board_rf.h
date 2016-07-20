//    Make sure P1 interrupts are serviced
//+++ "p1irq.c"

REQUEST_EXTERNAL (p1irq);	// Needed to force the file to be loaded
				// if loading from library

// Register access:
//
//	bit 7 cleared for REG access (6-bit register number)
//	bit 6 R/W
//	registers are 16 bit

#define	cc2420_ini_regs		CNOP

// P1.0		FIFOP
// P1.3		FIFO
// P1.4		CCA
#define	cc2420_fifop_val	(P1IN & 0x01)
#define	cc2420_fifo_val		(P1IN & 0x08)
#define	cc2420_cca_val		(P1IN & 0x10)

// P3.1		SI
// P3.2		SO	(drive [low] when CSN is up)
// P3.3		CLK
#define	cc2420_so_val		(P3IN & 0x04)
#define	cc2420_si_up		_BIS (P3OUT, 0x02)
#define	cc2420_si_down		_BIC (P3OUT, 0x02)
#define	cc2420_sclk_up		_BIS (P3OUT, 0x08)
#define	cc2420_sclk_down	_BIC (P3OUT, 0x08)

// P4.1		SFD
// P4.2		CS
// P4.5		VRGEN_EN
// P4.6		RESET
#define	cc2420_sfd_val		(P4IN & 0x02)
#define	cc2420_csn_up		_BIS (P4OUT, 0x04)
#define	cc2420_csn_down		_BIC (P4OUT, 0x04)
#define	cc2420_vrgen_up		_BIS (P4OUT, 0x20)
#define	cc2420_vrgen_down	_BIC (P4OUT, 0x20)
#define	cc2420_reset_up		_BIS (P4OUT, 0x40)
#define	cc2420_reset_down	_BIC (P4OUT, 0x40)

// ============================================================================

#define	cc2420_int		(P1IFG & 0x01)
#define	cc2420_clear_int	_BIC (P1IFG, 0x01)

#define cc2420_rcv_int_enable	do { \
					_BIS (P1IE, 0x01); \
					if (cc2420_fifop_val) \
						_BIS (P1IFG, 0x01); \
				} while (0)
						
#define cc2420_rcv_int_disable		_BIC (P1IE, 0x01)
