if (pin_sensor_int) {
	i_trigger ((aword)(&__input_pins));
	__pinsen_disable_and_clear;
	RISE_N_SHINE;
}
