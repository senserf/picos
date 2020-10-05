/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "bmp085.h"

#ifndef	bmp085_delay
#define	bmp085_delay	CNOP
#endif

//
// This is how it works:
//
// If the chip is power-switched on and off, these symbols should be defined:
//
//	bmp085_on, bmp085_off
//
// as the respective macros. They are CNOPs by default.
//
// If BMP085_AUTO_CALIBRATE is defined, then the sensor has a non-void
// initializer, bmp_085_init, which pre-fetches the calibration data into a
// memory structure. Then the result (returned as the sensor value) is:
//
//	typedef struct { lword press; sint temp; };
//
// where press is the pressure in Pa and temp is temperature in 0.1C.
//
// Otherwise, the returned value is simply word (pressure) and word (temp), the
// two being the raw 16-bit readouts of the sensor. In that case, the
// calibration data can be read as a special sensor (via bmp085_read_calib).
//
// In both cases, this function (macro) is available:
//
//	bmp085_oversample (byte b);
//
// where b can be 0, 1, 2, 3 (default 0). Higher values may improve accuracy,
// but that probably requires higher precision readout (not described in the
// available doc).
//

#define	ADDR_WRITE	0xEE
#define	ADDR_READ	0xEF
#define	REG_CAL_F	0xAA
#define	REG_CAL_L	0xBE
#define	REG_OPER	0xF4
#define	REG_RES		0xF6
#define	OPER_TEMP	0x2E
#define	OPER_PRES	0x34
#define	OPER_PROF	0x40

#ifdef	BMP085_AUTO_CALIBRATE

static bmp_085_calibration_data_t cd;	// Calibration parameters
static lint cd_tmp;			// Calibration value derived from temp

#else

static word cd_tmp;

#endif

#ifndef	bmp085_on
#define	bmp085_on	CNOP
#define	bmp085_off	CNOP
#endif

// ============================================================================

byte bmp085_osrs = 0, bmp085_state = 0;

static void sstrt () {

	// Both SCL and SDA are parked high
	bmp085_sda_lo;
	bmp085_delay;
	bmp085_scl_lo;
}

static void sstop () {

	bmp085_sda_lo;
	bmp085_scl_hi;
	bmp085_sda_hi;
	bmp085_delay;
}

// ============================================================================

static void put_byte (byte b) {

	register int i;

	for (i = 0; i < 8; i++) {
		if (b & 0x80)
			bmp085_sda_hi;
		else
			bmp085_sda_lo;

		bmp085_scl_hi;
		b <<= 1;
		bmp085_scl_lo;
	}

	// Absorb the ACK bit
	bmp085_sda_in;
	bmp085_scl_hi;
	bmp085_scl_lo;
	bmp085_sda_ou;
}

static word get_byte (Boolean wide) {

	register int i;
	register byte b;

	bmp085_sda_in;
	for (b = 0, i = 0; i < 8; i++) {
		bmp085_scl_hi;
		b <<= 1;
		if (bmp085_sda_val)
			b |= 1;
		bmp085_scl_lo;
	}
	bmp085_sda_ou;

	if (wide)
		bmp085_sda_lo;		// ACK
	else
		bmp085_sda_hi;		// NACK

	bmp085_scl_hi;
	bmp085_scl_lo;

	return b;
}

// ============================================================================

static void wreg (byte reg, byte data) {

	sstrt ();
	put_byte (ADDR_WRITE);
	put_byte (reg);
	put_byte (data);
	sstop ();
}

static word rreg (byte reg) {
//
// Read a 16-bit value from a register
//
	word res;

	sstrt ();

	put_byte (ADDR_WRITE);
	put_byte (reg);

	// Repeated start: SCL is low after put_byte, SDA is high

	bmp085_sda_hi;
	bmp085_scl_hi;

	sstrt ();

	put_byte (ADDR_READ);

	res  = get_byte (1) << 8;
	res |= get_byte (0)     ;

	sstop ();

	return res;
}

