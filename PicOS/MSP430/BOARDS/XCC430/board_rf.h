//
// This is for CC430, CC110x
//

//+++ "irq_cc430_rf.c"

// This is not needed, everything initialized in board_pins.h
#define	ini_regs		CNOP

// We are using the core interrupts triggered by CFG0 on packet reception; note
// that the default edge (up) is OK
#define	cc1100_int		(RF1AIFG & 0x01)

#define	clear_cc1100_int	_BIC (RF1AIFG, 0x01)

// This is my mildly educated guess; the manual really sucks at this point
#define	RX_FIFO_READY		(RF1AIN & 0x01)

#define rcv_enable_int		do { \
					_BIS (RF1AIE, 0x01); \
					if (RX_FIFO_READY) \
						_BIS (RF1AIFG, 0x01); \
				} while (0)
						
#define rcv_disable_int		_BIC (RF1AIE, 0x01)
