//
// IRQ service for the BMA250 accelerometer
//
if (bma250_int) {

	bma250_clear;

	if (bma250_wait_pending) {
		i_trigger ((word)(&bma250_wait_pending));
		bma250_wait_pending = NO;
		RISE_N_SHINE;
	}
}
