/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_bmp280_h
#define	__pg_bmp280_h

// ============================================================================
// Copied from the official Sensortec code (all credits go there)
// ============================================================================

#define BMP280_CHIP_ID1		(0x56)
#define BMP280_CHIP_ID2		(0x57)
#define BMP280_CHIP_ID3		(0x58)

// Registers
#define BMP280_TEMPERATURE_CALIB_DIG_T1_LSB_REG             (0x88)
#define BMP280_TEMPERATURE_CALIB_DIG_T1_MSB_REG             (0x89)
#define BMP280_TEMPERATURE_CALIB_DIG_T2_LSB_REG             (0x8A)
#define BMP280_TEMPERATURE_CALIB_DIG_T2_MSB_REG             (0x8B)
#define BMP280_TEMPERATURE_CALIB_DIG_T3_LSB_REG             (0x8C)
#define BMP280_TEMPERATURE_CALIB_DIG_T3_MSB_REG             (0x8D)
#define BMP280_PRESSURE_CALIB_DIG_P1_LSB_REG                (0x8E)
#define BMP280_PRESSURE_CALIB_DIG_P1_MSB_REG                (0x8F)
#define BMP280_PRESSURE_CALIB_DIG_P2_LSB_REG                (0x90)
#define BMP280_PRESSURE_CALIB_DIG_P2_MSB_REG                (0x91)
#define BMP280_PRESSURE_CALIB_DIG_P3_LSB_REG                (0x92)
#define BMP280_PRESSURE_CALIB_DIG_P3_MSB_REG                (0x93)
#define BMP280_PRESSURE_CALIB_DIG_P4_LSB_REG                (0x94)
#define BMP280_PRESSURE_CALIB_DIG_P4_MSB_REG                (0x95)
#define BMP280_PRESSURE_CALIB_DIG_P5_LSB_REG                (0x96)
#define BMP280_PRESSURE_CALIB_DIG_P5_MSB_REG                (0x97)
#define BMP280_PRESSURE_CALIB_DIG_P6_LSB_REG                (0x98)
#define BMP280_PRESSURE_CALIB_DIG_P6_MSB_REG                (0x99)
#define BMP280_PRESSURE_CALIB_DIG_P7_LSB_REG                (0x9A)
#define BMP280_PRESSURE_CALIB_DIG_P7_MSB_REG                (0x9B)
#define BMP280_PRESSURE_CALIB_DIG_P8_LSB_REG                (0x9C)
#define BMP280_PRESSURE_CALIB_DIG_P8_MSB_REG                (0x9D)
#define BMP280_PRESSURE_CALIB_DIG_P9_LSB_REG                (0x9E)
#define BMP280_PRESSURE_CALIB_DIG_P9_MSB_REG                (0x9F)

#define BMP280_CHIP_ID_REG                   (0xD0)  /*Chip ID Register */
#define BMP280_RST_REG                       (0xE0)  /*Softreset Register */
#define BMP280_STAT_REG                      (0xF3)  /*Status Register */
#define BMP280_CTRL_MEAS_REG                 (0xF4)  /*Ctrl Measure Register */
#define BMP280_CONFIG_REG                    (0xF5)  /*Configuration Register */
#define BMP280_PRESSURE_MSB_REG              (0xF7)  /*Pressure MSB Register */
#define BMP280_PRESSURE_LSB_REG              (0xF8)  /*Pressure LSB Register */
#define BMP280_PRESSURE_XLSB_REG             (0xF9)  /*Pressure XLSB Register */
#define BMP280_TEMPERATURE_MSB_REG           (0xFA)  /*Temperature MSB Reg */
#define BMP280_TEMPERATURE_LSB_REG           (0xFB)  /*Temperature LSB Reg */
#define BMP280_TEMPERATURE_XLSB_REG          (0xFC)  /*Temperature XLSB Reg */

