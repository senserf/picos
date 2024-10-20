/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// DIO_1	- i reed switch (active high, needs pulldown)
// DIO_2	- i mic input
// DIO_3	- o mic clock
// DIO_4	- i button 0 (active low, needs pullup)
// DIO_5	- b SDA	(both externally pulled up)
// DIO_6	- o SCL	(shared by bmp280, hdc1000ypa, tmp007, opt3001
// DIO_7	- i MPU INT (mpu-9250 gir/acc)
// DIO_8	- b SDA mpu-9250 	(both externally pulled up)
// DIO_9	- o SCL mpu-9250
// DIO_10	- o LED0	(single LED lit when high)
// DIO_11	- i tmp007 RDY	(externally pulled up)
// DIO_12	- b mpu-9250 PWR
// DIO_13	- o mic power (keep low) 
// DIO_14	- o mx25r8035 CS (externally pulled up)
// DIO_15	- o button 1 (active low, needs pullup)
// DIO_16	- ? conn (audio?)
// DIO_17	- o mx25r8035 SCLK
// DIO_18	- i mx25r8035 MISO
// DIO_19	- o mx25r8035 MOSI
// DIO_20	- ? conn (CSN?)
// DIO_21	- o buzzer (keep low)
// DIO_22	- ? conn (AUDIO DO?)
// DIO_23	- ? conn DP2
// DIO_24	- ? conn DP1
// DIO_25	- ? conn DP0
// DIO_26	- i connected to VDD (volt sensor???)
// DIO_27	- ? conn DP3
// DIO_28	- i conn DP4	UART RX
// DIO_29	- o conn DP5	UART TX
// DIO_30	- i connected to VDD via 200k res (external button?)

// ============================================================================

#define	IOCPORTS { \
		iocportconfig (IOID_1, IOC_PORT_GPIO, \
			/* Reed switch (button sen), active high, pulldown */ \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_RISING_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_IOPULL_DOWN		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_ENABLE		| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_2, IOC_PORT_GPIO, \
			/* Mic input, dynamically reconfigured, starts off */ \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_3, IOC_PORT_GPIO, \
			/* Mic clock, dynamically reconfigured */ \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_DISABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_4, IOC_PORT_GPIO, \
			/* Button 0, active low, pullup, wake up from s/d */ \
			IOC_IOMODE_NORMAL 	| \
			IOC_FALLING_EDGE	| \
			IOC_INT_DISABLE		| \
			IOC_IOPULL_UP		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			IOC_WAKE_ON_LOW		| \
			0, 0, 0), \
		iocportconfig (IOID_5, IOC_PORT_GPIO, \
			/* SDA, shared, pulled up externally, init open */ \
			IOC_IOMODE_OPEN_DRAIN_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_6, IOC_PORT_GPIO, \
			/* SCL, shared, pulled up externally, init open */ \
			IOC_IOMODE_OPEN_DRAIN_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_7, IOC_PORT_GPIO, \
			/* mpu9250 int */ \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_RISING_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_8, IOC_PORT_GPIO, \
			/* gir/acc SDA, pulled up externally, init open */ \
			IOC_IOMODE_OPEN_DRAIN_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_9, IOC_PORT_GPIO, \
			/* gir/acc SCL, pulled up externally, init open */ \
			IOC_IOMODE_OPEN_DRAIN_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_10, IOC_PORT_GPIO, \
			/* single LED, lit when high */ \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_DISABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 1, 0), \
		iocportconfig (IOID_11, IOC_PORT_GPIO, \
			/* tmp007 Alert */ \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_FALLING_EDGE	| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_12, IOC_PORT_GPIO, \
			/* mpu9250 PWR */ \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_DISABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_4MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 1, 0), \
		iocportconfig (IOID_13, IOC_PORT_GPIO, \
			/* Mic PWR */ \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_DISABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 1, 0), \
		iocportconfig (IOID_14, IOC_PORT_GPIO, \
			/* mx25r8035 CS */ \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_DISABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_15, IOC_PORT_GPIO, \
			/* Button 1, active low, needs pullup */ \
			IOC_IOMODE_NORMAL 	| \
			IOC_FALLING_EDGE	| \
			IOC_INT_DISABLE		| \
			IOC_IOPULL_UP		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			IOC_WAKE_ON_LOW		| \
			0, 0, 0), \
		iocportconfig (IOID_17, IOC_PORT_GPIO, \
			/* mx25r8035 SCLK */ \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_DISABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 1, 0), \
		iocportconfig (IOID_18, IOC_PORT_GPIO, \
			/* mx25r8035 MISO */ \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_IOPULL_DOWN		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_19, IOC_PORT_GPIO, \
			/* mx25r8035 MOSI */ \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_DISABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 1, 0), \
		iocportconfig (IOID_21, IOC_PORT_GPIO, \
			/* buzzer, not used yet, keep low */ \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_DISABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 1, 0), \
		iocportconfig (IOID_28, IOC_PORT_MCU_UART0_RX, \
			/* UART RX */ \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_IOPULL_UP		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_29, IOC_PORT_MCU_UART0_TX, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_DISABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
	}

// ============================================================================
// PINS AVAILABLE ON THE CONN =================================================
// ============================================================================

#define	PIN_LIST { \
		PIN_DEF (IOID_16),	\
		PIN_DEF (IOID_20),	\
		PIN_DEF (IOID_22),	\
		PIN_DEF (IOID_23),	\
		PIN_DEF (IOID_24),	\
		PIN_DEF (IOID_25),	\
		PIN_DEF (IOID_27),	\
	}

#define	PIN_MAX	7

// ============================================================================
// BUTTONS ====================================================================
// ============================================================================

