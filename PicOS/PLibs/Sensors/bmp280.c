//
// Copyright (C) 2017, Olsonet Communications, All rights reserved
//

//
// BMP280 pressure/temperature combo
//

#include "sysio.h"
#include "bmp280.h"

#ifndef	BMP280_ADDR
#define	BMP280_ADDR	0x76
#endif

#ifndef	bmp280_bring_up
#define	bmp280_bring_up		CNOP
#endif

#ifndef	bmp280_bring_down
#define	bmp280_bring_down	CNOP
#endif

#if I2C_INTERFACE
// Use I2C interface commands; for now, this only works for CC1350; MSP430
// implementation (raw pin) will be trivial, if we ever need it

#define	sbus	__i2c_open (bmp280_scl, bmp280_sda, bmp280_rate)

void bmp280_wreg (byte reg, byte val) {
//
// Write byte to reg
//
	byte msg [2];

	msg [0] = reg;
	msg [1] = val;

	sbus;
	while (__i2c_op (BMP280_ADDR, msg, 2, NULL, 0));
}

void bmp280_rregn (byte reg, byte *data, word n) {
//
// Read bytes from the indicated register
//
	sbus;
	while (__i2c_op (BMP280_ADDR, &reg, 1, data, n));
}

#else
#error "S: bmp280 only works with I2C_INTERFACE!!!"
#endif

// ============================================================================
// Calibration parameters
// ============================================================================

// This is a byte block no need to swab (optional)
//	u16 	dig_T1;	/**<calibration T1 data*/
//	s16 	dig_T2;	/**<calibration T2 data*/
//	s16 	dig_T3;	/**<calibration T3 data*/
//	u16 	dig_P1;	/**<calibration P1 data*/
//	s16 	dig_P2;	/**<calibration P2 data*/
//	s16 	dig_P3;	/**<calibration P3 data*/
//	s16 	dig_P4;	/**<calibration P4 data*/
//	s16 	dig_P5;	/**<calibration P5 data*/
//	s16 	dig_P6;	/**<calibration P6 data*/
//	s16 	dig_P7;	/**<calibration P7 data*/
//	s16 	dig_P8;	/**<calibration P8 data*/
//	s16 	dig_P9;	/**<calibration P9 data*/

// This one is separate
static lint 	t_fine;	/**<calibration t_fine data*/

static word	cal_data [BMP280_PRESSURE_TEMPERATURE_CALIB_DATA_LENGTH/2];

// The order is little-edian, so we have to swab on a big endian machine
#define	dig_T1	(*(word*)((byte*)cal_data+BMP280_TEMPERATURE_CALIB_DIG_T1_LSB))
#define	dig_T2	(*(wint*)((byte*)cal_data+BMP280_TEMPERATURE_CALIB_DIG_T2_LSB))
#define	dig_T3	(*(wint*)((byte*)cal_data+BMP280_TEMPERATURE_CALIB_DIG_T3_LSB))
#define	dig_P1	(*(word*)((byte*)cal_data+BMP280_PRESSURE_CALIB_DIG_P1_LSB))
#define	dig_P2	(*(wint*)((byte*)cal_data+BMP280_PRESSURE_CALIB_DIG_P2_LSB))
#define	dig_P3	(*(wint*)((byte*)cal_data+BMP280_PRESSURE_CALIB_DIG_P3_LSB))
#define	dig_P4	(*(wint*)((byte*)cal_data+BMP280_PRESSURE_CALIB_DIG_P4_LSB))
#define	dig_P5	(*(wint*)((byte*)cal_data+BMP280_PRESSURE_CALIB_DIG_P5_LSB))
#define	dig_P6	(*(wint*)((byte*)cal_data+BMP280_PRESSURE_CALIB_DIG_P6_LSB))
#define	dig_P7	(*(wint*)((byte*)cal_data+BMP280_PRESSURE_CALIB_DIG_P7_LSB))
#define	dig_P8	(*(wint*)((byte*)cal_data+BMP280_PRESSURE_CALIB_DIG_P8_LSB))
#define	dig_P9	(*(wint*)((byte*)cal_data+BMP280_PRESSURE_CALIB_DIG_P9_LSB))

