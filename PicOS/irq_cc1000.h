#ifndef __pg_irq_cc1000_h
#define __pg_irq_cc1000_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

if (cc1000_int) {

    clear_cc1000_int;

    switch (__pi_v_istate) {

	case IRQ_OFF:

		// Disable
		clr_xcv_int;
		__pi_v_status = 0;
		break;

	case IRQ_XPR:

		// Transmitting preamble
		if ((__pi_v_prmble & 1))
			chp_clrdbit;
		else
			chp_setdbit;

		if (__pi_v_prmble)
			// More
			__pi_v_prmble--;
		else 
			__pi_v_istate = IRQ_XPT;
		// The last bit was one; we need one more
		set_xmt_int;
		break;

	case IRQ_XPT:

		// Preamble trailer
		chp_setdbit;
		__pi_v_istate = IRQ_XMP;
		set_xmt_int;
		break;

	case IRQ_XMP:

		// Transmit a packet bit
		if (((*__pi_x_buffp >> __pi_v_curbit) & 1))
			chp_setdbit;
		else
			chp_clrdbit;

		if (__pi_v_curbit == 15)
			__pi_v_istate = IRQ_XEW;
		else
			__pi_v_curbit++;
		set_xmt_int;
		break;

	case IRQ_XEW:

		// End of word: one extra bit
		if (++__pi_x_buffp == __pi_x_buffl) {
			// Done with the packet
			chp_clrdbit;
			__pi_v_istate = IRQ_XMT;
		} else {
			// More
			chp_setdbit;
			__pi_v_curbit = 0;
			__pi_v_istate = IRQ_XMP;
		}
		set_xmt_int;
		break;

	case IRQ_XMT:

		// Packet trailer
		chp_setdbit;
		__pi_v_istate = IRQ_XTE;
		set_xmt_int;
		break;

	case IRQ_XTE:

		// Final bit of trailer
		chp_clrdbit;
		// Complete transmission
		RISE_N_SHINE;
		__pi_v_istate = IRQ_OFF;
		__pi_v_status = 0;
		clr_xcv_int;
		i_trigger (ETYPE_USER, txevent);
		LEDI (1, 0);
		break;

	case IRQ_RPR:

		// Receiving preamble
		if (chp_getdbit) {
			__pi_v_prmble = (__pi_v_prmble << 1) | 1;
		} else {
			__pi_v_prmble <<= 1;
		}

		if (__pi_v_prmble == RCV_PREAMBLE) {
			// Initialize the first word of packet
			*__pi_r_buffp = 0;
			LEDI (2, 1);
			__pi_v_istate = IRQ_RCV;
			adc_start_refon;
		}
		break;

	case IRQ_RCV:

		// Receive a packet bit
		if (chp_getdbit)
			*__pi_r_buffp |= (1 << __pi_v_curbit);
		if (__pi_v_curbit == 15)
			// End of word
			__pi_v_istate = IRQ_RTR;
		else
			__pi_v_curbit++;
		break;

	case IRQ_RTR:

		// Receive the word trailer bit
		__pi_r_buffp++;
		if (chp_getdbit && __pi_r_buffp != __pi_r_buffl) {
			// There is more
			__pi_v_curbit = 0;
			*__pi_r_buffp = 0;
			__pi_v_istate = IRQ_RCV;
			// In case anything is needed to keep the ADC ticking
			adc_advance;
		} else {
			// End of packet
			RISE_N_SHINE;
			adc_stop;
			adc_wait;
			adc_disable;
				// Collect RSSI
			__pi_v_status = 0;
			clr_xcv_int;
			__pi_r_length = __pi_r_buffp - __pi_r_buffer;
			__pi_r_buffp = NULL;
			__pi_v_istate = IRQ_OFF;
			i_trigger (ETYPE_USER, rxevent);
			gbackoff;
			LEDI (2, 0);
		}
		break;
    }
}

#endif
