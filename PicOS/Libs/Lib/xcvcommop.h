#ifndef	__pg_xcvcommop_h
#define __pg_xcvcommop_h
//
// Common option processing for xcvcommon-compatible RF modules
//
    case PHYSOPT_STATUS:

	if (__pi_v_txoff == 0)
		ret = 2;
	if ((__pi_v_txoff & 1) == 0) {
		// On or draining
		if (__pi_x_buffer != NULL || tcvphy_top (__pi_v_physid) != NULL)
			ret |= 4;
	}
	if (__pi_v_rxoff == 0)
		ret++;
ORet:
	if (val != NULL)
		*val = ret;
	break;

    case PHYSOPT_TXON:

	__pi_v_txoff = 0;
	if (__pi_v_rxoff)
		LEDI (0, 1);
	else
		LEDI (0, 2);
	trigger (__pi_v_qevent);
	break;

    case PHYSOPT_RXON:

	__pi_v_rxoff = 0;
	if (__pi_v_txoff)
		LEDI (0, 1);
	else
		LEDI (0, 2);
	trigger (rxevent);
	break;

    case PHYSOPT_TXOFF:

	/* Drain */
	__pi_v_txoff = 2;
	if (__pi_v_rxoff)
		LEDI (0, 0);
	else
		LEDI (0, 1);
	trigger (__pi_v_qevent);
	break;

    case PHYSOPT_TXHOLD:

	__pi_v_txoff = 1;
	if (__pi_v_rxoff)
		LEDI (0, 0);
	else
		LEDI (0, 1);
	trigger (__pi_v_qevent);
	break;

    case PHYSOPT_RXOFF:

	__pi_v_rxoff = 1;
	if (__pi_v_txoff)
		LEDI (0, 0);
	else
		LEDI (0, 1);
	hard_lock;
	if (receiver_active) {
		LEDI (2, 0);
		// Force RCV interrupt reset: the receiver process may
		// not be given a chance to run
		__pi_v_status = 0;
		__pi_v_istate = IRQ_OFF;
		disable_xcv_timer;
	}
	hard_drop;
	adc_disable;
	trigger (rxevent);
	break;

    case PHYSOPT_CAV:

	/* Force an explicit backoff */
	if (val == NULL)
		__pi_x_backoff = 0;
	else
		__pi_x_backoff = *val;
	trigger (__pi_v_qevent);
	break;

    case PHYSOPT_SETSID:

	__pi_v_statid = (val == NULL) ? 0 : *val;
	break;

    case PHYSOPT_GETSID:

	ret = (int) __pi_v_statid;
	goto ORet;

    case PHYSOPT_GETMAXPL:

	ret = (int) ((__pi_r_buffl - __pi_r_buffer - 1) << 1);
	goto ORet;
#endif
