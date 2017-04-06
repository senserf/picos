//
// IRQ service for the MAX30102 oximeter
//
if (max30102_int) {

	max30102_clear;
	max30102_disable;
	i_trigger (max30102_event);
	RISE_N_SHINE;
}
