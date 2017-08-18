//
// IRQ service for the TMP007 thermopile
//
if (tmp007_int) {

	tmp007_clear;

	_BIS (tmp007_status, TMP007_STATUS_EVENT);
	tmp007_disable;

	if (tmp007_status & TMP007_STATUS_WAIT) {
		i_trigger ((aword)(&tmp007_status));
		_BIC (tmp007_status, TMP007_STATUS_WAIT);
		RISE_N_SHINE;
	}
}
