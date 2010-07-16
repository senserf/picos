#ifndef __pg_irq_dm2100_h
#define __pg_irq_dm2100_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/*
 * This code handles signal strobes. For transmission, the strobe is triggered
 * on every timed signal transition edge. For reception, the strobe is triggered
 * by the (expected) edge of the input signal.
 */

    switch (__pi_v_istate) {

	case IRQ_OFF:

		// Stop the automaton
		disable_xcv_timer;
		__pi_v_status = 0;
		RTNI;

	case IRQ_XPR:

		// Transmitting preamble low
		toggle_signal;
		SL1;
		if (--__pi_v_prmble) {
			__pi_v_istate = IRQ_XPQ;
		} else {
			// End of preamble
			__pi_v_curbit = 3;
			__pi_v_cursym = 0x20;
			__pi_v_istate = IRQ_XSV;
		}
		RTNI;

	case IRQ_XPQ:

		// Transmitting preamble high
		toggle_signal;
		SL1;
		__pi_v_istate = IRQ_XPR;
		RTNI;

	case IRQ_XSV:

	    // First symbol of the start vector

	    { 
		register word sl;

		toggle_signal;

		if (__pi_v_curbit == 0) {
			// The last segment; we know that the sequence is
			// 101110 001011 ... and we are at the beginning of
			// the second symbol. So we set sl (signal length)
			// explicitly to 3 (2+1) reflecting the three zeros
			// on the boundary.
			sl = 2;
			__pi_v_curbit = 2;		// 3 - 1
			__pi_v_cursym = 0x41 >> 2;	// Shift already done
			__pi_v_istate = IRQ_XSW;
		} else {
			sl = (__pi_v_cursym & 0x3);
			__pi_v_curbit--;
			__pi_v_cursym >>= 2;
		}
		TRA (sl);
	    }
	    RTNI;

	case IRQ_XSW:

	    // Second symbol of the start vector

	    { 
		register word sl;

		toggle_signal;

		if (__pi_v_curbit == 0) {
			// .... 001011 100010 ...
			//             ^
			sl = 2;
			__pi_v_curbit = 2;		// 3 - 1
			__pi_v_cursym = 0x08 >> 2;	// Shift already made
			__pi_v_istate = IRQ_XSX;
		} else {
			sl = (__pi_v_cursym & 0x3);
			__pi_v_curbit--;
			__pi_v_cursym >>= 2;
		}
		TRA (sl);
	    }
	    RTNI;

	case IRQ_XSX:

	    // Third (and last) symbol of the start vector

	    {
		register word sl, ni;

		toggle_signal;

		sl = (__pi_v_cursym & 0x3);
		if (__pi_v_curbit == 0) {
			// We have ended at level zero, and now begin to look
			// at the packet
			ni = (*__pi_x_buffp >> 12) & 0xf;
			__pi_v_curnib = 12;
			__pi_v_cursym = __pi_v_nibtable [ni];
			__pi_v_curbit = __pi_v_srntable [ni];
			if (ni < 8) {
				// Initial signal level is 0, this is also our 
				// present level
				sl += (__pi_v_cursym & 0x3) + 1;
				__pi_v_curbit--;
				__pi_v_cursym >>= 2;
			}
			__pi_v_istate = IRQ_XPK;
		} else {
			__pi_v_curbit--;
			__pi_v_cursym >>= 2;
		}
		TRA (sl);
	    }
	    RTNI;

	case IRQ_XPK:

	    {
		register word sl, ni;

		toggle_signal;

		sl = (__pi_v_cursym & 0x3);
		if (__pi_v_curbit == 0) {
			// Look at the next packet nibble
			if (__pi_v_curnib == 0) {
				// Need next word
				if (++__pi_x_buffp == __pi_x_buffl) {
					// We are done, send EOP
					__pi_v_istate = IRQ_XEP;
					if (current_signal_level == 0) {
						// Two more zeros
						sl += 2;
						__pi_v_cursym = 0x41 >> 2;
						__pi_v_curbit = 3;
					} else {
						__pi_v_cursym = 0x41;
						__pi_v_curbit = 4;
					}
				} else {
					ni = (*__pi_x_buffp >> 12) & 0xf;
					__pi_v_curnib = 12;
					__pi_v_cursym = __pi_v_nibtable [ni];
					__pi_v_curbit = __pi_v_srntable [ni];
					if ((current_signal_level != 0) ==
					    (ni >= 8)) {
						// Identical signal
						sl += (__pi_v_cursym & 0x3) + 1;
						__pi_v_curbit--;
						__pi_v_cursym >>= 2;
					}
				}
			} else {
				__pi_v_curnib -= 4;
				ni = (*__pi_x_buffp >> __pi_v_curnib) & 0xf;
				__pi_v_cursym = __pi_v_nibtable [ni];
				__pi_v_curbit = __pi_v_srntable [ni];
				if ((current_signal_level != 0) == (ni >= 8)) {
					sl += (__pi_v_cursym & 0x3) + 1;
					__pi_v_curbit--;
					__pi_v_cursym >>= 2;
				}
			}
		} else {
			__pi_v_curbit--;
			__pi_v_cursym >>= 2;
		}
		TRA (sl);
	    }

	    RTNI;

	case IRQ_XEP:

	    // Send EOP symbol
	    { 
		register word sl;

		toggle_signal;

		if (__pi_v_curbit == 0) {
			// We are done: the signal is now set set low
			TRA (2); TRA (2);
			__pi_v_istate = IRQ_EXM;
			RTNI;
		}
		sl = (__pi_v_cursym & 0x3);
		__pi_v_curbit--;
		__pi_v_cursym >>= 2;
		TRA (sl);
	    }
	    RTNI;

	case IRQ_EXM:

		RISE_N_SHINE;
		__pi_v_istate = IRQ_OFF;
		disable_xcv_timer;
		__pi_v_status = 0;
		i_trigger (txevent);
		LEDI (1, 0);
		break;

	case IRQ_RPR:

	    // Looking for preamble
	    {
		register word st, sv;

		get_signal_params (st, sv);
		add_entropy (st);

		if (st <= SH1) {
			// A preamble bit
			__pi_v_prmble <<= 1;
			if (sv == 0)
				// The polarity is reversed: 0 means 1
				__pi_v_prmble |= 1;
			// We stay in the same state
			RTNI;
		}

		if (st > SH2 && st <= SH3 && __pi_v_prmble == 0xAA) {
			// End of preamble: the three ones begin the
			// start vector, which looks like
			// xx1110 001011 100010 (three symbols, xx
			// stands for the last two bits of the preamble
			__pi_v_curbit = 3;
			*__pi_r_buffp = 0x7;
			__pi_v_istate = IRQ_RSV;
			set_rcv_timeout;
			enable_rcv_timeout;
			RTNI;
		}

		// This is illegal, reset the preamble
		__pi_v_prmble = 0;
	    }

	    RTNI;

	case IRQ_RSV:

	    // Receiving the start vector
	    {
		register word st, sv;

		get_signal_params (st, sv);
		add_entropy (st);
		set_rcv_timeout;

		if (st <= SH2) {
			if (st <= SH1) {
				st = 1;
			} else {
				st = 2;
			}
		} else {
			if (st <= SH3) {
				st = 3;
			} else if (st <= SH4) {
				st = 4;
			} else {
				disable_rcv_timeout;
				__pi_v_prmble = 0;
				__pi_v_istate = IRQ_RPR;
				RTNI;
			}
		}

		while (st) {
			*__pi_r_buffp <<= 1;
			if (sv == 0)
				*__pi_r_buffp |= 1;
			st--;
			if (++__pi_v_curbit == 16) {
				// This should be a valid start vector
				if (*__pi_r_buffp != 0xE2E2) {
					disable_rcv_timeout;
					__pi_v_prmble = 0;
					__pi_v_istate = IRQ_RPR;
					RTNI;
				}
				// Begin collecting RSSI after the start vector
				adc_start_refon;
				LEDI (2, 1);
				// Start the packet
				*__pi_r_buffp = 0;
				__pi_v_curbit = st;
				__pi_v_cursym = 0;
				// Left shift count
				__pi_v_curnib = 12;
				if (sv == 0) {
					while (st--)
						__pi_v_cursym =
							(__pi_v_cursym << 1) |
								1;
				}
				__pi_v_istate = IRQ_RPK;
				RTNI;
			}
		}
	    }

	    RTNI;

	case IRQ_RPK:

	    // Receiving packet
	    {
		register word st, sv, sc;

		get_signal_params (st, sv);
		add_entropy (st);
		set_rcv_timeout;

		if (st <= SH2) {
			if (st <= SH1) {
				st = 1;
			} else {
				st = 2;
			}
		} else {
			if (st > SH4) {
				// Forced end (same as timeout)
				__pi_v_curbit = 129;
				__pi_v_tmaux = st;
				goto REND;
			}
			if (st <= SH3) {
				st = 3;
			} else {
				st = 4;
			}
		}

		while (st) {
			__pi_v_cursym <<= 1;
			if (sv == 0)
				__pi_v_cursym |= 1;
			st--;
			if (++__pi_v_curbit == 6) {
				// End of symbol
				sc = __pi_v_symtable [__pi_v_cursym];
				if (sc > 15) {
					// Illegal
					__pi_v_curbit = 130;
					__pi_v_tmaux = __pi_v_cursym;
					goto REND;
				}
				__pi_v_curbit = 0;
				__pi_v_cursym = 0;
				*__pi_r_buffp |= (sc << __pi_v_curnib);
				if (__pi_v_curnib == 0) {
					if (++__pi_r_buffp == __pi_r_buffl)
						goto REND;
					__pi_v_curnib = 12;
					*__pi_r_buffp = 0;
				} else {
					__pi_v_curnib -= 4;
				}
			}
		}

		RTNI;

REND:
		// Disable all interrupts
		disable_xcv_timer;
		// If there is any pending nibble, ignore it; at the worst,
		// it can be the EOP, which we don't care about
		if (__pi_v_curnib != 12)
			// We have a partial word, round it up
			++__pi_r_buffp;
		RISE_N_SHINE;
		adc_stop;
		__pi_v_status = 0;
		__pi_r_length = __pi_r_buffp - __pi_r_buffer;
		__pi_r_buffp = NULL;
		__pi_v_istate = IRQ_OFF;
		i_trigger (rxevent);
		gbackoff;
		LEDI (2, 0);
	    }

	    RTNI;
    }

#endif
