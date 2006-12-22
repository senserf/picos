#ifndef __pg_xcvcommon_h
#define	__pg_xcvcommon_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/*
 * Common code of CC1000 and DM transveiver driver
 */

#define	RCV_GETIT		0
#define	RCV_CHECKIT		1

#ifndef	DISABLE_CLOCK_INTERRUPT
#define	DISABLE_CLOCK_INTERRUPT	0
#endif

static thread (rcvradio, void)

    entry (RCV_GETIT)

	if (zzv_rxoff) {
Finish:
		if (zzv_txoff) {
			hard_lock;
			if (!xmitter_active)
				hstat (HSTAT_SLEEP);
			hard_drop;
		}
		finish;
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
	zzr_buffp = zzr_buffer;
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
		zzr_buffp = NULL;
		if (receiver_active) {
			zzv_status = 0;
			zzv_istate = IRQ_OFF;
			disable_xcv_timer;
		}
		hard_drop;
	}

	end_rcv;
#if DISABLE_CLOCK_INTERRUPT
	ena_tim;
#endif
	if (zzv_rxoff)
		goto Finish;

#if 0
	diag ("*** RCV: [%d,%d] %x %x %x", zzr_length, zzv_curbit - IRQ_RP0,
		zzr_buffer [0],
		zzr_buffer [1],
		zzr_buffer [2]);
#endif
	/* The length is in words */
	if (zzr_length < MINIMUM_PACKET_LENGTH/2) {
#if 0
		diag ("*** ABT [too short]: [%d,%d] %x %x %x",
			zzr_length,
			zzv_curbit - IRQ_RP0,
			zzr_buffer [0],
			zzr_buffer [1],
			zzr_buffer [2]);
#endif
		proceed (RCV_GETIT);
	}

	/* Check the station Id */
	if (zzv_statid != 0 && zzr_buffer [0] != 0 &&
	    zzr_buffer [0] != zzv_statid) {
		/* Wrong packet */
#if 0
		diag ("*** ABT [wrong SID]: [%d,%d] %x %x %x",
			zzr_length,
			zzv_curbit - IRQ_RP0,
			zzr_buffer [0],
			zzr_buffer [1],
			zzr_buffer [2]);
#endif
		proceed (RCV_GETIT);
	}

	/* Validate checksum */
	if (w_chk (zzr_buffer, zzr_length, 0)) {
#if 0
		diag ("*** ABT [CRC]: [%d,%d] %x %x %x",
			zzr_length,
			zzv_curbit - IRQ_RP0,
			zzr_buffer [0],
			zzr_buffer [1],
			zzr_buffer [2]);
#endif
		proceed (RCV_GETIT);
	}
	 /* Return RSSI in the last checksum byte */
	adc_wait;
	zzr_buffer [zzr_length - 1] = (word) rssi_cnv (adc_value) << 8;

	tcvphy_rcv (zzv_physid, zzr_buffer, zzr_length << 1);

	proceed (RCV_GETIT);

endthread

static INLINE void xmt_down (void) {
/*
 * Executed when the transmitter goes down to properly set the chip
 * state
 */
	if (zzr_buffp == NULL) {
		hard_lock;
		hstat (zzv_rxoff ? HSTAT_SLEEP : HSTAT_RCV);
		hard_drop;
	}
}
	
#define	XM_LOOP		0
#define XM_TXDONE	1
#define	XM_LBS		2

