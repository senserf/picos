#ifndef	__pg_xcvcommop_h
#define __pg_xcvcommop_h
//
// Common option processing for xcvcommon-compatible RF modules
//
    case PHYSOPT_STATUS:

	if (zzv_txoff == 0)
		ret = 2;
	if ((zzv_txoff & 1) == 0) {
		// On or draining
		if (zzx_buffer != NULL || tcvphy_top (zzv_physid) != NULL)
			ret |= 4;
	}
	if (zzv_rxoff == 0)
		ret++;
ORet:
	if (val != NULL)
		*val = ret;
	break;

    case PHYSOPT_TXON:

	zzv_txoff = 0;
	if (zzv_rxoff)
		LEDI (0, 1);
	else
		LEDI (0, 2);
	if (!running (xmtradio))
		runthread (xmtradio);
	trigger (zzv_qevent);
	break;

    case PHYSOPT_RXON:

	zzv_rxoff = 0;
	if (zzv_txoff)
		LEDI (0, 1);
	else
		LEDI (0, 2);
	if (!running (rcvradio))
		runthread (rcvradio);
	trigger (rxevent);
	break;

    case PHYSOPT_TXOFF:

	/* Drain */
	zzv_txoff = 2;
	if (zzv_rxoff)
		LEDI (0, 0);
	else
		LEDI (0, 1);
	trigger (zzv_qevent);
	break;

    case PHYSOPT_TXHOLD:

	zzv_txoff = 1;
	if (zzv_rxoff)
		LEDI (0, 0);
	else
		LEDI (0, 1);
	trigger (zzv_qevent);
	break;

    case PHYSOPT_RXOFF:

	zzv_rxoff = 1;
	if (zzv_txoff)
		LEDI (0, 0);
	else
		LEDI (0, 1);
	hard_lock;
	if (receiver_active) {
		LEDI (2, 0);
		// Force RCV interrupt reset: the receiver process may
		// not be given a chance to run
		zzv_status = 0;
		zzv_istate = IRQ_OFF;
		disable_xcv_timer;
	}
	hard_drop;
	adc_disable;
	trigger (rxevent);
	break;

    case PHYSOPT_CAV:

	/* Force an explicit backoff */
	if (val == NULL)
		zzx_backoff = 0;
	else
		zzx_backoff = *val;
	trigger (zzv_qevent);
	break;

    case PHYSOPT_SETSID:

	zzv_statid = (val == NULL) ? 0 : *val;
	break;

    case PHYSOPT_GETSID:

	ret = (int) zzv_statid;
	goto ORet;

    case PHYSOPT_GETMAXPL:

	ret = (int) ((zzr_buffl - zzr_buffer - 1) << 1);
	goto ORet;
#endif
