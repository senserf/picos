#ifndef __pg_xcvcommon_h
#define	__pg_xcvcommon_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/*
 * Common code of CHIPCON and DM transveiver driver
 */

#define	RCV_GETIT	0
#define	RCV_CHECKIT	1

#ifndef	LBS_TIME
#define	LBS_TIME	0
#endif

static process (rcvradio, void)

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

	if (zzv_rxoff)
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
	if (zzv_statid != 0 && zzr_buffer [0] != 0 &&
	    zzr_buffer [0] != zzv_statid)
		/* Wrong packet */
		proceed (RCV_GETIT);

	/* Validate checksum */
	if (w_chk (zzr_buffer, zzr_length))
		proceed (RCV_GETIT);
	 /* Return RSSI in the last checksum byte */
	zzr_buffer [zzr_length - 1] = (word) rssi_cnv ();

	tcvphy_rcv (zzv_physid, zzr_buffer, zzr_length << 1);

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
		hstat (zzv_rxoff ? HSTAT_SLEEP : HSTAT_RCV);
		hard_drop;
	}
}
	
#define	XM_LOOP		0
#define XM_TXDONE	1

static process (xmtradio, void)

    int stln;

    entry (XM_LOOP)

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

	if ((stln = tcvphy_top (zzv_physid)) == 0) {
		/* Packet queue is empty */
		if (zzv_txoff == 2) {
			/* Draining; stop xmt if the output queue is empty */
			zzv_txoff = 3;
			xmt_down ();
			/* Redo */
			goto Drain;
		}
		wait (zzv_qevent, XM_LOOP);
		release;
	}

	if (zzx_backoff && stln < 2) {
		/* We have to wait and the packet is not urgent */
		delay (zzx_backoff, XM_LOOP);
		zzx_backoff = 0;
		wait (zzv_qevent, XM_LOOP);
		release;
	}

	hard_lock;
	if (receiver_busy) {
		hard_drop;
		delay (zzx_delmnbkf, XM_LOOP);
		wait (zzv_qevent, XM_LOOP);
		release;
	}
	LEDI (2, 0);
	if ((zzx_buffp = tcvphy_get (zzv_physid, &stln)) != NULL) {

		// Holding the lock

		zzx_buffer = zzx_buffp;

		/* This must be even */
		if (stln < 4 || (stln & 1) != 0)
			syserror (EREQPAR, "xmt/tlength");
		stln >>= 1;

		// Insert the station Id
		if (zzv_statid)
	    		zzr_buffer [0] = zzv_statid;
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

#if LBS_TIME == 0
	// This is to reduce the risk of multiple transmitters livelocks
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
		/* Delay for a while before next transmission */
		delay (zzx_delmnbkf, XM_LOOP);
	} else {
		/* This is void: receiver is not active */
		hard_drop;
		sysassert (zzv_status == 0, "xmt illegal chip status");
		tcvphy_end (zzx_buffer);
		if (tcvphy_top (zzv_physid)) {
			/* More to xmit: keep the transmitter up */
			delay (zzx_delxmspc, XM_LOOP);
			release;
		}
		/* Shut down the transmitter */
		hstat (HSTAT_SLEEP);
		delay (zzx_delxmspc, XM_LOOP);
	}
	release;

    nodata;

endprocess (1)


#endif