#define	BUTTON_LIST	{ \
				BUTTON_DEF (IOID_4 , 0), \
				BUTTON_DEF (IOID_15, 0), \
			}

#define	BUTTON_DEBOUNCE_DELAY	4
#define	BUTTON_PRESSED_LOW	1
// This is needed for CC1350, not needed for MSP430; should we get rid of them
// by some kind of better (dynamic) initialization? Code size is (probably) not
// an issue on CC1350.
#define	N_BUTTONS		2
#define	BUTTON_GPIOS		((1 << IOID_4) | (1 << IOID_15))

// ============================================================================
// PIN SENSOR (REED SWITCH) ===================================================
// ============================================================================

// Edge == 0 means pressed high
#define	INPUT_PIN_LIST	{ \
				INPUT_PIN (IOID_1, 0), \
			}

#define	N_PINLIST		1
#define	INPUT_PINLIST_GPIOS	(1 << IOID_1)

// ============================================================================
// ============================================================================

#define	SENSOR_DIGITAL
#define	SENSOR_EVENTS
#define	N_HIDDEN_SENSORS	2
#define	SENSOR_INITIALIZERS

#include "sensors.h"
#include "pin_sensor.h"

// ============================================================================
// TMP007 thermopile sensor ===================================================
// ============================================================================

#define	TMP007_ADDR		0x44

#include "tmp007.h"

#define	tmp007_enable		HWREGBITW (IOC_BASE + (IOID_11 << 2), \
					IOC_IOCFG0_EDGE_IRQ_EN_BITN) = 1

#define	tmp007_disable		HWREGBITW (IOC_BASE + (IOID_11 << 2), \
					IOC_IOCFG0_EDGE_IRQ_EN_BITN) = 0

#define	tmp007_clear		GPIO_clearEventDio (IOID_11)

#define	tmp007_int		(HWREG (GPIO_BASE + GPIO_O_EVFLAGS31_0) & \
					(1 << IOID_11))

// This is set to account for multpile configurations of I2C pins when there
// are multiple busses which then have to be switched as needed
#define	tmp007_scl		IOID_6
#define	tmp007_sda		IOID_5
#define	tmp007_rate		1

// ============================================================================
// MPU9250 accel, gyro, compass combo
// ============================================================================

#define	MPU9250_I2C_ADDRESS	0x68
#define	MPU9250_AKM_ADDRESS	0x0C

#include "mpu9250.h"

#define	mpu9250_bring_up	do { GPIO_setDio (IOID_12); \
					mdelay (40); } while (0)

#define	mpu9250_bring_down	do { GPIO_clearDio (IOID_12); \
					mdelay (2); } while (0)

#define	mpu9250_enable		HWREGBITW (IOC_BASE + (IOID_7 << 2), \
					IOC_IOCFG0_EDGE_IRQ_EN_BITN) = 1

#define	mpu9250_disable		HWREGBITW (IOC_BASE + (IOID_7 << 2), \
					IOC_IOCFG0_EDGE_IRQ_EN_BITN) = 0

#define	mpu9250_clear		GPIO_clearEventDio (IOID_7)

#define	mpu9250_int		(HWREG (GPIO_BASE + GPIO_O_EVFLAGS31_0) & \
					(1 << IOID_7))

#define	mpu9250_pending		GPIO_readDio (IOID_7)

#define	mpu9250_scl		IOID_9
#define	mpu9250_sda		IOID_8
#define	mpu9250_rate		1

// ============================================================================
// SPH0641LM4H one-bit microphone =============================================
// ============================================================================

#include "obmicrophone.h"

// Using the new "standard" SSI interface

#define	obmic_bring_up		GPIO_setDio (IOID_13)
#define	obmic_bring_down	GPIO_clearDio (IOID_13)
#define	obmic_rx		IOID_2
#define	obmic_ck		IOID_3
#define	obmic_if		0
// MOTOROLA MODE 3, polarity 1, phase 1
#define	obmic_mode		3

// ============================================================================
// BMP280 pressure/temperature combo ==========================================
// ============================================================================

#include "bmp280.h"

#define	BMP280_ADDR		0x77

#define	bmp280_scl		IOID_6
#define	bmp280_sda		IOID_5
#define	bmp280_rate		1

// ============================================================================
// HDC1000 humid/temp combo ===================================================
// ============================================================================

#include "hdc1000.h"

#define	HDC1000_ADDR		0x43

#define	hdc1000_scl		IOID_6
#define	hdc1000_sda		IOID_5
#define	hdc1000_rate		1

// ============================================================================
// OPT3001 light ==============================================================
// ============================================================================

#include "opt3001.h"

#define	OPT3001_ADDR		0x45

#define	opt3001_scl		IOID_6
#define	opt3001_sda		IOID_5
#define	opt3001_rate		1

// ============================================================================
// BUZZER =====================================================================
// ============================================================================

// Not sure what to do about the buzzer (aka beeper)
#define	beeper_pin_on		GPIO_setDio (IOID_21)
#define	beeper_pin_off		GPIO_clearDio (IOID_21)

#include "beeper.h"

// ============================================================================

#if 1
#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,			\
		INTERNAL_VOLTAGE_SENSOR,			\
		DIGITAL_SENSOR (0, NULL, pin_sensor_read),	\
		DIGITAL_SENSOR (0, NULL, mpu9250_read),		\
		DIGITAL_SENSOR (0, NULL, obmicrophone_read),	\
		DIGITAL_SENSOR (0, NULL, bmp280_read),		\
		DIGITAL_SENSOR (0, NULL, hdc1000_read),		\
		DIGITAL_SENSOR (0, NULL, opt3001_read),		\
		DIGITAL_SENSOR (0, tmp007_init, tmp007_read),	\
}
#endif

// ============================================================================
// ============================================================================
