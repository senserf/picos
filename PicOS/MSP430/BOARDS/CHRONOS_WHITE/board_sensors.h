// ============================================================================
// BMA250 - acceleration sensor ===============================================
// ============================================================================

#include "bma250.h"

// Connection:
//
// PJ.0 -> power switch
// PJ.1 -> CSB
// P2.5 -> interrupt (INT1)
// P1.5 -> SDI (sensor to uC)
// P1.6 -> SDO (uC to sensor)
// P1.7 -> Clock

#define	bma250_bring_down		do { \
						_BIC (PJOUT, 0x03); \
						_BIC (P1OUT, 0xE0); \
						_BIS (P1DIR, 0xE0); \
						_BIC (P1SEL, 0xE0); \
						_BIC (P2IE, 0x20); \
					} while (0)
//+++ "p2irq.c"
REQUEST_EXTERNAL (p2irq);

// SPI master, 8 bits, MSB first, clock idle low, data output on first edge
// Rate = 12MHz/30 = 400kHz (### can [should] the rate be increased?)
#define	bma250_bring_up		do { \
					UCA0CTL1 = UCSWRST; \
					UCA0CTL0 |= UCSYNC | UCMST | UCMSB | \
						UCCKPH; \
					UCA0CTL1 |= UCSSEL1; \
					UCA0BR0 = 30; \
					UCA0BR1 = 0; \
					UCA0CTL1 &= ~UCSWRST; \
					_BIC (P1DIR, 0x20); \
					_BIS (P1SEL, 0xE0); \
					_BIS (PJOUT, 0x03); \
				} while (0)

#define	bma250_csel		do { \
					_BIC (P1REN, 0x20); \
					_BIC (PJOUT, 0x02); \
				} while (0)

#define	bma250_cunsel		do { \
					_BIS (PJOUT, 0x02); \
					_BIS (P1REN, 0x20); \
				} while (0)

// Rising edge + already on
#define	bma250_enable		do { if (P2IN & 0x20) _BIS (P2IFG, 0x20); \
				     _BIS (P2IE, 0x20); } while (0)
#define	bma250_disable		_BIC (P2IE, 0x20)
#define	bma250_clear		_BIC (P2IFG, 0x20)
#define	bma250_int		(P2IFG & 0x20)

#define	bma250_spi_read		UCA0RXBUF
#define	bma250_spi_write(b)	UCA0TXBUF = (b)

#define	bma250_busy		((UCA0IFG & UCRXIFG) == 0)
#define	bma250_delay		udelay (150)

// ============================================================================
// BMP085 pressure sensor =====================================================
// ============================================================================

// Connection:
//
// PJ.2 -> SDA (pulled up)
// PJ.3 -> SCL
// P2.6 -> EOC (high -> ready)

#include "bmp085.h"

// #define	bmp085_delay		__asm__ __volatile__("nop")

#define	bmp085_sda_val		(PJIN & 0x04)

// Note: the pullup resistor used for SDA in ez430-chronos (47K) is too large
// to use the pin in open drain mode; this is why I am driving it in low
// impedance mode dropping it explicitly for data reception
#define	bmp085_sda_lo		_BIC (PJOUT, 0x04)
#define	bmp085_sda_hi		_BIS (PJOUT, 0x04)
#define	bmp085_sda_in		_BIC (PJDIR, 0x04)
#define	bmp085_sda_ou		_BIS (PJDIR, 0x04)

// ============================================================================

#define	bmp085_scl_hi		do { _BIS (PJOUT, 0x08); bmp085_delay; } \
					while (0)

#define	bmp085_scl_lo		do { _BIC (PJOUT, 0x08); bmp085_delay; } \
					while (0)

// #define	bmp085_ready		(P2IN & 0x40)

// ============================================================================
// ============================================================================

#define	SENSOR_DIGITAL
#define	SENSOR_ANALOG
#define	SENSOR_EVENTS

#include "analog_sensor.h"
#include "sensors.h"

#ifdef	BMP085_AUTO_CALIBRATE
#define	__bmp085_cal_sensor
#define	N_HIDDEN_SENSORS	2
#else
#define	__bmp085_cal_sensor	DIGITAL_SENSOR (0, NULL, bmp085_read_calib),
#define	N_HIDDEN_SENSORS	3
#define	SENSOR_PRESSTEMP_CALIB	(-3)
#endif

#ifdef	BUTTON_LIST

// Buttons implemented via a driver

#define	SENSOR_LIST { \
		__bmp085_cal_sensor				\
		INTERNAL_TEMPERATURE_SENSOR,			\
		INTERNAL_VOLTAGE_SENSOR,			\
		DIGITAL_SENSOR (0, NULL, bma250_read), 		\
		DIGITAL_SENSOR (0, bmp085_init, bmp085_read)	\
}

#else

// Buttons implemented as a sensor
#include "pin_sensor.h"

#define	SENSOR_LIST { \
		__bmp085_cal_sensor					\
		INTERNAL_TEMPERATURE_SENSOR,				\
		INTERNAL_VOLTAGE_SENSOR,				\
		DIGITAL_SENSOR (0, NULL, bma250_read),			\
		DIGITAL_SENSOR (0, bmp085_init, bmp085_read),		\
		DIGITAL_SENSOR (0, pin_sensor_init, pin_sensor_read) 	\
}

#define	SENSOR_BUTTONS		2

#endif	/* BUTTON_LIST */


#define	SENSOR_TEMP		(-2)
#define	SENSOR_BATTERY		(-1)
#define	SENSOR_MOTION		0
#define	SENSOR_PRESSTEMP	1
#define	SENSOR_INITIALIZERS

//+++ "sensors.c"