static thread (xmtradio, void)

    int stln;

    entry (XM_LOOP)

	if (zzx_buffer == NULL) {
		// Postpone going off until the current packet is done
		if (zzv_txoff) {
			/* We are off */
			if (zzv_txoff == 3) {
Drain:
				tcvphy_erase (zzv_physid);
				wait (zzv_qevent, XM_LOOP);
				release;
			} else if (zzv_txoff == 1) {
				/* Queue held, transmitter off */
				xmt_down ();
				zzx_backoff = 0;
				finish;
			}
		}

		// Now, as of 060523, tcvphy_get dequeues the buffer
		if ((zzx_buffer = tcvphy_get (zzv_physid, &stln)) != NULL) {
			zzx_buffp = zzx_buffer;
		
			/* This must be even */
			if (stln < 4 || (stln & 1) != 0)
				syserror (EREQPAR, "rxmt/tlength");
			stln >>= 1;

			// Insert the station Id
    			zzx_buffer [0] = zzv_statid;
			// Insert the checksum
			zzx_buffp [stln - 1] = w_chk (zzx_buffp, stln - 1, 0);
			zzx_buffl = zzx_buffp + stln;
		} else {
			// Nothing to transmit
			if (zzv_txoff == 2) {
				/* Draining; stop if the queue is empty */
				zzv_txoff = 3;
				xmt_down ();
				/* Redo */
				goto Drain;
			}
			wait (zzv_qevent, XM_LOOP);
			release;
		}
	}

	// We have a packet

	if (zzx_backoff && !tcv_isurgent (zzx_buffer)) {
		/* We have to wait and the packet is not urgent */
		delay (zzx_backoff, XM_LOOP);
		zzx_backoff = 0;
		wait (zzv_qevent, XM_LOOP);
		release;
	}

	hard_lock;
	if (receiver_busy) {
		// We are receiving. This means the start vector has been
		// recognized. Do not interfere now.
		hard_drop;
		delay (MIN_BACKOFF, XM_LOOP);
		wait (zzv_qevent, XM_LOOP);
		release;
	}

#if LBT_DELAY > 0
	if (receiver_active) {
		// LBS requires the receiver to be listening
		adc_start;
		hard_drop;
		delay (LBT_DELAY, XM_LBS);
		release;
	}
#endif

#if LBT_DELAY > 0
Xmit:
#endif
	LEDI (1, 0);
	// Holding the lock
#if 0
	diag ("SND: %d (%x) %x %x %x %x %x %x", zzx_buffl - zzx_buffp,
		(word) zzx_buffp,
		zzx_buffer [0],
		zzx_buffer [1],
		zzx_buffer [2],
		zzx_buffer [3],
		zzx_buffer [4],
		zzx_buffer [5]);
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
	diag ("SND: DONE (%x)", (word) zzx_buffp);
#endif

#if LBT_DELAY == 0
	// This is to reduce the risk of multiple transmitters livelocks; not
	// needed with LBS
	gbackoff;
#endif
	/* Restart a pending reception */
	hard_lock;
	if (zzr_buffp != NULL) {
		/* Reception queued or active */
		if (!receiver_active) {
			/* Not active, we can keep the lock for a while */
			hstat (HSTAT_RCV);
			start_rcv;
			/* Reception commences asynchronously at hard_drop */
		}
		hard_drop;
		/* The receiver is running at this point */
		tcvphy_end (zzx_buffer);
		zzx_buffer = NULL;
	} else {
		/* This is void: receiver is not active */
		hard_drop;
		sysassert (zzv_status == 0, "xmt illegal chip status");
		tcvphy_end (zzx_buffer);
		zzx_buffer = NULL;
		if (tcvphy_top (zzv_physid) != NULL) {
			/* More to xmit: keep the transmitter up */
			delay (MIN_BACKOFF, XM_LOOP);
			release;
		}
		/* Shut down the transmitter */
		hstat (HSTAT_SLEEP);
	}
	delay (MIN_BACKOFF, XM_LOOP);
	release;

#if LBT_DELAY > 0

    entry (XM_LBS)

	hard_lock;
	if (receiver_busy) {
		hard_drop;
		delay (MIN_BACKOFF, XM_LOOP);
		wait (zzv_qevent, XM_LOOP);
		release;
	}

	if (!receiver_active)
		goto Xmit;

	adc_stop;
	adc_wait;
	if (adc_value < (word)(((long)LBT_THRESHOLD * 4096) / 100)) {
#if 0
		diag ("O %u", adc_value);
#endif
		goto Xmit;
	}
	hard_drop;
	// Backoff
	gbackoff;
#if 0
diag ("D %u %u", adc_value, zzx_backoff);
#endif
	proceed (XM_LOOP);
		
#endif

endthread


#endif
