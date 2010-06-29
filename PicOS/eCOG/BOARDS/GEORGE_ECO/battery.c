#include "sysio.h"
#include "kernel.h"
#include "cc1000.h"
#include "pins.h"

int battery (void) {
//
// Battery status for GEORGE's board
//
	int i;
	word w;

	if (adc_inuse)
		return -1;

	adc_config_read (0x02, 0, 1);

	// Load the battery
	rg.io.gp4_7_out = IO_GP4_7_OUT_EN6_MASK | IO_GP4_7_OUT_SET6_MASK;

	mdelay (10);

	adc_start;

	for (i = 0; i < 5; i++)
		// Skip 5 samples
		adc_advance;

	// Average next 16 readings
	for (w = 0, i = 0; i < 16; i++) {
		adc_wait;
		w += adc_value;
	}

	rg.io.gp4_7_out = IO_GP4_7_OUT_CLR6_MASK;

	adc_stop;
	adc_disable;
	adc_config_rssi;

	// Round and average
	return (int)((w + 8) >> 4);
}
