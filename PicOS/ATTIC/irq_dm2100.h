#ifndef __pg_irq_dm2100_h
#define __pg_irq_dm2100_h

/*
 * This code handles signal strobes. For transmission, the strobe is triggered
 * on every timed signal transition edge. For reception, the strobe is triggered
 * by the (expected) edge of the input signal.
 */

    switch (zzv_istate) {

	case IRQ_OFF:

		// Stop the automaton
		disable_xcv_timer;
		zzv_status = 0;
		break;

	case IRQ_XPR:

		// Transmitting low
		SL1;
		zzv_prmble--;
		zzv_istate = zzv_prmble ? IRQ_XPQ : IRQ_XPT;
		break;

	case IRQ_XPQ:

		// Transmitting high
		TRA (1, XPR);
		break;

	case IRQ_XPT:

		// Preamble trailer, two consecutive ones
		TRA (2, TU0);
		break;
	/*
	 * This is the configuration of states for transmission: we are
	 * generous with code but frugal with variables
	 *
	 * TURN == SENDING 0: (TU0)
	 *
	 *   00:    3X                            ->  TU1
	 *   01:    2X         1X (T01)           ->  TU0
	 *   10:    1X         1X (T02), 1X (T03) ->  TU1
	 *   11:    1X         2X (T04)           ->  TU0
	 *
	 * TURN == SENDING 1: (TU1)
 	 *
	 *   00:    1X         2X (T05)           ->  TU1
	 *   01:    1X         1X (T06), 1X (T07) ->  TU0
	 *   10:    2X         1X (T08)           ->  TU1
	 *   11:    3X                            ->  TU0
	 */

	case IRQ_TU0:

		if (zzv_curbit == 16) {
			// Done with the word
			if (++zzx_buffp == zzx_buffl) {
				// Done with the packet, send at least three
				// zeros
				TRA (3, EXM);
				break;
			}
			zzv_curbit = 0;
		}
				
		switch ((*zzx_buffp >> zzv_curbit) & 0x3) {

			// This is the distributor of cases

			case 0:		// 00
				TRA (3, TU1);
				break;
			case 1:		// 10 (bits go from the right)
				TRA (1, T02);
				break;
			case 2:		// 01
				TRA (2, T01);
				break;
			default:	// 11
				TRA (1, T04);
		}
		zzv_curbit += 2;
		break;

	case IRQ_T01:

		SL1;
		zzv_istate = IRQ_TU0;
		break;

	case IRQ_T02:

		TRA (1, T03);
		break;

	case IRQ_T03:

		TRA (1, TU1);
		break;

	case IRQ_T04:

		TRA (2, TU0);
		break;

	case IRQ_TU1:
		
		if (zzv_curbit == 16) {
			// Done with the word
			if (++zzx_buffp == zzx_buffl) {
				// Done with the packet, continue sending zero
				goto EXM;
			}
			zzv_curbit = 0;
		}
				
		switch ((*zzx_buffp >> zzv_curbit) & 0x3) {

			case 0:		// 00
				TRA (1, T05);
				break;
			case 1:		// 10	(they go from the right!)
				TRA (2, T08);
				break;
			case 2: 	// 01
				TRA (1, T06);
				break;
			default:	// 11
				TRA (3, TU0);
		}
		zzv_curbit += 2;
		break;

	case IRQ_T05:

		TRA (2, TU1);
		break;

	case IRQ_T06:

		TRA (1, T07);
		break;

	case IRQ_T07:

		TRA (1, TU0);
		break;

	case IRQ_T08:

		TRA (1, TU1);
		break;

	case IRQ_EXM:
EXM:
		RISE_N_SHINE;
		zzv_istate = IRQ_OFF;
		disable_xcv_timer;
		zzv_status = 0;
		i_trigger (ETYPE_USER, txevent);
		LEDI (3, 0);
		break;

	case IRQ_RPR:

	    // Start fishing for preamble
	    {
		register word st, sv;

		get_signal_params (st, sv);

		if (st <= SH0 || st > SH2) {
			// Improper duration (too short or too long); reset
			zzv_prmble = 0;
			// This resets the capture timer to expect low-high,
			// and also clears reception timeout
			break;
		}

		if (st <= SH1) {
			// The right duration for a preamble bit
			zzv_prmble <<= 1;
			if (sv == 0)
				// Zero means 1 ;-)
				zzv_prmble |= 1;
			// We stay in the same state
			break;
		}

		// This may look like the end of preamble; we assume 
		// this is the case when we have collected at least 8
		// bits looking like a preamble; the long signal we see
		// now must be a 1, so the preamble should look like
		// an alternating pattern of zeros and ones ending
		// with a zero
		if (zzv_prmble != 0xAA) {
			// Sorry
			zzv_prmble = 0;
			break;
		}

		// Preamble OK, the present bit is necessarily 1
		*zzr_buffp = 0;
		LEDI (2, 1);
		// Initial state for packet reception
		zzv_istate = IRQ_RC0;
		// Collect RSSI
		adc_enable;
		// Abort on a timeout
		set_rcv_timer;
		enable_rcv_timeout;
	    }

	    break;

	/*
	 * This is the configuration of states for reception
	 *
	 * RECEIVING x (0/1) at a triplet boundary: (RC0)
	 *
         *   1:     SKIP       ->  (01)    RCV 2b ~x
	 *   2:     RCV x      ->  (02)    RCV 1b ~x
	 *   3:     RCV xx     ->  (RC0)   Back to triplet boundary
	 *   4:     Complete
	 *
	 * 01 (Receiving two bits at level x)
 	 *
	 *   1:     RCV x      ->  (03)    RCV 1b ~x 
	 *   2:     RCV xx     ->  (RC0)   Back to triplet boundary
	 *   3:     Complete
	 *
	 * 02 (Receiving one bit at level x)
	 *
	 *   1:     RCV x      ->  (RC0)
	 *   2:     Complete
	 *
	 */
	case IRQ_RC0:

#define	b_add(b)	do { \
				if (zzv_curbit == 16) { \
					if (++zzr_buffp == zzr_buffl) \
						goto REND; \
					zzv_curbit = 0; \
					*zzr_buffp = 0; \
				} \
				if ((b) == 0) \
					*zzr_buffp |= (1 << zzv_curbit); \
				zzv_curbit++; \
			} while (0)

	    {
		register word st, sv;

		get_signal_params (st, sv);
		set_rcv_timer;

		if (st <= SH1) {
			zzv_istate = IRQ_R01;
			break;
		}

		if (st <= SH2) {
			b_add (sv);
			zzv_istate = IRQ_R02;
			break;
		}

		if (st <= SH3) {
			b_add (sv);
			b_add (sv);
			zzv_istate = IRQ_RC0;
			break;
		}
	    }
REND:
		disable_xcv_timer;
		// Make sure the last word is properly counted
		++zzr_buffp;
		RISE_N_SHINE;
		adc_disable;
		if (zzv_rdbk->rssif)
			// Collecting RSSI
			zzv_rdbk->rssi = adc_value;
		zzv_status = 0;
		zzr_length = zzr_buffp - zzr_buffer;
		zzr_buffp = NULL;
		zzv_istate = IRQ_OFF;
		i_trigger (ETYPE_USER, rxevent);
		gbackoff;
		LEDI (2, 0);

		break;

	case IRQ_R01:

	    {
		register word st, sv;

		get_signal_params (st, sv);
		set_rcv_timer;

		if (st <= SH1) {
			b_add (sv);
			zzv_istate = IRQ_R02;
			break;
		}

		if (st <= SH2) {
			b_add (sv); b_add (sv);
			zzv_istate = IRQ_RC0;
			break;
		}

		goto REND;
	    }

	case IRQ_R02:

	    {
		register word st, sv;

		get_signal_params (st, sv);
		set_rcv_timer;

		if (st <= SH1) {
			b_add (sv);
			zzv_istate = IRQ_RC0;
			break;
		}

		goto REND;
	    }
    }
#undef	b_add

#endif