// ============================================================================

void bmp085_init () {
#ifdef	BMP085_AUTO_CALIBRATE

	address val;
	byte reg;

	bmp085_on;
	for (val = (address)(&cd), reg = REG_CAL_F; reg <= REG_CAL_L; reg += 2)
		*val++ = rreg (reg);
	bmp085_off;
#endif
}

#ifndef	BMP085_AUTO_CALIBRATE

void bmp085_read_calib (word st, const byte *junk, address val) {
//
// Read the calibration data (this is a dummy sensor)
//
	byte reg;

	bmp085_on;
	for (reg = REG_CAL_F; reg <= REG_CAL_L; reg += 2)
		*val++ = rreg (reg);
	bmp085_off;
}

#endif

// ============================================================================

void bmp085_read (word st, const byte *junk, address val) {

#ifdef	BMP085_AUTO_CALIBRATE
	lint x1, b3, b4;
	word t;
#endif

#ifdef	bmp085_ready
	// Take advantage of the ready signal
	if ((bmp085_state & 1)) {
		// Odd state means waiting for result
		if (!bmp085_ready) {
Delay:
			delay (2, st);
			release;
		}
		bmp085_state++;
	}
#endif
	if (bmp085_state == 0) {
		// Initialize temp measurement
		bmp085_on;
		wreg (REG_OPER, OPER_TEMP);
#ifdef	bmp085_ready
		bmp085_state = 1;
		goto Delay;
#else
		// No ready signal, just wait until you think the result is
		// ready
		bmp085_state = 2;
Delay:
		delay (40, st);
		release;
#endif
	}

	if (bmp085_state == 2) {
		// Temperature ready
#ifdef 	BMP085_AUTO_CALIBRATE
		t = rreg (REG_RES);
		cd_tmp = (((lint)t - (lint)cd.ac6) * (lint)cd.ac5) / 32768;
		cd_tmp += ((lint)cd.mc * 2048) / (cd_tmp + cd.md);
#else
		cd_tmp = rreg (REG_RES);
#endif
		// Start pressure readout
		if (bmp085_osrs > 3)
			bmp085_osrs = 3;
		wreg (REG_OPER, OPER_PRES | (bmp085_osrs << 6));
		bmp085_state = 3;
		goto Delay;
	}

	// Both results ready
#ifdef	BMP085_AUTO_CALIBRATE
	*((sint*)(val + 2)) = ((cd_tmp + 8) / 16);
	t = rreg (REG_RES);
	bmp085_off;
	cd_tmp -= 4000;
	x1 = (cd_tmp * cd_tmp) >> 12;	 	 
	x1 = (cd.b2 * x1) / 2048;
	x1 += (cd.ac2 * cd_tmp) / 2048;
	b3 = (((((lint)cd.ac1) * 4 + x1)) + 2) / 4;

	x1 = (cd.ac3 * cd_tmp) / 8192;
	x1 += (cd.b1 * ((cd_tmp * cd_tmp) >> 12)) / 65536;
	x1 = (x1 + 2) / 4;
	b4 = (cd.ac4 * (lword) (x1 + 32768)) / 32768;
	cd_tmp = ((lword)(t - b3) * 50000);   
	if ((cd_tmp & 0x80000000) == 0)
		cd_tmp = ((lword)cd_tmp * 2) / b4;
	else
		cd_tmp = ((lword)cd_tmp / b4) * 2;
   
	x1 = (lword)cd_tmp / 256;
	x1 *= x1;
	x1 = (x1 * 3038) / 65536;
	*((lword*)val) = (lword)cd_tmp + (x1 + (((lword)cd_tmp * (-7357)) /
		65536) + 3791) / 16;
#else
	*val = rreg (REG_RES);
	*(val + 1) = cd_tmp;
	bmp085_off;
#endif
	bmp085_state = 0;
}
