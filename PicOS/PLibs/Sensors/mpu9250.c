/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "mpu9250.h"

#ifndef	mpu9250_bring_up
#define	mpu9250_bring_up	CNOP
#ifndef	MPU9250_DELAY_RESET
#define	MPU9250_DELAY_RESET	5
#endif
#ifndef	MPU9250_DELAY_WAKEUP
#define	MPU9250_DELAY_WAKEUP	24
#endif
#endif

#ifndef	mpu9250_bring_down
#define	mpu9250_bring_down	CNOP
#endif

#ifndef	MPU9250_DELAY_RESET
#define	MPU9250_DELAY_RESET	0
#endif

#ifndef	MPU9250_DELAY_WAKEUP
#define	MPU9250_DELAY_WAKEUP	0
#endif

#ifndef	MPU9250_DELAY_AKMADJ
#define	MPU9250_DELAY_AKMADJ	1
#endif

#define	__mdelay(d)		do { if (d) mdelay (d); } while (0)

// ============================================================================

#if I2C_INTERFACE

#define	sbus	__i2c_open (mpu9250_scl, mpu9250_sda, mpu9250_rate)

// For now, this only works for CC1350

static inline void __wreg (byte addr, byte reg, byte val) {
//
// Write a register; all we ever need is one byte at a time
//
	byte msg [2];

	msg [0] = reg;
	msg [1] = val;

	sbus;
	while (__i2c_op (addr, msg, 2, NULL, 0));
}

static inline void __rregn (byte addr, byte reg, byte *data, word n) {
//
// Read n bytes from a register
//
	sbus;
	while (__i2c_op (addr, &reg, 1, data, n));
}
	
#else
#error "S: mpu9250 only works with I2C_INTERFACE!!!"
#endif

// ============================================================================

void mpu9250_wrega (byte reg, byte val) {
//
// Single register write to the main device
//
	__wreg (MPU9250_I2C_ADDRESS, reg, val);
}

void mpu9250_wregc (byte reg, byte val) {
//
// Single register write to the compass device
//
	__wreg (MPU9250_AKM_ADDRESS, reg, val);
}

void mpu9250_rregan (byte reg, byte *data, word n) {
	__rregn (MPU9250_I2C_ADDRESS, reg, data, n);
}

void mpu9250_rregcn (byte reg, byte *data, word n) {
	__rregn (MPU9250_AKM_ADDRESS, reg, data, n);
}

// ============================================================================

byte	mpu9250_status = 0,
	// Compass adjustments; could be static, but this way we avoid
	// fragmentation on misalignment
	mpu9250_cmp_sad [3];

// ============================================================================

