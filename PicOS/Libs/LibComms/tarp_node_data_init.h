
	_da (tarp_ctrl).rcv = _da (tarp_ctrl).snd = _da (tarp_ctrl).fwd = 0;
	_da (tarp_ctrl).pp_urg = 0;
	_da (tarp_ctrl).pp_widen = 0;
	_da (tarp_ctrl).spare = 0;
	_da (tarp_ctrl).param = TARP_DEF_PARAMS;
	_da (tarp_ctrl).rssi_th = TARP_DEF_RSSITH;
	_da (tarp_ctrl).ssignal = YES;

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
	_da (tarp_pxopts) = TARP_DEF_PXOPTS;
#endif

	tarp_cyclingSeq = 0;

	// These defaults will be reset by the praxis
	_da (net_id)	 	= (word) preinit ("NID");
	_da (local_host) 	= (word)((lword) preinit ("HID"));
	_da (master_host)	= (word) preinit ("MHOST");

#if TARP_CACHES_MALLOCED

	ddCache = NULL;
	spdCache = NULL;

#endif

