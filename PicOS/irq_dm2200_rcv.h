#ifndef __pg_irq_dm2200_rcv_h
#define __pg_irq_dm2200_rcv_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "dm2200.h"

/*
 * This code handles reception. Transmission is strobed by the CC timer.
 */

  if (rcv_interrupt) {

    rcv_clrint;

    switch (__pi_v_istate) {

	case IRQ_OFF:

		// Stop the automaton
		rcv_disable;
		__pi_v_status = 0;
		break;

	case IRQ_RPR:

#if COLLECT_RXDATA_ON_LOW
		rcv_edgehl;
		rcv_clrint;
#endif
		__pi_v_istate = IRQ_RP0;
		adc_start_refon;
		LEDI (2, 1);
#if FCC_TEST_MODE
		mdelay (10);
#endif
		__pi_v_cursym = 0;
		__pi_v_curbit = 0;

	case IRQ_RP0:

	    {
		register word sc;

		__pi_v_cursym <<= 1;
		if (rcv_sig_high)
			__pi_v_cursym |= 1;
		if (__pi_v_curbit == 5) {
			// We have a complete nibble
			sc = __pi_v_symtable [__pi_v_cursym];
			if (sc > 15) {
#if 0
			// Reason for failure
			diag ("SYM 0: %x %d %x %x", __pi_v_cursym,
				__pi_r_buffp - __pi_r_buffer, *(__pi_r_buffp-2),
					*(__pi_r_buffp-1));
#endif
REND:
				rcv_disable;
				__pi_v_status = 0;
				RISE_N_SHINE;
				adc_stop;
				__pi_r_length = __pi_r_buffp - __pi_r_buffer;
				__pi_r_buffp = NULL;
#if 0
				// Make nibble number available for debugging
				__pi_v_curbit = __pi_v_istate;
#endif
				__pi_v_istate = IRQ_OFF;
				i_trigger (rxevent);
				gbackoff;
				LEDI (2, 0);
			} else {
				__pi_v_cursym = 0;
				__pi_v_curbit = 0;
				*__pi_r_buffp = (sc << 12);
				__pi_v_istate = IRQ_RP1;
			}
		} else {
			__pi_v_curbit++;
		}
	    }

            break;

	case IRQ_RP1:

	    {
		register word sc;

		__pi_v_cursym <<= 1;
		if (rcv_sig_high)
			__pi_v_cursym |= 1;
		if (__pi_v_curbit == 5) {
			// Second complete nibble
			sc = __pi_v_symtable [__pi_v_cursym];
			if (sc > 15) {
#if 0
				diag ("SYM 1: %x %d %x %x", __pi_v_cursym,
					__pi_r_buffp - __pi_r_buffer,
						*(__pi_r_buffp-1),
							*(__pi_r_buffp));
#endif
				// Illegal
				goto REND;
			}

			__pi_v_cursym = 0;
			__pi_v_curbit = 0;
			*__pi_r_buffp |= (sc << 8);
			__pi_v_istate = IRQ_RP2;
		} else {
			__pi_v_curbit++;
		}
	    }

	    break;

	case IRQ_RP2:

	    {
		register word sc;

		__pi_v_cursym <<= 1;
		if (rcv_sig_high)
			__pi_v_cursym |= 1;
		if (__pi_v_curbit == 5) {
			// Third complete nibble
			sc = __pi_v_symtable [__pi_v_cursym];
			if (sc > 15) {
#if 0
				diag ("SYM 2: %x %d %x %x", __pi_v_cursym,
					__pi_r_buffp - __pi_r_buffer,
						*(__pi_r_buffp-1),
							*(__pi_r_buffp));
#endif
				// Illegal
				goto REND;
			}

			__pi_v_cursym = 0;
			__pi_v_curbit = 0;
			*__pi_r_buffp |= (sc << 4);
			__pi_v_istate = IRQ_RP3;
		} else {
			__pi_v_curbit++;
		}
	    }

	    break;

	case IRQ_RP3:

	    {
		register word sc;

		__pi_v_cursym <<= 1;
		if (rcv_sig_high)
			__pi_v_cursym |= 1;
		if (__pi_v_curbit == 5) {
			// Last nibble
			sc = __pi_v_symtable [__pi_v_cursym];
			if (sc > 15) {
#if 0
				diag ("SYM 3: %x %d %x %x", __pi_v_cursym,
					__pi_r_buffp - __pi_r_buffer,
						*(__pi_r_buffp-1),
							*(__pi_r_buffp));
#endif
				// Illegal
				goto REND;
			}
			*__pi_r_buffp |= sc;

			if (++__pi_r_buffp == __pi_r_buffl)
				goto REND;

			__pi_v_cursym = 0;
			__pi_v_curbit = 0;
			__pi_v_istate = IRQ_RP0;
		} else {
			__pi_v_curbit++;
		}
	    }
    }
  }

#endif
