#ifndef __pg_xcvcommon_h
#define	__pg_xcvcommon_h

/*
 * Common code of CHIPCON and DM transveiver driver
 */

#define	RCV_GETIT	0
#define	RCV_CHECKIT	1

static process (rcvradio, void)

    entry (RCV_GETIT)

	if (zzv_rdbk->rxoff) {
Finish:
		if (zzv_rdbk->txoff) {
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

	/* Tell the receiver to stop in case we have been interrupted */
	if (receiver_active) {
		hard_lock;
		zzr_buffp = NULL;
		if (receiver_active) {
			zzv_status = 0;
			zzv_istate = IRQ_OFF;
			disable_xcv_timer;
			gbackoff;
		}
		hard_drop;
	}

	if (zzv_rdbk->rxoff)
		/* rcvevent can be also triggered by this */
		goto Finish;

#if 0
	diag ("RCV: %d %x %x %x %x %x %x", zzr_length,
		zzr_buffer [0],
		zzr_buffer [1],
		zzr_buffer [2],
		zzr_buffer [3],
		zzr_buffer [4],
		zzr_buffer [5]);
#endif
	/* The length is in words */
	if (zzr_length < MINIMUM_PACKET_LENGTH/2)
		proceed (RCV_GETIT);

	/* Check the station Id */
	if (zzv_rdbk->statid != 0 && zzr_buffer [0] != 0 &&
	    zzr_buffer [0] != zzv_rdbk->statid)
		/* Wrong packet */
		proceed (RCV_GETIT);

	/* Validate checksum */
	if (w_chk (zzr_buffer, zzr_length))
		proceed (RCV_GETIT);
	 /* Return RSSI in the last checksum byte */
	zzr_buffer [zzr_length - 1] = (word) rssi_cnv ();

	tcvphy_rcv (zzv_rdbk->physid, zzr_buffer, zzr_length << 1);

	proceed (RCV_GETIT);

    nodata;

endprocess (1)

static INLINE void xmt_down (void) {
/*
 * Executed when the transmitter goes down to properly set the chip
 * state
 */
	if (zzr_buffp == NULL) {
		hard_lock;
		hstat (zzv_rdbk->rxoff ? HSTAT_SLEEP : HSTAT_RCV);
		hard_drop;
	}
}
	
#define	XM_LOOP		0
#define XM_TXDONE	1

static process (xmtradio, void)

    int stln;

    entry (XM_LOOP)

	if (zzv_rdbk->txoff) {
		/* We are off */
		if (zzv_rdbk->txoff == 3) {
Drain:
			tcvphy_erase (zzv_rdbk->physid);
			wait (zzv_rdbk->qevent, XM_LOOP);
			release;
		} else if (zzv_rdbk->txoff == 1) {
			/* Queue held, transmitter off */
			xmt_down ();
			zzv_rdbk->backoff = 0;
			finish;
		}
	}

	if ((stln = tcvphy_top (zzv_rdbk->physid)) == 0) {
		/* Packet queue is empty */
		if (zzv_rdbk->txoff == 2) {
			/* Draining; stop xmt if the output queue is empty */
			zzv_rdbk->txoff = 3;
			xmt_down ();
			/* Redo */
			goto Drain;
		}
		wait (zzv_rdbk->qevent, XM_LOOP);
		release;
	}

	if (zzv_rdbk->backoff && stln < 2) {
		/* We have to wait and the packet is not urgent */
		delay (zzv_rdbk->backoff, XM_LOOP);
		zzv_rdbk->backoff = 0;
		wait (zzv_rdbk->qevent, XM_LOOP);
		release;
	}

	hard_lock;
	if (receiver_busy) {
		hard_drop;
		delay (zzv_rdbk->delmnbkf, XM_LOOP);
		wait (zzv_rdbk->qevent, XM_LOOP);
		release;
	}
	LEDI (2, 0);
	if ((zzx_buffp = tcvphy_get (zzv_rdbk->physid, &stln)) != NULL) {

		// Holding the lock

		zzx_buffer = zzx_buffp;

		/* This must be even */
		if (stln < 4 || (stln & 1) != 0)
			syserror (EREQPAR, "xmt/tlength");
		stln >>= 1;

		// Insert the station Id
		if (zzv_rdbk->statid)
	    		zzr_buffer [0] = zzv_rdbk->statid;
		// Insert the checksum
		zzx_buffp [stln - 1] = w_chk (zzx_buffp, stln - 1);
#if 0
		diag ("SND: %d (%x) %x %x %x %x %x %x", stln, (word) zzx_buffp,
			zzx_buffer [0],
			zzx_buffer [1],
			zzx_buffer [2],
			zzx_buffer [3],
			zzx_buffer [4],
			zzx_buffer [5]);
#endif
		hstat (HSTAT_XMT);
		zzx_buffl = zzx_buffp + stln;
		start_xmt;
		/*
		 * Now, txevent is used exclusively for the end of transmission.
		 * We HAVE to perceive it, as we are responsible for restarting
		 * a pending reception.
		 */
		wait (txevent, XM_TXDONE);
		hard_drop;
		release;
	}
	hard_drop;
	// We should never get here
	proceed (XM_LOOP);

    entry (XM_TXDONE)

#if 0
	diag ("SND: DONE (%x)", (word) zzx_buffp);
#endif

	/* Restart a pending reception */
	gbackoff;
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
		/* Delay for a while before next transmission */
		delay (zzv_rdbk->delmnbkf, XM_LOOP);
	} else {
		/* This is void: receiver is not active */
		hard_drop;
		sysassert (zzv_status == 0, "xmt illegal chip status");
		tcvphy_end (zzx_buffer);
		if (tcvphy_top (zzv_rdbk->physid)) {
			/* More to xmit: keep the transmitter up */
			delay (zzv_rdbk->delxmspc, XM_LOOP);
			release;
		}
		/* Shut down the transmitter */
		hstat (HSTAT_SLEEP);
		delay (zzv_rdbk->delxmspc, XM_LOOP);
	}
	release;

    nodata;

endprocess (1)


#endif
