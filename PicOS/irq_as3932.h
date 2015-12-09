//
// IRQ service for the AS3932 LF wake receiver
//
if (as3932_int) {

	as3932_clear;

	_BIS (as3932_status, AS3932_STATUS_EVENT);
	as3932_disable;

	if (as3932_status & AS3932_STATUS_WAIT) {
		i_trigger ((word)(&as3932_status));
		_BIC (as3932_status, AS3932_STATUS_WAIT);
		RISE_N_SHINE;
	}
}
