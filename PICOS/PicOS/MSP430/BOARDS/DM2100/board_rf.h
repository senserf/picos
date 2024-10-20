/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

/*
 * Pin assignment (see IM2100):
 *
 *  TR1000                      MSP430Fxxx
 * ===========================================
 *  RXDATA			CCI0B (P2.2)		in
 *  TXMOD			OUT0 (P2.7)		out
 *  CNTRL0			P5.1			out
 *  CNTRL1			P5.0			out
 *  RSSI			A0 (P6.0)		analog in
 *  RSSI POWER-UP		P2.0			up on high
 */


/*
 * Transmission rate: this is determined as the number of SLCK ticks per
 * physical bit time. Remember that SLCK runs at 4.5MHz. The math is simple:
 * one physical bit = 2/3 of real bit (excluding the preamble and checksum).
 */
#define	BIT_RATE	6429

#define	ini_regs	do { \
				_BIC (P2OUT, 0x85); \
				_BIS (P2DIR, 0x81); \
				_BIC (P2DIR, 0x1C); \
				_BIS (P2SEL, 0x84); \
				_BIC (P5OUT, 0x03); \
				_BIS (P5DIR, 0x03); \
				_BIC (P6DIR, 0xfe); \
				_BIC (P1DIR, 0x0f); \
			} while (0)
/*
 * DM2100 signal operations. Timer's A Capture/Compare Block is used for signal
 * insertion/extraction.
 */
#define	c0up		_BIS (P5OUT, 0x02)
#define	c0down		_BIC (P5OUT, 0x02)
#define	c1up		_BIS (P5OUT, 0x01)
#define	c1down		_BIC (P5OUT, 0x01)
#define	rssi_on		_BIS (P2OUT, 0x01)
#define	rssi_off	_BIC (P2OUT, 0x01)

