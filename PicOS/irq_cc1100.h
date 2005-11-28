#ifndef __pg_irq_cc1100_h
#define __pg_irq_cc1100_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

if (cc1100_int) {

    int len;

    clear_cc1100_int;

    switch (rcv_istate) {

	case RCV_STA:

		if (!fifo_ready)
			return;
#if 0
		len = cc1100_rx_status ();
		if (len < 4)
			goto Abort;
#endif
		// Starting: extract the length and set things up
		zzr_left = cc1100_get_reg (CCxxx0_RXFIFO);
		if (zzr_left == 0 || zzr_left > zzr_buffl) {
			// Zero length or too long
Abort:
			zzr_bptr = NULL;
Return:
			zzr_state &= ~RC_RCVI;
			LEDI (3, 0);
			set_rcv_istate (RCV_IGN);
			rcv_disable_int;
			i_trigger (ETYPE_USER, rxevent);
			return;
		}

		// Length is OK: include the status bytes
		LEDI (3, 1);
		zzr_left += 2;
		zzr_bptr = (byte*) zzr_buffer;

		set_rcv_istate (RCV_GET);

		lock_rcv;

	case RCV_GET:

		while (zzr_left) {
			len = cc1100_rx_status ();
			if (len < 0) {
				// Overflow
				goto Abort;
			}

			if (len > zzr_left)
				// This will never happen
				len = zzr_left;

			if (len == 0) {
				// Wait for interrupt?
				if (zzr_left <= 8)
					// Get assistance from the timer for
					// the last few
					zzr_state |= RC_RCVI;
					// This is because of a bug in FIFOTHR
					// (value 0x01 doesn't seem to work).
					// Sorry about that.
				return;
			}

			if (len < zzr_left) {
				// This circumvents another bug (in FIFO)
				len--;
				if (len == 0) {
					if (zzr_left <= 8)
						zzr_state |= RC_RCVI;
					return;
				}
			}
			cc1100_get_reg_burst (CCxxx0_RXFIFO, zzr_bptr,
				(byte) len);
			zzr_bptr += len;
			zzr_left -= len;
		}

		// We are done
		goto Return;

	default:

		rcv_disable_int;
		return;
    }
}

#endif
