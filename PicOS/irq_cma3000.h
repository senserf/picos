//
// Irq service for the CMA3000 sensor
//
if (cma3000_int) {

	cma3000_clear;

	// Always read the interrupt status register to remove the interrupt
	// condition

	cma3000_accdata [0] = (char) cma3000_rreg (0x05);

	if (cma3000_accdata [0] && cma3000_event_thread) {
		// Read the acceleration data
		cma3000_accdata [1] = (char) cma3000_rreg (0x06);
		cma3000_accdata [2] = (char) cma3000_rreg (0x07);
		cma3000_accdata [3] = (char) cma3000_rreg (0x08);
		p_trigger (cma3000_event_thread, (word)(&cma3000_event_thread));
		cma3000_event_thread = 0;
		RISE_N_SHINE;
	} else {
		// Zero it out if nobody is waiting, so it can be used as a flag
		// to tell whether accdata is present
		cma3000_accdata [0] = 0;
	}
}