static byte	bstatus;
static byte	bmode;

// ============================================================================

static void get_calib_data () {

	bmp280_rregn (BMP280_TEMPERATURE_CALIB_DIG_T1_LSB_REG, (byte*)cal_data,
		BMP280_PRESSURE_TEMPERATURE_CALIB_DATA_LENGTH);
#if BIG_ENDIAN
	{
		for (sint i = 0;
		    i < BMP280_PRESSURE_TEMPERATURE_CALIB_DATA_LENGTH/2; i++)
			cal_data [i] = __swab (cal_data [i]);
	}
#endif
}

static lint unpack (byte *d) {

	return (lint) ( (((lword) d [0]) << 12) | (((lword) d [1]) << 4) | 
		(((lword) d [2]) >> 4) );
}

static word measurement_time () {
//
// Calculates the measurement time in milliseconds
//
	sint	tovs, povs;

	tovs = (bmode >> 5) & 7;
	povs = (bmode >> 2) & 7;
	return ((BMP280_MEASUREMENT_TIME_PER_OSRS_MAX * (
		((1 << tovs) >> 1) +
		((1 << povs) >> 1) +
		povs ? BMP280_MEASUREMENT_TIME_SETUP_PRESS_MAX : 0) + 15) >> 4);
}

static lint compensate_press (lint vunc) {
//
// Copied from Sensortec
//
	lint v_x1_u32r = 0;
	lint v_x2_u32r = 0;
	lword v_pressure_u32 = 0;

	/* calculate x1*/
	v_x1_u32r = (((lint)t_fine) >> 1) - (lint)64000;
	/* calculate x2*/
	v_x2_u32r = (((v_x1_u32r >> 2) * (v_x1_u32r >> 2)) >> 11)
			* ((lint)dig_P6);
	v_x2_u32r = v_x2_u32r + ((v_x1_u32r *
			((lint)dig_P5)) << 1);
	v_x2_u32r = (v_x2_u32r >> 2) + (((lint)dig_P4) << 16);
	/* calculate x1*/
	v_x1_u32r = (((dig_P3 * (((v_x1_u32r >> 2) * (v_x1_u32r >> 2)) >> 13))
			>> 3) + ((((lint)dig_P2) * v_x1_u32r) >> 1)) >> 18;
	v_x1_u32r = ((((32768 + v_x1_u32r)) * ((lint)dig_P1)) >> 15);
	/* calculate pressure*/
	v_pressure_u32 = (((lword)(((lint)1048576) - vunc)
			- (v_x2_u32r >> 12))) * 3125;
	/* check overflow*/
	if (v_pressure_u32 < 0x80000000)
		/* Avoid exception caused by division by zero */
		if (v_x1_u32r != 0)
			v_pressure_u32 = (v_pressure_u32 << 1)
					/ ((lword)v_x1_u32r);
		else
			// Invalid
			return 0;
	else
	/* Avoid exception caused by division by zero */
	if (v_x1_u32r != 0)
		v_pressure_u32 = (v_pressure_u32 / (lword)v_x1_u32r) * 2;
	else
		// Invalid
		return 0;
	/* calculate x1*/
	v_x1_u32r = (((lint)dig_P9) * ((lint)( ((v_pressure_u32 >> 3)
			* (v_pressure_u32 >> 3)) >> 13))) >> 12;
	/* calculate x2*/
	v_x2_u32r = (((lint)(v_pressure_u32 >> 2)) * ((lint)dig_P8)) >> 13;
	/* calculate true pressure*/
	v_pressure_u32 = (lword)((lint)v_pressure_u32 + ((v_x1_u32r + v_x2_u32r
			+ dig_P7) >> 4));

	return v_pressure_u32;
}

