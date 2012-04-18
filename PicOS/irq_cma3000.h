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
		if (cma3000_event_thread) {
			// Wake up the thread waiting for the sensor
			p_trigger (cma3000_event_thread,
				(word)(&cma3000_event_thread));
			// Only once
			cma3000_event_thread = 0;
			RISE_N_SHINE;
		}
	}
}
