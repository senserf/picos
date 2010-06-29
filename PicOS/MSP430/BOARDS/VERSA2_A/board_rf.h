//+++ "dm2200irq.c"
//+++ "p2irq.c"

#ifndef	FCC_TEST_MODE
#define	FCC_TEST_MODE	0
#endif

REQUEST_EXTERNAL (p2irq);

#define	CFG_P1		CNOP

#define	CFG_P2		_BIS (P2DIR, 0x81); \
			_BIS (P2SEL, 0x80)

#define	CFG_P3		CNOP

#define	CFG_P4		_BIS (P4DIR, 0x01); \
			_BIC (P4OUT, 0x01)

#define	CFG_P5		_BIS (P5DIR, 0x02)

#define	CFG_P6		CNOP

#define	ini_regs	do { CFG_P1; CFG_P2; CFG_P3; CFG_P4; CFG_P5; CFG_P6; } \
			while (0)
#if FCC_TEST_MODE
#define	fcc_test_send		((P1IN & 0x01) != 0)
#endif

/*
 * DM2200 signal operations. Timer's A Capture/Compare Block is used for signal
 * insertion (transmission).
 */
#define	cfg_up		_BIS (P4OUT, 0x01)
#define	cfg_down	_BIC (P4OUT, 0x01)

#define	ser_up		_BIS (P5OUT, 0x01)
#define	ser_down	_BIC (P5OUT, 0x01)
#define	ser_out		_BIS (P5DIR, 0x01)
#define	ser_in		_BIC (P5DIR, 0x01)
#define	ser_data	(P5IN & 0x1)

#define	ser_clk_up	_BIS (P5OUT, 0x02)
#define	ser_clk_down	_BIC (P5OUT, 0x02)

#define	rssi_on		_BIS (P2OUT, 0x01)
#define	rssi_off	_BIC (P2OUT, 0x01)

#define	rcv_sig_high	(P2IN & 0x04)

#define	rcv_interrupt	(P2IFG & 0x40)
#define	rcv_edgelh	_BIC (P2IES, 0x40)
#define	rcv_edgehl	_BIS (P2IES, 0x40)
#define	rcv_clrint	_BIC (P2IFG, 0x40)
#define	rcv_enable	_BIS (P2IE, 0x40)
#define	rcv_disable	_BIC (P2IE, 0x40)
#define	rcv_setedge	do { } while (0)