void mpu9250_on (lword options, byte threshold) {
//
//	options:	 0 -  2		lpf
//			 3 -  6		odr
//			 7 - 10		which: A G C T
//			11 - 12		arange
//			13 - 14		grange
//			15 - 16		1 - motion detect, 2 - sync read
//			17		LP mode for normal operation
//			18		FIFO enable
//			19 - 23		reserved
//			24 - 31		sampling rate
//
//      x x x x  x x x x  x x x x  x x x x  x x x x  x x x x  x x x x  x x x x
//      ----------------  ---------- - - ---- --- ---- -------- -------- -----
//	 sampling rate      unused   f l etp  rg   ra    comp      odr    lpf
//
//	For motion detect: 1000 0000 1010 1001 = 80A9, th = 32
//
	byte sensors, etype, b, lpf;

	if (mpu9250_status & 0x0f)
		// The device is on, turn off first
		mpu9250_off ();

	mpu9250_bring_up;
#if MPU9250_DELAY_RESET
	// Need reset after power up
	mpu9250_wrega (MPU9250_REG_PWR_MGMT_1, 0x80);
	// Will add a state arg, if these delays are serious
	__mdelay (MPU9250_DELAY_RESET);
#endif
#if MPU9250_DELAY_WAKEUP
	// Need wakeup to start, as in clearing the SLEEP bit
	mpu9250_wrega (MPU9250_REG_PWR_MGMT_1, 0x00);
	__mdelay (MPU9250_DELAY_WAKEUP);
#endif
	// Check if we are up and running
	mpu9250_rregan (MPU9250_REG_WHO_AM_I, &b, 1);
	if (b != 0x71)
		syserror (EHARDWARE, "MPU9250 BAD");

	// 1 = A, 2 = G, 4 = C, 8 = T
	sensors = (options >> MPU9250_OPT_SH_SENSORS) & 0x0f;
	etype = (options >> MPU9250_OPT_SH_EVENT_TYPE) & 0x3;
	if (sensors == 0 || etype != 0) {
		// For event processing, select accel by default
		sensors = 0x01;
	}
	// Disable what's not needed
	b = (sensors & 1) ? 0x00 : 0x38;
	if ((sensors & 2) == 0)
		b |= 0x07;
	mpu9250_wrega (MPU9250_REG_PWR_MGMT_2, b);

	// LPF (same for accel and gyro)
	lpf = (options >> MPU9250_OPT_SH_LPF) & 0x07;
	mpu9250_wrega (MPU9250_REG_CONFIG, lpf);

	// Set ranges; fchoice stays at 00
	mpu9250_wrega (MPU9250_REG_GYRO_CONFIG,
		((options >> MPU9250_OPT_SH_GYRO_RANGE) << 3) & 0x18);

	mpu9250_wrega (MPU9250_REG_ACCEL_CONFIG,
		((options >> MPU9250_OPT_SH_ACCEL_RANGE) << 3) & 0x18);

	// Interrupt + bypass configuration; motion detection ->
	// latch + clear on read + bypass
	b = etype ? 0x30 : 0x00;

	if (sensors & 4)
		// Compass needed, bypass enable
		b |= 0x02;
	mpu9250_wrega (MPU9250_REG_INT_PIN_CFG, b);

	if (sensors & 4) {
		// Set up the compass
		mpu9250_rregcn (MPU9250_REG_AKM_WHOAMI, &b, 1);
		if (b != 0x48)
			syserror (EHARDWARE, "MPU9250 AKM");
		if ((mpu9250_status & MPU9250_STATUS_COMPASS_ADJREAD) == 0) {
			// Need to read adjustments
			mpu9250_wregc (MPU9250_REG_AKM_CNTL, 0x1f);
			__mdelay (MPU9250_DELAY_AKMADJ);
			mpu9250_rregcn (MPU9250_REG_AKM_ASAX,
				mpu9250_cmp_sad, 3);
			mpu9250_status |= MPU9250_STATUS_COMPASS_ADJREAD;
		}
		// 16-bit data, power down, single measurement
		mpu9250_wregc (MPU9250_REG_AKM_CNTL, 0x11);
	}

	// Set the low pass filter; ACCEL_FCHOICE = 1 [~0]
	// Fixed this (210630); ACCEL_FCHOICE was set to zero [~1 = 0x80] which
	// disabled LPF setting for ACCEL
	mpu9250_wrega (MPU9250_REG_ACCEL_CONFIG2, 0x00 | lpf);

	// Set the sampling rate divider
	mpu9250_wrega (MPU9250_REG_SMPLRT_DIV,
		(byte)(options >> MPU9250_OPT_SH_RATE_DIVIDER));

	// The cycle bit; this actually enables LPA (the low-power data rates),
	// without this bit, the LPA settings are ignored
	b =  (etype == 1 || (options & MPU9250_LP_MODE)) ? 0x20 : 0x00;
	if (etype == 1) {
		// Motion detection, accel hardware intelligence
		mpu9250_wrega (MPU9250_REG_MOT_DETECT_CTRL, 0xc0);
		// Motion threshold
		mpu9250_wrega (MPU9250_REG_WOM_THR, threshold);
	}

	if (etype)
		// Enable interrupts for motion detection or data ready, if
		// we don't want FIFO. FIFO doesn't work in LP mode !!!
		mpu9250_wrega (MPU9250_REG_INT_ENABLE, (etype == 1) ?
			0x40 : 0x01);

	// Frequency of wakeups
	mpu9250_wrega (MPU9250_REG_LP_ACCEL_ODR,
		(options >> MPU9250_OPT_SH_ODR) & 0x0f);

	if (sensors & 2)
		// Clock from PLL, if gyro used
		b |= 1;

	mpu9250_wrega (MPU9250_REG_PWR_MGMT_1, b);

	mpu9250_status |= sensors;
}

void mpu9250_off () {

	if ((mpu9250_status & 0x0f) == 0)
		// Already off
		return;

	mpu9250_fifo_stop ();

#if MPU9250_DELAY_RESET
	// Need reset after power up; this basically means that power down
	// won't do the trick; set the SLEEP bit
	mpu9250_wrega (MPU9250_REG_PWR_MGMT_1, 0x40);
#endif
	mpu9250_bring_down;
	mpu9250_status &= ~0x0f;
}

static void swab (address p, word n) {
//
// Swap bytes
//
	while (n--) {
		*p = __swabw (*p);
		p++;
	}
}

