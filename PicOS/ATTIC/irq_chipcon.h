#ifndef __pg_irq_chipcon_h
#define __pg_irq_chipcon_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

if (chipcon_int) {

    clear_chipcon_int;

    switch (zzv_istate) {

	case IRQ_OFF:

		// Disable
		clr_xcv_int;
		zzv_status = 0;
		break;

	case IRQ_XPR:

		// Transmitting preamble
		if ((zzv_prmble & 1))
			chp_clrdbit;
		else
			chp_setdbit;

		if (zzv_prmble)
			// More
			zzv_prmble--;
		else 
			zzv_istate = IRQ_XPT;
		// The last bit was one; we need one more
		set_xmt_int;
		break;

	case IRQ_XPT:

		// Preamble trailer
		chp_setdbit;
		zzv_istate = IRQ_XMP;
		set_xmt_int;
		break;

	case IRQ_XMP:

		// Transmit a packet bit
		if (((*zzx_buffp >> zzv_curbit) & 1))
			chp_setdbit;
		else
			chp_clrdbit;

		if (zzv_curbit == 15)
			zzv_istate = IRQ_XEW;
		else
			zzv_curbit++;
		set_xmt_int;
		break;

	case IRQ_XEW:

		// End of word: one extra bit
		if (++zzx_buffp == zzx_buffl) {
			// Done with the packet
			chp_clrdbit;
			zzv_istate = IRQ_XMT;
		} else {
			// More
			chp_setdbit;
			zzv_curbit = 0;
			zzv_istate = IRQ_XMP;
		}
		set_xmt_int;
		break;

	case IRQ_XMT:

		// Packet trailer
		chp_setdbit;
		zzv_istate = IRQ_XTE;
		set_xmt_int;
		break;

	case IRQ_XTE:

		// Final bit of trailer
		chp_clrdbit;
		// Complete transmission
		RISE_N_SHINE;
		zzv_istate = IRQ_OFF;
		zzv_status = 0;
		clr_xcv_int;
		i_trigger (ETYPE_USER, txevent);
		LEDI (3, 0);
		break;

	case IRQ_RPR:

		// Receiving preamble
		if (chp_getdbit)
			zzv_prmble = (zzv_prmble << 1) | 1;
		else
			zzv_prmble <<= 1;
		if (zzv_prmble == RCV_PREAMBLE) {
			// Initialize the first word of packet
			*zzr_buffp = 0;
			LEDI (2, 1);
			zzv_istate = IRQ_RCV;
			adc_start;
		}
		break;

	case IRQ_RCV:

		// Receive a packet bit
		if (chp_getdbit)
			*zzr_buffp |= (1 << zzv_curbit);
		if (zzv_curbit == 15)
			// End of word
			zzv_istate = IRQ_RTR;
		else
			zzv_curbit++;
		break;

	case IRQ_RTR:

		// Receive the word trailer bit
		zzr_buffp++;
		if (chp_getdbit && zzr_buffp != zzr_buffl) {
			// There is more
			zzv_curbit = 0;
			*zzr_buffp = 0;
			zzv_istate = IRQ_RCV;
		} else {
			// End of packet
			RISE_N_SHINE;
			adc_stop;
			adc_disable;
				// Collect RSSI
			zzv_status = 0;
			clr_xcv_int;
			zzr_length = zzr_buffp - zzr_buffer;
			zzr_buffp = NULL;
			zzv_istate = IRQ_OFF;
			i_trigger (ETYPE_USER, rxevent);
			gbackoff;
			LEDI (2, 0);
		}
		break;
    }
}

#endif
