
	_da (tarp_ctrl).rcv = _da (tarp_ctrl).snd = _da (tarp_ctrl).fwd = 0;
	_da (tarp_ctrl).flags = 0;
	_da (tarp_ctrl).param = 0xA3;

	tarp_cyclingSeq = 0;

	// These defaults will be reset by the praxis
	_da (net_id)	 	= 85;
	_da (local_host) 	= 97;
	_da (master_host)	= 1;

#if TARP_CACHES_MALLOCED

	ddCache = NULL;
	spdCache = NULL;

#endif

#if SPD_RSSI_THRESHOLD
	strong_signal = YES;
#endif

