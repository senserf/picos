//
// IRQ service for the BMA250 accelerometer
//
if (bma250_int) {

	bma250_clear;

	_BIS (bma250_status, BMA250_STATUS_EVENT);
	bma250_disable;

	if (bma250_status & BMA250_STATUS_WAIT) {
		i_trigger ((aword)(&bma250_status));
		_BIC (bma250_status, BMA250_STATUS_WAIT);
		RISE_N_SHINE;
	}
}
