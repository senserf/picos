// ============================================================================
// CMA3000 - acceleration sensor ==============================================
// ============================================================================

#include "cma_3000.h"

//
// Select the mode for the CMA3000 sensor; we are using it for motion detection
//

#define	CMA3000_MODE_MOTION

// Connection:
//
// PJ.0 -> power switch
// PJ.1 -> CS
// P2.5 -> interrupt
// P1.5 -> SDI (sensor to uC)
// P1.6 -> SDO (uC to sensor)
// P1.7 -> Clock

#define	zz_cma_3000_bring_down	do { \
					_BIC (PJOUT, 0x03); \
					_BIC (P1OUT, 0xE0); \
					_BIS (P1DIR, 0xE0); \
					_BIC (P1SEL, 0xE0); \
					_BIC (P2IE, 0x20); \
				} while (0)
//+++ "p2irq.c"

// SPI master, 8 bits, MSB first, clock idle low, data output on
// SMCLK
// Rate = 12MHz/30 = 400kHz
// Interrupt direction
// SPI
// Power up
// Delay for > 5ms
#define	zz_cma_3000_bring_up	do { \
					UCA0CTL0 |= UCSYNC | UCMST | UCMSB | \
						UCCKPH; \
					UCA0CTL1 |= UCSSEL1; \
					UCA0BR0 = 30; \
					UCA0BR1 = 0; \
					UCA0CTL1 &= ~UCSWRST; \
					_BIC (P1DIR, 0x20); \
					_BIS (P1SEL, 0xE0); \
					_BIS (PJOUT, 0x01); \
					mdelay (50); \
				} while (0)

#define	zz_cma_3000_csel	do { \
					_BIC (P1REN, 0x20); \
					_BIC (PJOUT, 0x02); \
					mdelay (50); \
				} while (0)

#define	zz_cma_3000_cunsel	do { \
					_BIS (PJOUT, 0x02); \
					_BIS (P1REN, 0x20); \
				} while (0)

// Rising edge, no need to change IES
#define	zz_cma_3000_enable	_BIS (P2IE, 0x20)
#define	zz_cma_3000_disable	_BIC (P2IE, 0x20)
#define	zz_cma_3000_clear	_BIC (P2IFG, 0x20)
#define	zz_cma_3000_int		(P2IFG & 0x20)

#define	zz_cma_3000_read	UCA0RXBUF
#define	zz_cma_3000_write(b)	UCA0TXBUF = (b)

#define	zz_cma_3000_busy	((UCA0IFG & UCRXIFG) == 0)

#ifdef	CMA3000_MODE_MOTION
// 0x20 = permanently stay in motion detection mode
// 0x08 = motion detection mode (interrupts enabled)
// 0x10 = I2C disabled
#define	CMA3000_CONFIG		(0x20 + 0x00 + 0x08)
//#define	CMA3000_CONFIG		0x84
#define	CMA3000_THRESHOLD	1

// This is the only mode for now
#endif


// ============================================================================
// SCP1000-D01 pressure sensor ================================================
// ============================================================================

#include "scp_1000.h"

#define	SCP1000_DELAY		__asm__ __volatile__("nop")

#define	scp1000_sda_val		(PJIN & 0x04)

// Note: the pullup resistor used for SDA in ez430-chronos (47K) is too large
// properly use the pin in open drain mode; this is why I am driving it in low
// impedance mode dropping it explicitly for data reception
#define	scp1000_sda_lo		_BIC (PJOUT, 0x04)
#define	scp1000_sda_hi		_BIS (PJOUT, 0x04)
#define	scp1000_sda_in		_BIC (PJDIR, 0x04)
#define	scp1000_sda_ou		_BIS (PJDIR, 0x04)

// ============================================================================

#define	scp1000_scl_hi		do { _BIS (PJOUT, 0x08); SCP1000_DELAY; } \
					while (0)

#define	scp1000_scl_lo		do { _BIC (PJOUT, 0x08); SCP1000_DELAY; } \
					while (0)

#define	scp1000_ready		(P2IN & 0x40)

// ============================================================================
// ============================================================================

#define	SENSOR_DIGITAL
#define	SENSOR_INITIALIZERS

#define	SENSOR_LIST { \
		DIGITAL_SENSOR (0, NULL, cma_3000_read), \
		DIGITAL_SENSOR (1, scp_1000_init, scp_1000_read) \
}

#define	MOTION_SENSOR	0
#define	PRESSURE_SENSOR	1
