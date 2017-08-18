#ifndef __pg_xcvcommon_h
#define	__pg_xcvcommon_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/*
 * Common code of CC1000 and DM transceiver driver
 */

#define	RCV_GETIT		0
#define	RCV_CHECKIT		1

#ifndef	DISABLE_CLOCK_INTERRUPT
#define	DISABLE_CLOCK_INTERRUPT	0
#endif

static thread (rcvradio)

    entry (RCV_GETIT)

	if (__pi_v_rxoff) {
Finish:
		if (__pi_v_txoff) {
			hard_lock;
			if (!xmitter_active)
				hstat (HSTAT_SLEEP);
			hard_drop;
		}
		wait (rxevent, RCV_GETIT);
		release;
	}
	/*
	 * Initialize things for reception. Note that the transmitter may be
	 * running at this time. We block the interrupts for a little while
	 * to avoid race with the transmitter.
	 */
#if DISABLE_CLOCK_INTERRUPT
	// DEBUG ONLY !!!
	dis_tim;
#endif
	hard_lock;
	/* This also resets the receiver if it was running */
	__pi_r_buffp = __pi_r_buffer;
	if (!xmitter_active) {
		hstat (HSTAT_RCV);
		start_rcv;
	}
	hard_drop;
	/*
	 * Formally, we have a race here because we are not holding the lock
	 * while issuing the wait. However, it is physically impossible for
	 * any reception to complete while we are finishing this state.
	 */
	wait (rxevent, RCV_CHECKIT);
	release;

    entry (RCV_CHECKIT)

	if (receiver_active) {
		/*
		 * Abort the reception if we have been interrupted, e.g.,
		 * by an RXOFF request
		 */
		hard_lock;
		if (receiver_active) {
			__pi_v_status = 0;
			__pi_v_istate = IRQ_OFF;
			disable_xcv_timer;
		}
		hard_drop;
	}
	__pi_r_buffp = NULL;
	end_rcv;
#if DISABLE_CLOCK_INTERRUPT
	ena_tim;
#endif
	if (__pi_v_rxoff)
		goto Finish;

#if 0
	diag ("*** RCV: [%d] %x %x %x", __pi_r_length,
		__pi_r_buffer [0],
		__pi_r_buffer [1],
		__pi_r_buffer [2]);
#endif
	/* The length is in words */
	if (__pi_r_length < MINIMUM_PACKET_LENGTH/2) {
#if 0
		diag ("*** ABT [too short]: [%d] %x %x %x",
			__pi_r_length,
			__pi_r_buffer [0],
			__pi_r_buffer [1],
			__pi_r_buffer [2]);
#endif
		proceed (RCV_GETIT);
	}

	/* Check the station Id */
	if (__pi_v_statid != 0 && __pi_r_buffer [0] != 0 &&
	    __pi_r_buffer [0] != __pi_v_statid) {
		/* Wrong packet */
#if 0
		diag ("*** ABT [wrong SID]: [%d] %x %x %x",
			__pi_r_length,
			__pi_r_buffer [0],
			__pi_r_buffer [1],
			__pi_r_buffer [2]);
#endif
		proceed (RCV_GETIT);
	}

	/* Validate checksum */
	if (w_chk (__pi_r_buffer, __pi_r_length, 0)) {
#if 0
		diag ("*** ABT [CRC]: [%d] %x %x %x",
			__pi_r_length,
			__pi_r_buffer [0],
			__pi_r_buffer [1],
			__pi_r_buffer [2]);
#endif
		proceed (RCV_GETIT);
	}
	/* Return RSSI in the last checksum byte */

// diag ("V0 %x", adc_value);
	__pi_r_buffer [__pi_r_length - 1] = (word) rssi_cnv (adc_value) << 8;
// diag ("V1 %x", adc_value);

	tcvphy_rcv (__pi_v_physid, __pi_r_buffer, __pi_r_length << 1);

	proceed (RCV_GETIT);

endthread

static INLINE void xmt_down (void) {
/*
 * Executed when the transmitter goes down to properly set the chip
 * state
 */
	if (__pi_r_buffp == NULL) {
		hard_lock;
		hstat (__pi_v_rxoff ? HSTAT_SLEEP : HSTAT_RCV);
		hard_drop;
	}
}
	
#define	XM_LOOP		0
#define XM_TXDONE	1
#define	XM_LBS		2

