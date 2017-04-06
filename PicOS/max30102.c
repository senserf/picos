#include "sysio.h"
#include "max30102.h"

#define	I2C_WRITE_ADDR	0xAE
#define	I2C_READ_ADDR	0xAF

// Registers
#define REG_INTR_STATUS_1 	0x00
#define REG_INTR_STATUS_2 	0x01
#define REG_INTR_ENABLE_1 	0x02
#define REG_INTR_ENABLE_2 	0x03
#define REG_FIFO_WR_PTR 	0x04
#define REG_OVF_COUNTER 	0x05
#define REG_FIFO_RD_PTR 	0x06
#define REG_FIFO_DATA 		0x07
#define REG_FIFO_CONFIG 	0x08
#define REG_MODE_CONFIG 	0x09
#define REG_SPO2_CONFIG 	0x0A
#define REG_LED1_PA 		0x0C
#define REG_LED2_PA 		0x0D
#define REG_PILOT_PA 		0x10
#define REG_MULTI_LED_CTRL1 	0x11
#define REG_MULTI_LED_CTRL2 	0x12
#define REG_TEMP_INTR 		0x1F
#define REG_TEMP_FRAC 		0x20
#define REG_TEMP_CONFIG 	0x21
#define REG_PROX_INT_THRESH 	0x30
#define REG_REV_ID 		0xFE
#define REG_PART_ID 		0xFF

// ============================================================================

static void i2c_start () {

	// SCL and SDA are always parked high
	max30102_sda_lo;
	// The normal state of SCL while active is low
	max30102_scl_lo;
}

static void i2c_stop () {

	max30102_sda_lo;
	max30102_scl_hi;
	max30102_sda_hi;
}

static void i2c_restart () {

	max30102_sda_hi;
	max30102_scl_hi;
	i2c_start ();
}

// ============================================================================

static Boolean wnga (byte b) {
//
// Write a byte and return ACK
//
	int i;

	for (i = 0; i < 8; i++) {
		if (b & 0x80)
			max30102_sda_hi;
		else
			max30102_sda_lo;
		max30102_scl_hi;
		max30102_scl_lo;
		b <<= 1;
	}

	// Read the ACK bit

	max30102_sda_hi;
	max30102_scl_hi;
	i = max30102_sda;
	max30102_scl_lo;
	return (Boolean)i;
}

static byte rdb (Boolean ack) {
//
// Read a byte
//
	byte r;
	int i;

	// Drop SDA if holding
	max30102_sda_hi;

	for (r = 0, i = 0; i < 8; i++) {
		max30102_scl_hi;
		r <<= 1;
		if (max30102_sda) 
			r |= 1;
		max30102_scl_lo;
	}

	// ACK/NAK
	if (ack)
		max30102_sda_lo;
	max30102_scl_hi;
	max30102_scl_lo;

	return r;
}

void max30102_wreg (byte reg, byte data) {
//
// Write a register
//
	while (1) {
		// Timeouts ?
		i2c_restart ();
		if (!wnga (I2C_WRITE_ADDR) && !wnga (reg) && !wnga (data)) {
			i2c_stop ();
			return;
		}
	}
}

#define	wreg(a,b)	max30102_wreg (a, b)

byte max30102_rreg (byte reg) {
//
// Read a register
//
	byte r;

	while (1) {
		i2c_restart ();
		if (!wnga (I2C_WRITE_ADDR) && !wnga (reg)) {
			i2c_restart ();
			if (!wnga (I2C_READ_ADDR)) {
				r = rdb (0);
				i2c_stop ();
				return r;
			}
		}
udelay (100);
	}
}

#define	rreg(a)		max30102_rreg (a)

// ============================================================================

void max30102_start () {
//
// Initialization (copied from the Arduino reference code)
//
	// Reset
	wreg (REG_MODE_CONFIG, 		0x40);
	udelay (100);
	wreg (REG_INTR_ENABLE_1,	0xc0);
	wreg (REG_INTR_ENABLE_2,	0x00);
	// wreg (REG_FIFO_WR_PTR, 		0x00);
	// wreg (REG_OVF_COUNTER, 		0x00);
	// wreg (REG_FIFO_RD_PTR, 		0x00);
	// Four averaged samples, interrupt on anything in FIFO
	wreg (REG_FIFO_CONFIG, 		0x4f);
	// SPO2 mode
	wreg (REG_MODE_CONFIG, 		0x03);
	// Range 15.63, 100 sps [4 avg], led power max
	wreg (REG_SPO2_CONFIG, 		0x27);
	wreg (REG_LED1_PA, 		0x24);
	wreg (REG_LED2_PA, 		0x24);
	wreg (REG_PILOT_PA,		0x7f);
}
	
void max30102_read_sample (word st, max30102_sample_t *re,
						max30102_sample_t *ir) {

	lword s;

	if (!max30102_data) {
		// Nothing to read, wait for int
		when (max30102_event, st);
		max30102_enable;
		release;
	}


	// Do we need this to clear the interrupt?
	rreg (REG_INTR_STATUS_1);
  	// rreg (REG_INTR_STATUS_2);

	*re = *ir = 0;

	i2c_restart ();
	wnga (I2C_WRITE_ADDR);
	wnga (REG_FIFO_DATA);
	i2c_restart ();
	wnga (I2C_READ_ADDR);

	s  = ((lword) rdb (1)) << 16;
	s |= ((lword) rdb (1)) << 8;
	s |= ((lword) rdb (1));

#if MAX30102_SHORT_SAMPLES
	*re = (word)(s /* >> 2 */);
#else
	*re = s &= 0x03ffff;
#endif
	s  = ((lword) rdb (1)) << 16;
	s |= ((lword) rdb (1)) << 8;
	s |= ((lword) rdb (0));

#if MAX30102_SHORT_SAMPLES
	*ir = (word)(s /* >> 2 */);
#else
	*ir = s &= 0x03ffff;
#endif
	i2c_stop ();
}

void max30102_stop () {

	// Reset
	wreg (REG_MODE_CONFIG, 		0x40);
	udelay (100);

	// Shutdown
	wreg (REG_MODE_CONFIG,		0x80);
}