// Location of calibration parameters in the block
#define	BMP280_TEMPERATURE_CALIB_DIG_T1_LSB			(0)
#define	BMP280_TEMPERATURE_CALIB_DIG_T1_MSB			(1)
#define	BMP280_TEMPERATURE_CALIB_DIG_T2_LSB			(2)
#define	BMP280_TEMPERATURE_CALIB_DIG_T2_MSB			(3)
#define	BMP280_TEMPERATURE_CALIB_DIG_T3_LSB			(4)
#define	BMP280_TEMPERATURE_CALIB_DIG_T3_MSB			(5)
#define	BMP280_PRESSURE_CALIB_DIG_P1_LSB			(6)
#define	BMP280_PRESSURE_CALIB_DIG_P1_MSB			(7)
#define	BMP280_PRESSURE_CALIB_DIG_P2_LSB			(8)
#define	BMP280_PRESSURE_CALIB_DIG_P2_MSB			(9)
#define	BMP280_PRESSURE_CALIB_DIG_P3_LSB			(10)
#define	BMP280_PRESSURE_CALIB_DIG_P3_MSB			(11)
#define	BMP280_PRESSURE_CALIB_DIG_P4_LSB			(12)
#define	BMP280_PRESSURE_CALIB_DIG_P4_MSB			(13)
#define	BMP280_PRESSURE_CALIB_DIG_P5_LSB			(14)
#define	BMP280_PRESSURE_CALIB_DIG_P5_MSB			(15)
#define	BMP280_PRESSURE_CALIB_DIG_P6_LSB			(16)
#define	BMP280_PRESSURE_CALIB_DIG_P6_MSB			(17)
#define	BMP280_PRESSURE_CALIB_DIG_P7_LSB			(18)
#define	BMP280_PRESSURE_CALIB_DIG_P7_MSB			(19)
#define	BMP280_PRESSURE_CALIB_DIG_P8_LSB			(20)
#define	BMP280_PRESSURE_CALIB_DIG_P8_MSB			(21)
#define	BMP280_PRESSURE_CALIB_DIG_P9_LSB			(22)
#define	BMP280_PRESSURE_CALIB_DIG_P9_MSB			(23)

#define	BMP280_PRESSURE_TEMPERATURE_CALIB_DATA_LENGTH		24

// Delays for forced measurement
#define	BMP280_MEASUREMENT_TIME_INIT				20
#define	BMP280_MEASUREMENT_TIME_PER_OSRS_MAX			37
#define	BMP280_MEASUREMENT_TIME_SETUP_PRESS_MAX			10

// Calibration data read in
#define	BMP280_STATUS_CDPRESENT					0x80
// Waiting for forced data
#define	BMP280_STATUS_PENDING					0x01

// Mode components
#define	BMP280_MODE_NORMAL					0x03
#define	BMP280_MODE_FORCED					0x01

#define	BMP280_MODE_PRESS_OVS_OFF				(0 << 2)
#define	BMP280_MODE_PRESS_OVS_1X				(1 << 2)
#define	BMP280_MODE_PRESS_OVS_2X				(2 << 2)
#define	BMP280_MODE_PRESS_OVS_4X				(3 << 2)
#define	BMP280_MODE_PRESS_OVS_8X				(4 << 2)
#define	BMP280_MODE_PRESS_OVS_16X				(5 << 2)
#define	BMP280_MODE_PRESS_OVS_MASK				(7 << 2)

#define	BMP280_MODE_TEMP_OVS_OFF				(0 << 5)
#define	BMP280_MODE_TEMP_OVS_1X					(1 << 5)
#define	BMP280_MODE_TEMP_OVS_2X					(2 << 5)
#define	BMP280_MODE_TEMP_OVS_4X					(3 << 5)
#define	BMP280_MODE_TEMP_OVS_8X					(4 << 5)
#define	BMP280_MODE_TEMP_OVS_16X				(5 << 5)
#define	BMP280_MODE_TEMP_OVS_MASK				(7 << 5)

#define	BMP280_MODE_FILTER_OFF					(0 << 10)
#define	BMP280_MODE_FILTER_2					(1 << 10)
#define	BMP280_MODE_FILTER_4					(2 << 10)
#define	BMP280_MODE_FILTER_8					(3 << 10)
#define	BMP280_MODE_FILTER_16					(4 << 10)

#define	BMP280_MODE_STANDBY_05_MS				(0 << 13)
#define	BMP280_MODE_STANDBY_63_MS				(1 << 13)
#define	BMP280_MODE_STANDBY_125_MS				(2 << 13)
#define	BMP280_MODE_STANDBY_250_MS				(3 << 13)
#define	BMP280_MODE_STANDBY_500_MS				(4 << 13)
#define	BMP280_MODE_STANDBY_1000_MS				(5 << 13)
#define	BMP280_MODE_STANDBY_2000_MS				(6 << 13)
#define	BMP280_MODE_STANDBY_4000_MS				(7 << 13)

// ============================================================================

typedef struct {
//
// Sensor value:
//
	lint	press, temp;
} bmp280_data_t;

#ifndef __SMURPH__

#include "pins.h"
//+++ "bmp280.c"

void bmp280_read (word, const byte*, address);

void bmp280_on (word);		// Mode
void bmp280_off ();
void bmp280_wreg (byte, byte);
void bmp280_rregn (byte, byte*, word);

#else	/* __SMURPH__ */

#define	bmp280_on(a)		emul (9, "BMP280_ON: %04x", a)
#define	bmp280_off()		emul (9, "BMP280_OFF: <>")
#define	bmp280_rregn(a,b,c)	bzero (b, c)
#define	bmp280_wreg(a,b)	emul (9, "BMP280_WREG: %02x %04x", a, b)

#endif	/* __SMURPH__ */


#endif
