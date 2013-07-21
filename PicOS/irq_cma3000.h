//
// Irq service for the CMA3000 sensor
//
if (cma3000_int) {

	char b;

	cma3000_clear;

	// Always read the interrupt status register to remove the interrupt
	// condition
	if ((b = (char) cma3000_rreg (0x05)) != 0) {
		// Ignore altogether if zero (I don't think this can happen
		cma3000_accdata [0] = b;
		cma3000_accdata [1] = (char) cma3000_rreg (0x06);
		cma3000_accdata [2] = (char) cma3000_rreg (0x07);
		cma3000_accdata [3] = (char) cma3000_rreg (0x08);
		if (cma3000_wait_pending) {
			// Wake up the threads waiting for the sensor
			i_trigger ((word)(&cma3000_wait_pending));
			// Only once
			cma3000_wait_pending = NO;
			RISE_N_SHINE;
		}
	}
}