static lint compensate_temp (lint vunc) {
//
// Copied from Sensortec
//
	lint v_x1_u32r = 0;
	lint v_x2_u32r = 0;
	lint temperature = 0;
	/* calculate true temperature*/
	/*calculate x1*/
	v_x1_u32r = ((((vunc >> 3) - ((lint)dig_T1 << 1)))
			* ((lint)dig_T2)) >> 11;
	/*calculate x2*/
	v_x2_u32r = (((((vunc >> 4) - ((lint)dig_T1))
			* ((vunc >> 4) - ((lint)dig_T1)))
			>> 12) * ((lint)dig_T3)) >> 14;
	/*calculate t_fine*/
	t_fine = v_x1_u32r + v_x2_u32r;
	/*calculate temperature*/
	temperature = (t_fine * 5 + 128) >> 8;

	return temperature;
}

// ============================================================================

void bmp280_on (word wmode) {
//
//	mode bits:		 0 - 1	mode 01 forced, 11 normal [00 sleep]
//				 2 - 4	press ovs
//				 5 - 7	temp ovs
//				-----------------
//				 8 - 9	unused (0)
//				10 -12	filter
//				13 -15	standby
//
	byte d;

	if (bmode & 3)
		bmp280_off ();

	bmp280_bring_up;

	d = 0;
	bmp280_rregn (BMP280_CHIP_ID_REG, &d, 1);
	if (d != BMP280_CHIP_ID1 && d != BMP280_CHIP_ID2 &&
	    d != BMP280_CHIP_ID3)
		syserror (EHARDWARE, "BMP280 BAD");

	// Get the calibration parameters, if not there already
	if ((bstatus & BMP280_STATUS_CDPRESENT) == 0) {
		get_calib_data ();
		bstatus |= BMP280_STATUS_CDPRESENT;
	}

	// Set up the chip in the requested mode
	if (((bmode = (byte) wmode) & 0x3) != BMP280_MODE_NORMAL) {
		// Assume this is forced mode, i.e., make sure the chip is
		// in fact put to sleep
		bmode = (bmode & 0xfc) | BMP280_MODE_FORCED;
		wmode &= ~3;
	}

	bmp280_wreg (BMP280_CONFIG_REG, ((byte)(wmode >> 8)) & 0xfc);
	bmp280_wreg (BMP280_CTRL_MEAS_REG, ((byte)(wmode)));
}

void bmp280_off () {

	if (bmode & 3) {
		bmp280_wreg (BMP280_CTRL_MEAS_REG, bmode = 0);
		bstatus &= ~BMP280_STATUS_PENDING;
		bmp280_bring_down;
	}
}

void bmp280_read (word st, const byte *junk, address val) {

	sint nb;
	byte pp, tp, da [6];
	word mt;
	lint *vp;

	// Determine the number of bytes to return
	nb = 0;

	if ((pp = (bmode & 0x1c)))
		nb += 3;
	if ((tp = (bmode & 0xe0)))
		nb += 3;

	if ((bmode & 3) == 0 || nb == 0) {
		// The sensor is off
		bzero (val, nb);
		return;
	}

	if ((bmode & 0x3) == BMP280_MODE_FORCED) {

		if ((bstatus & BMP280_STATUS_PENDING) == 0) {
			// Have to start it up
			bmp280_wreg (BMP280_CTRL_MEAS_REG, bmode);
			bstatus |= BMP280_STATUS_PENDING;
			mt = measurement_time ();
			if (st != WNONE) {
				delay (mt, st);
				release;
			}
			mdelay (mt);
		}

		bstatus &= ~BMP280_STATUS_PENDING;
	}

	vp = (lint*) val;

	if (pp && tp) {
		// Read them both at once
		bmp280_rregn (BMP280_PRESSURE_MSB_REG, da, 6);
		*vp++ = compensate_press (unpack (da));
		*vp   = compensate_temp (unpack (da + 3));
		return;
	}

	if (pp) {
		bmp280_rregn (BMP280_PRESSURE_MSB_REG, da, 3);
		*vp = compensate_press (unpack (da));
		return;
	}

	bmp280_rregn (BMP280_TEMPERATURE_MSB_REG, da, 3);
	*vp = compensate_temp (unpack (da));
}
