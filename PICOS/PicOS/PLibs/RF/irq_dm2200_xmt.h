/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_irq_dm2200_xmt_h
#define __pg_irq_dm2200_xmt_h

#include "dm2200.h"

/*
 * This code handles signal strobes for transmission. Reception is driven by
 * the data clock recovered from the signal.
 *
 */
    switch (__pi_v_istate) {

	case IRQ_OFF:

		// Stop the automaton
		disable_xmt_timer;
		__pi_v_status = 0;
		RTNI;

	case IRQ_XPQ:

		// Transmitting preamble low
		toggle_signal;
		SL1;
		if (--__pi_v_prmble) {
			__pi_v_istate = IRQ_XPR;
		} else {
			// End of preamble
			__pi_v_curbit = 3;
			__pi_v_cursym = 0x20;
			__pi_v_istate = IRQ_XSV;
		}
		RTNI;

	case IRQ_XPR:

		// Transmitting preamble high
		toggle_signal;
		SL1;
		__pi_v_istate = IRQ_XPQ;
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

#if FCC_TEST_MODE
		__pi_x_buffp = 0;
		__pi_v_istate = IRQ_XEP;

	case IRQ_XEP:
		// Send a continuous alternating sequence of 0 and 1
		toggle_signal;
		SL1;

		if (++((word)__pi_x_buffp) >= 1024) {
			__pi_v_istate = IRQ_XPR;
		}
		break;
#else
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
			SLE;
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

		disable_xmt_timer;
		RISE_N_SHINE;
		__pi_v_istate = IRQ_OFF;
		__pi_v_status = 0;
		i_trigger (txevent);
		LEDI (1, 0);
		break;

#endif	/* FCC_TEST_MODE */
    }

#endif
