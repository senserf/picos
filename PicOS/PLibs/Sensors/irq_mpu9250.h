//
// IRQ service for the MPU9250 accelerometer
//
if (mpu9250_int) {

	mpu9250_clear;

	_BIS (mpu9250_status, MPU9250_STATUS_EVENT);
	mpu9250_disable;

	if (mpu9250_status & MPU9250_STATUS_WAIT) {
		i_trigger ((aword)(&mpu9250_status));
		_BIC (mpu9250_status, MPU9250_STATUS_WAIT);
		RISE_N_SHINE;
	}
}
