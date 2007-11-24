
	_da (tarp_ctrl).rcv = _da (tarp_ctrl).snd = _da (tarp_ctrl).fwd = 0;
	_da (tarp_ctrl).flags = 0;
	_da (tarp_ctrl).param = 0xA3;
	_da (tarp_ctrl).rssi_th = DEFAULT_RSSI_THOLD;
	_da (tarp_ctrl).ssignal = YES;

	tarp_cyclingSeq = 0;

	// These defaults will be reset by the praxis
	_da (net_id)	 	= (word) preinit ("NID");
	_da (local_host) 	= (word)((lword) preinit ("HID"));
	_da (master_host)	= (word) preinit ("MHOST");

#if TARP_CACHES_MALLOCED

	ddCache = NULL;
	spdCache = NULL;

#endif

