#ifndef __pg_irq_dm2200_xmt_h
#define __pg_irq_dm2200_xmt_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/*
 * This code handles signal strobes for transmission. Reception is driven by
 * the data clock recovered from the signal.
 *
 */
    switch (zzv_istate) {

	case IRQ_OFF:

		// Stop the automaton
		disable_xmt_timer;
		zzv_status = 0;
		RTNI;

	case IRQ_XPQ:

		// Transmitting preamble low
		toggle_signal;
		SL1;
		if (--zzv_prmble) {
			zzv_istate = IRQ_XPR;
		} else {
			// End of preamble
			zzv_curbit = 3;
			zzv_cursym = 0x20;
			zzv_istate = IRQ_XSV;
		}
		RTNI;

	case IRQ_XPR:

		// Transmitting preamble high
		toggle_signal;
		SL1;
		zzv_istate = IRQ_XPQ;
		RTNI;

	case IRQ_XSV:

	    // First symbol of the start vector

	    { 
		register word sl;

		toggle_signal;

		if (zzv_curbit == 0) {
			// The last segment; we know that the sequence is
			// 101110 001011 ... and we are at the beginning of
			// the second symbol. So we set sl (signal length)
			// explicitly to 3 (2+1) reflecting the three zeros
			// on the boundary.
			sl = 2;
			zzv_curbit = 2;			// 3 - 1
			zzv_cursym = 0x41 >> 2;		// Shift already done
			zzv_istate = IRQ_XSW;
		} else {
			sl = (zzv_cursym & 0x3);
			zzv_curbit--;
			zzv_cursym >>= 2;
		}
		TRA (sl);
	    }
	    RTNI;

	case IRQ_XSW:

	    // Second symbol of the start vector

	    { 
		register word sl;

		toggle_signal;

		if (zzv_curbit == 0) {
			// .... 001011 100010 ...
			//             ^
			sl = 2;
			zzv_curbit = 2;			// 3 - 1
			zzv_cursym = 0x08 >> 2;		// Shift already made
			zzv_istate = IRQ_XSX;
		} else {
			sl = (zzv_cursym & 0x3);
			zzv_curbit--;
			zzv_cursym >>= 2;
		}
		TRA (sl);
	    }
	    RTNI;

	case IRQ_XSX:

	    // Third (and last) symbol of the start vector

	    {
		register word sl, ni;

		toggle_signal;

		sl = (zzv_cursym & 0x3);
		if (zzv_curbit == 0) {
			// We have ended at level zero, and now begin to look
			// at the packet
			ni = (*zzx_buffp >> 12) & 0xf;
			zzv_curnib = 12;
			zzv_cursym = zzv_nibtable [ni];
			zzv_curbit = zzv_srntable [ni];
			if (ni < 8) {
				// Initial signal level is 0, this is also our 
				// present level
				sl += (zzv_cursym & 0x3) + 1;
				zzv_curbit--;
				zzv_cursym >>= 2;
			}
			zzv_istate = IRQ_XPK;
		} else {
			zzv_curbit--;
			zzv_cursym >>= 2;
		}
		TRA (sl);
	    }
	    RTNI;

	case IRQ_XPK:

#if FCC_TEST_MODE
		zzx_buffp = 0;
		zzv_istate = IRQ_XEP;

	case IRQ_XEP:
		// Send a continuous alternating sequence of 0 and 1
		toggle_signal;
		SL1;

		if (++((word)zzx_buffp) >= 1024) {
			zzv_istate = IRQ_XPR;
		}
		break;
#else
	    {
		register word sl, ni;

		toggle_signal;

		sl = (zzv_cursym & 0x3);
		if (zzv_curbit == 0) {
			// Look at the next packet nibble
			if (zzv_curnib == 0) {
				// Need next word
				if (++zzx_buffp == zzx_buffl) {
					// We are done, send EOP
					zzv_istate = IRQ_XEP;
					if (current_signal_level == 0) {
						// Two more zeros
						sl += 2;
						zzv_cursym = 0x41 >> 2;
						zzv_curbit = 3;
					} else {
						zzv_cursym = 0x41;
						zzv_curbit = 4;
					}
				} else {
					ni = (*zzx_buffp >> 12) & 0xf;
					zzv_curnib = 12;
					zzv_cursym = zzv_nibtable [ni];
					zzv_curbit = zzv_srntable [ni];
					if ((current_signal_level != 0) ==
					    (ni >= 8)) {
						// Identical signal
						sl += (zzv_cursym & 0x3) + 1;
						zzv_curbit--;
						zzv_cursym >>= 2;
					}
				}
			} else {
				zzv_curnib -= 4;
				ni = (*zzx_buffp >> zzv_curnib) & 0xf;
				zzv_cursym = zzv_nibtable [ni];
				zzv_curbit = zzv_srntable [ni];
				if ((current_signal_level != 0) == (ni >= 8)) {
					sl += (zzv_cursym & 0x3) + 1;
					zzv_curbit--;
					zzv_cursym >>= 2;
				}
			}
		} else {
			zzv_curbit--;
			zzv_cursym >>= 2;
		}
		TRA (sl);
	    }

	    RTNI;

	case IRQ_XEP:

	    // Send EOP symbol
	    { 
		register word sl;

		toggle_signal;

		if (zzv_curbit == 0) {
			// We are done: the signal is now set set low
			SLE;
			zzv_istate = IRQ_EXM;
			RTNI;
		}
		sl = (zzv_cursym & 0x3);
		zzv_curbit--;
		zzv_cursym >>= 2;
		TRA (sl);
	    }
	    RTNI;

	case IRQ_EXM:

		disable_xmt_timer;
		RISE_N_SHINE;
		zzv_istate = IRQ_OFF;
		zzv_status = 0;
		i_trigger (ETYPE_USER, txevent);
		LEDI (1, 0);
		break;

#endif	/* FCC_TEST_MODE */
    }

#endif