static thread (xmtradio)

    int stln;

    entry (XM_LOOP)

	if (__pi_x_buffer == NULL) {
		// Postpone going off until the current packet is done
		if (__pi_v_txoff) {
			/* We are off */
			if (__pi_v_txoff == 3) {
Drain:
				tcvphy_erase (__pi_v_physid);
			} else if (__pi_v_txoff == 1) {
				/* Queue held, transmitter off */
				xmt_down ();
				__pi_x_backoff = 0;
			}
			wait (__pi_v_qevent, XM_LOOP);
			release;
		}

		// Now, as of 060523, tcvphy_get dequeues the buffer
		if ((__pi_x_buffer = tcvphy_get (__pi_v_physid, &stln)) !=
		    NULL) {
			__pi_x_buffp = __pi_x_buffer;
		
			/* This must be even */
			if (stln < 4 || (stln & 1) != 0)
				syserror (EREQPAR, "rxmt/tlength");
			stln >>= 1;

			// Insert the station Id
    			__pi_x_buffer [0] = __pi_v_statid;
			// Insert the checksum
			__pi_x_buffp [stln - 1] = w_chk (__pi_x_buffp,
				stln - 1, 0);
			__pi_x_buffl = __pi_x_buffp + stln;
		} else {
			// Nothing to transmit
			if (__pi_v_txoff == 2) {
				/* Draining; stop if the queue is empty */
				__pi_v_txoff = 3;
				xmt_down ();
				/* Redo */
				goto Drain;
			}
			wait (__pi_v_qevent, XM_LOOP);
			release;
		}
	}

	// We have a packet

	if (__pi_x_backoff && !tcv_isurgent (__pi_x_buffer)) {
		/* We have to wait and the packet is not urgent */
		delay (__pi_x_backoff, XM_LOOP);
		__pi_x_backoff = 0;
		wait (__pi_v_qevent, XM_LOOP);
		release;
	}

	hard_lock;
	if (receiver_busy) {
		// We are receiving. This means the start vector has been
		// recognized. Do not interfere now.
		hard_drop;
		delay (RADIO_LBT_MIN_BACKOFF, XM_LOOP);
		wait (__pi_v_qevent, XM_LOOP);
		release;
	}

#if RADIO_LBT_DELAY > 0
	if (receiver_active) {
		// LBT requires the receiver to be listening
		adc_start_refon;
		hard_drop;
		delay (RADIO_LBT_DELAY, XM_LBS);
		release;
	}
#endif

#if RADIO_LBT_DELAY > 0
Xmit:
#endif
	LEDI (1, 0);
	// Holding the lock
#if 0
	diag ("SND: %d (%x) %x %x %x %x %x %x", __pi_x_buffl - __pi_x_buffp,
		(word) __pi_x_buffp,
		__pi_x_buffer [0],
		__pi_x_buffer [1],
		__pi_x_buffer [2],
		__pi_x_buffer [3],
		__pi_x_buffer [4],
		__pi_x_buffer [5]);
#endif

#if DISABLE_CLOCK_INTERRUPT
	dis_tim;
#endif
	hstat (HSTAT_XMT);
	start_xmt;
	/*
	 * Now, txevent is used exclusively for the end of transmission.
	 * We HAVE to perceive it, as we are responsible for restarting
	 * a pending reception.
	 */
	wait (txevent, XM_TXDONE);
	hard_drop;
	release;

    entry (XM_TXDONE)

#if DISABLE_CLOCK_INTERRUPT
	ena_tim;
#endif

#if 0
	diag ("SND: DONE (%x)", (word) __pi_x_buffp);
#endif

#if RADIO_LBT_DELAY == 0
	// This is to reduce the risk of multiple transmitters livelocks; not
	// needed with LBS
	gbackoff (RADIO_LBT_BACKOFF_EXP);
#endif
	/* Restart a pending reception */
	hard_lock;
	if (__pi_r_buffp != NULL) {
		/* Reception queued or active */
		if (!receiver_active) {
			/* Not active, we can keep the lock for a while */
			hstat (HSTAT_RCV);
			start_rcv;
			/* Reception commences asynchronously at hard_drop */
		}
		hard_drop;
		/* The receiver is running at this point */
		tcvphy_end (__pi_x_buffer);
		__pi_x_buffer = NULL;
	} else {
		/* This is void: receiver is not active */
		hard_drop;
		sysassert (__pi_v_status == 0, "xmt illegal chip status");
		tcvphy_end (__pi_x_buffer);
		__pi_x_buffer = NULL;
		if (tcvphy_top (__pi_v_physid) != NULL) {
			/* More to xmit: keep the transmitter up */
			delay (RADIO_LBT_MIN_BACKOFF, XM_LOOP);
			release;
		}
		/* Shut down the transmitter */
		hstat (HSTAT_SLEEP);
	}
	delay (RADIO_LBT_MIN_BACKOFF, XM_LOOP);
	release;

#if RADIO_LBT_DELAY > 0

    entry (XM_LBS)

	hard_lock;
	if (receiver_busy) {
		hard_drop;
		delay (RADIO_LBT_MIN_BACKOFF, XM_LOOP);
		wait (__pi_v_qevent, XM_LOOP);
		release;
	}

	if (!receiver_active)
		goto Xmit;

	adc_stop;
	adc_wait;
	adc_disable;

	if (lbt_ok (adc_value)) {
#if 0
		diag ("O %u", adc_value);
#endif
		goto Xmit;
	}

	hard_drop;
	// Backoff
	gbackoff (RADIO_LBT_BACKOFF_EXP);
#if 0
	diag ("D %u %u", adc_value, __pi_x_backoff);
#endif
	proceed (XM_LOOP);
		
#endif

endthread

static int run_rf_driver () {

	return runthread (rcvradio) && runthread (xmtradio);
}

#endif
