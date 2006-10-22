
	tarp_ctrl.rcv = tarp_ctrl.snd = tarp_ctrl.fwd = 0;
	tarp_ctrl.flags = 0;
	tarp_ctrl.param = 0xA3;

	tarp_cyclingSeq = 0;

#if TARP_CACHES_MALLOCED

	ddCache = NULL;
	spdCache = NULL;

#endif

#if SPD_RSSI_THRESHOLD
	strong_signal = YES;
#endif

