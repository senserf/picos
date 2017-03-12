
	_da (tarp_ctrl).rcv = _da (tarp_ctrl).snd = _da (tarp_ctrl).fwd = 0;
	_da (tarp_ctrl).pp_urg = 0;
	_da (tarp_ctrl).pp_widen = 0;
	_da (tarp_ctrl).mchg_msg = 0;
	_da (tarp_ctrl).mchg_set = 0;
	_da (tarp_ctrl).spare = 0;
	_da (tarp_ctrl).param = TARP_DEF_PARAMS;
	_da (tarp_ctrl).rssi_th = TARP_DEF_RSSITH;
	_da (tarp_ctrl).ssignal = YES;

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
	_da (tarp_pxopts) = TARP_DEF_PXOPTS;
#endif

	tarp_cyclingSeq = 0;

// see the comments about locaspot 1.81 vs. 1.83
//	_da (net_id) = _da (master_host) = 
//		(word) ((lword) preinit ("HID") >> 16);
	_da (net_id) =(word) ((lword) preinit ("HID") >> 16);
	_da (master_host) = (word) preinit ("MHOST"); // I don't think this is in use in locaspot
	_da (local_host) 	= (word)((lword) preinit ("HID"));

#if TARP_CACHES_MALLOCED

	ddCache = NULL;
	spdCache = NULL;

#endif