word mpu9250_fifo_get (address buf, word n) {
//
// Read from FIFO
//
	word cnt;

	if ((mpu9250_status & MPU9250_STATUS_FIFO) == 0)
		// FIFO is not on
		return 0;

	// Check for overflow
	mpu9250_rregan (MPU9250_REG_INT_STATUS, (byte*)&cnt, 1);
	if ((*((byte*)&cnt) & 0x10)) {
		// Overflow, reset
		mpu9250_wrega (MPU9250_REG_USER_CTRL, 0x44);
		return MPU9250_FIFO_OVERFLOW;
	}

	mpu9250_rregan (MPU9250_REG_FIFO_COUNT_H, (byte*)&cnt, 2);
	cnt = htons (cnt);
	if (cnt < 2)
		return 0;
	// Turn it into words
	cnt >>= 1;

	if (cnt > n)
		cnt = n;

	n = cnt;

	while (cnt--) {
#if LITTLE_ENDIAN
		mpu9250_rregan (MPU9250_REG_FIFO_R_W, ((byte*)buf) + 1, 1);
		mpu9250_rregan (MPU9250_REG_FIFO_R_W, ((byte*)buf)    , 1);
#else
		mpu9250_rregan (MPU9250_REG_FIFO_R_W, ((byte*)buf)    , 1);
		mpu9250_rregan (MPU9250_REG_FIFO_R_W, ((byte*)buf) + 1, 1);
#endif
		buf++;
	}

	return n;
}

void mpu9250_fifo_start () {
//
// Starts FIFO operation
//
	byte b;

	b = (mpu9250_status & MPU9250_STATUS_ACCEL_ON) ? 0x08 : 0;
	if ((mpu9250_status & MPU9250_STATUS_GYRO_ON))
		// Add all coordinates of the gyro
		b |= 0x70;
	if ((mpu9250_status & MPU9250_STATUS_TEMP_ON))
		// Throw in temp
		b |= 0x80;
	if (b) {
		// Enable FIFO + reset
		mpu9250_wrega (MPU9250_REG_USER_CTRL, 0x44);
		mpu9250_wrega (MPU9250_REG_FIFO_EN, b);
	}

	_BIS (mpu9250_status, MPU9250_STATUS_FIFO);
}

void mpu9250_fifo_stop () {
//
// Stop FIFO operation
//
	mpu9250_wrega (MPU9250_REG_FIFO_EN, 0);
	mpu9250_wrega (MPU9250_REG_USER_CTRL, 0x04);
	_BIC (mpu9250_status, MPU9250_STATUS_FIFO);
}

void mpu9250_read (word st, const byte *sen, address val) {

	address vap;
	sint i;

	if (val == NULL) {
		// Issued to wait for an event
		if (mpu9250_status & MPU9250_STATUS_EVENT) {
			// Event already pending
Race:
			_BIC (mpu9250_status, MPU9250_STATUS_EVENT);
			proceed (st);
		}
		// Reset the interrupt condition
		mpu9250_clear;
		// Declare the FSM as waiting
		when (&mpu9250_status, st);
		_BIS (mpu9250_status, MPU9250_STATUS_WAIT);
		// Enable interrupts
		mpu9250_enable;
		// Account for a race; this is impossible if enabling triggers
		// an already pending interrupt
		if (mpu9250_pending) {
			mpu9250_disable;
			goto Race;
		}
		release;
	}

	// Read the values according to which sensors are active, in this
	// order: A G C T

	vap = val;

	if (mpu9250_status & MPU9250_STATUS_ACCEL_ON) {
		// Accel present
		mpu9250_rregan (MPU9250_REG_ACCEL_XOUT_H, (byte*)vap, 6);
#if LITTLE_ENDIAN
		swab (vap, 3);
#endif
		vap += 3;
	}

	if (mpu9250_status & MPU9250_STATUS_GYRO_ON) {
		// Gyro present
		mpu9250_rregan (MPU9250_REG_GYRO_XOUT_H, (byte*)vap, 6);
#if LITTLE_ENDIAN
		swab (vap, 3);
#endif
		vap += 3;
	}

	if (mpu9250_status & MPU9250_STATUS_COMPASS_ON) {
		// Compass present
		mpu9250_rregcn (MPU9250_REG_AKM_HXL, (byte*)vap, 6);
#if BIG_ENDIAN
		swab (vap, 3);
#endif
		for (i = 0; i < 3; i++)
			vap [i] = (word)
				((lint) vap [i] *
					((lint) mpu9250_cmp_sad [i] + 128));
		vap += 3;
		// Single measurement for next read
		mpu9250_wregc (MPU9250_REG_AKM_CNTL, 0x11);
	}

	if (mpu9250_status & MPU9250_STATUS_TEMP_ON) {
		// Temperature
		mpu9250_rregan (MPU9250_REG_TEMP_OUT_H, (byte*)vap, 2);
#if LITTLE_ENDIAN
		swab (vap, 1);
#endif
		vap += 1;
	}

	_BIC (mpu9250_status, MPU9250_STATUS_EVENT);
}
