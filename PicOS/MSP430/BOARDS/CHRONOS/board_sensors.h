// ============================================================================
// CMA3000 - acceleration sensor ==============================================
// ============================================================================

#include "cma3000.h"

// Connection:
//
// PJ.0 -> power switch
// PJ.1 -> CS
// P2.5 -> interrupt
// P1.5 -> SDI (sensor to uC)
// P1.6 -> SDO (uC to sensor)
// P1.7 -> Clock

#define	cma3000_bring_down		do { \
						_BIC (PJOUT, 0x03); \
						_BIC (P1OUT, 0xE0); \
						_BIS (P1DIR, 0xE0); \
						_BIC (P1SEL, 0xE0); \
						_BIC (P2IE, 0x20); \
					} while (0)
//+++ "p2irq.c"

// SPI master, 8 bits, MSB first, clock idle low, data output on first edge
// Rate = 12MHz/30 = 400kHz
#define	cma3000_bring_up	do { \
					UCA0CTL1 = UCSWRST; \
					UCA0CTL0 |= UCSYNC | UCMST | UCMSB | \
						UCCKPH; \
					UCA0CTL1 |= UCSSEL1; \
					UCA0BR0 = 30; \
					UCA0BR1 = 0; \
					UCA0CTL1 &= ~UCSWRST; \
					_BIC (P1DIR, 0x20); \
					_BIS (P1SEL, 0xE0); \
					_BIS (PJOUT, 0x01); \
				} while (0)

#define	cma3000_csel		do { \
					_BIC (P1REN, 0x20); \
					_BIC (PJOUT, 0x02); \
				} while (0)

#define	cma3000_cunsel		do { \
					_BIS (PJOUT, 0x02); \
					_BIS (P1REN, 0x20); \
				} while (0)

// Rising edge, no need to change IES
#define	cma3000_enable		_BIS (P2IE, 0x20)
#define	cma3000_disable		_BIC (P2IE, 0x20)
#define	cma3000_clear		_BIC (P2IFG, 0x20)
#define	cma3000_int		(P2IFG & 0x20)

#define	cma3000_spi_read	UCA0RXBUF
#define	cma3000_spi_write(b)	UCA0TXBUF = (b)

#define	cma3000_busy		((UCA0IFG & UCRXIFG) == 0)
#define	cma3000_delay		udelay (2)

// ============================================================================
// SCP1000-D01 pressure sensor ================================================
// ============================================================================

#include "scp1000.h"

#define	SCP1000_DELAY		__asm__ __volatile__("nop")

#define	scp1000_sda_val		(PJIN & 0x04)

// Note: the pullup resistor used for SDA in ez430-chronos (47K) is too large
// to use the pin in open drain mode; this is why I am driving it in low
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
#define	SENSOR_ANALOG
#define	SENSOR_EVENTS
#define	SENSOR_INITIALIZERS

#include "analog_sensor.h"
#include "sensors.h"

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,	\
		INTERNAL_VOLTAGE_SENSOR,	\
		DIGITAL_SENSOR (0, NULL, cma3000_read), \
		DIGITAL_SENSOR (1, scp1000_init, scp1000_read) \
}

#define	N_HIDDEN_SENSORS	2

#define	SENSOR_TEMP		(-2)
#define	SENSOR_BATTERY		(-1)
#define	SENSOR_MOTION		0
#define	SENSOR_PRESSTEMP	1

//+++ "sensors.c"
