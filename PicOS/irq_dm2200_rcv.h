#ifndef __pg_irq_dm2200_rcv_h
#define __pg_irq_dm2200_rcv_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/*
 * This code handles reception. Transmission is strobed by the CC timer.
 */

    rcv_clrint;

    switch (zzv_istate) {

	case IRQ_OFF:

		// Stop the automaton
		rcv_disable;
		zzv_status = 0;
		break;

	case IRQ_RPR:

		zzv_istate = IRQ_RP0;
		adc_start;
		LEDI (2, 1);
#if FCC_TEST_MODE
		mdelay (10);
#endif
		zzv_cursym = 0;
		zzv_curbit = 0;

	case IRQ_RP0:

	    {
		register word sc;

		zzv_cursym <<= 1;
		if (rcv_sig_high)
			zzv_cursym |= 1;
		if (zzv_curbit == 5) {
			// We have a complete nibble
			sc = zzv_symtable [zzv_cursym];
			if (sc > 15) {
REND:
				rcv_disable;
				zzv_status = 0;
				RISE_N_SHINE;
				adc_stop;
				zzr_length = zzr_buffp - zzr_buffer;
				zzr_buffp = NULL;
				zzv_istate = IRQ_OFF;
				i_trigger (ETYPE_USER, rxevent);
				gbackoff;
				LEDI (2, 0);
			} else {
				zzv_cursym = 0;
				zzv_curbit = 0;
				*zzr_buffp = (sc << 12);
				zzv_istate = IRQ_RP1;
			}
		} else {
			zzv_curbit++;
		}
	    }

            break;

	case IRQ_RP1:

	    {
		register word sc;

		zzv_cursym <<= 1;
		if (rcv_sig_high)
			zzv_cursym |= 1;
		if (zzv_curbit == 5) {
			// Second complete nibble
			sc = zzv_symtable [zzv_cursym];
			if (sc > 15) 
				// Illegal
				goto REND;

			zzv_cursym = 0;
			zzv_curbit = 0;
			*zzr_buffp |= (sc << 8);
			zzv_istate = IRQ_RP2;
		} else {
			zzv_curbit++;
		}
	    }

	    break;

	case IRQ_RP2:

	    {
		register word sc;

		zzv_cursym <<= 1;
		if (rcv_sig_high)
			zzv_cursym |= 1;
		if (zzv_curbit == 5) {
			// Third complete nibble
			sc = zzv_symtable [zzv_cursym];
			if (sc > 15) 
				// Illegal
				goto REND;

			zzv_cursym = 0;
			zzv_curbit = 0;
			*zzr_buffp |= (sc << 4);
			zzv_istate = IRQ_RP3;
		} else {
			zzv_curbit++;
		}
	    }

	    break;

	case IRQ_RP3:

	    {
		register word sc;

		zzv_cursym <<= 1;
		if (rcv_sig_high)
			zzv_cursym |= 1;
		if (zzv_curbit == 5) {
			// Last nibble
			sc = zzv_symtable [zzv_cursym];
			if (sc > 15) 
				// Illegal
				goto REND;
			*zzr_buffp |= sc;

			if (++zzr_buffp == zzr_buffl)
				goto REND;

			zzv_cursym = 0;
			zzv_curbit = 0;
			zzv_istate = IRQ_RP0;
		} else {
			zzv_curbit++;
		}
	    }
    }

#endif
