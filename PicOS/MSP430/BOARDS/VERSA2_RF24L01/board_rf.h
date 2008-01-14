//+++ "p1irq.c"

REQUEST_EXTERNAL (p1irq);

/* ================================================================== */

/*
 * Pin assignment: 
 *
 *  RF24L01                     MSP430Fxxx
 * ===========================================
 *  CE	   Chip Enable		P6.3	GP3	OUT
 *  CSN	   Chip Select		P6.4	GP4	OUT
 *  SCK	   SPI clock		P6.5	GP5	OUT
 *  MOSI   Data out (chip IN)   P6.6    GP6     OUT
 *  MISO   Data in (chip OUT)   P6.7    GP7     IN
 *  IRQ    Interrupt            P1.0    CFG0    IN
 */

#define	ini_regs	do { \
				_BIC (P6OUT, 0xff); \
				_BIS (P6DIR, 0x78); \
				_BIC (P6DIR, 0x80); \
				_BIC (P1DIR, 0x01); \
				_BIS (P1IES, 0x01); \
			} while (0)

#define	rf24l01_int		(P1IFG & 0x01)
#define	clear_rf24l01_int	P1IFG &= ~0x01
#define	set_rcv_int		_BIS (P1IE, 0x01)
#define	clr_rcv_int		_BIC (P1IE, 0x01)

#define	sck_up			_BIS (P6OUT, 0x20)
#define	sck_down		_BIC (P6OUT, 0x20)

#define	ce_up			_BIS (P6OUT, 0x08)
#define	ce_down			_BIC (P6OUT, 0x08)
#define	ce_val			(P6OUT & 0x08)

#define	csn_up			_BIS (P6OUT, 0x10)
#define	csn_down		_BIC (P6OUT, 0x10)

#define	data_up			_BIS (P6OUT, 0x40)
#define	data_down		_BIC (P6OUT, 0x40)

#define	data_val		(P6IN & 0x80)
