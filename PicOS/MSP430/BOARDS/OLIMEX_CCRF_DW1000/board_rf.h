//
// This is for DW1000, CC430 parameters are hardwired
//

// MOSI/CLK set to output (they are always parked low),
// CS set to output (parked low); make sure to set MOSI low
// before pulling CS down
#define	DW1000_SPI_START	do { \
					_BIS (P1DIR, 0x18); \
					_BIS (P3DIR, 0x04); \
				} while (0)
				
// CS set to input, pulled high, MOSI/CLK set to input
// (CLK is low at the end, MOSI parked low)				
#define	DW1000_SPI_STOP		do { \
					_BIC (P3DIR, 0x04); \
					_BIC (P1DIR, 0x18); \
					_BIC (P1OUT, 0x18); \
				} while (0)
				
#define	dw1000_sclk_up		_BIS (P1OUT, 0x10)
#define	dw1000_sclk_down	_BIC (P1OUT, 0x10)

#define	dw1000_so_val		(P1IN & 0x04)

#define	dw1000_si_up		_BIS (P1OUT, 0x08)
#define	dw1000_si_down		_BIC (P1OUT, 0x08)

#define	dw1000_int		(P2IFG & 0x04)
#define	dw1000_clear_int	_BIC (P2IFG, 0x04)
#define	DW1000_INT_PENDING	(P2IN & 0x04)

#define	dw1000_int_enable	do { \
					_BIS (P2IE, 0x04); \
					if (DW1000_INT_PENDING) \
						_BIS (P2IFG, 0x04); \
				} while (0)

#define	dw1000_int_disable	_BIC (P2IE, 0x04)

// The delay is probably redundant, the manual says that 10ns is enough
#define	DW1000_RESET		do { \
					_BIS (P3DIR, 0x01); \
					udelay (10); \
					_BIC (P3DIR, 0x01); \
				} while (0)

// Used after wakeup from lp to assess whether the chip is ready to go
#define	dw1000_ready		(P3IN & 0x01)
