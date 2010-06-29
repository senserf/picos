/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

    byte b;

#if UART_TCV_MODE == UART_TCV_MODE_N
// ============================================================================
// The non-persistent packet variant ==========================================
// ============================================================================

    switch (UA->r_istate) {

	case IRQ_R_OFF:

		// Absorb bytes and ignore them
		b = RBUF;
		RTNI;

	case IRQ_R_STRT:

	    	// Receiving header byte
		b = RBUF;
		if (b != 0x55)
			// Waiting for the "preamble"
			RTNI;
		// Trigger 'reception started' event (for timeouts)
		RISE_N_SHINE;
		if (UA->r_prcs != 0)
			p_trigger (UA->r_prcs, ETYPE_USER, RSEVENT);
		UA->r_istate = IRQ_R_LEN;
		RTNI;

	case IRQ_R_LEN:

		b = RBUF;
		if ((b & 1) != 0 || b > UA->r_buffl - 4) {
			// Length field error, ignore bytes until we are
			// reset by the process
			UA->r_buffer [0] = 0xffff;
			// Wake up the process to reset things
			RISE_N_SHINE;
			if (UA->r_prcs != 0)
				p_trigger (UA->r_prcs, ETYPE_USER, RXEVENT);
			UA->r_istate = IRQ_R_OFF;
			RTNI;
		}
		// The number of expected bytes
		UA->r_buffs = b + 4;
		UA->r_buffp = 0;
		UA->r_istate = IRQ_R_PKT;
		RTNI;

	case IRQ_R_PKT:

		((byte*)(UA->r_buffer)) [UA->r_buffp++] = RBUF;
		if (UA->r_buffp == UA->r_buffs) {
			// All done
			RISE_N_SHINE;
			if (UA->r_prcs != 0)
				p_trigger (UA->r_prcs, ETYPE_USER, RXEVENT);
			UA->r_istate = IRQ_R_OFF;
		}
		RTNI;
    }	

#endif

#if UART_TCV_MODE == UART_TCV_MODE_P
// ============================================================================
// The persistent packet variant ==============================================
// ============================================================================

    switch (UA->r_istate) {

	case IRQ_R_OFF:

		// Absorb bytes and ignore them
		b = RBUF;
		RTNI;

	case IRQ_R_STRT:

	    	// Receiving header byte
		b = RBUF;
		if ((b & 0xfc) != 0)
			// Waiting for something that looks like a header
			// byte
			RTNI;
		// Trigger 'reception started' event
		RISE_N_SHINE;
		if (UA->r_prcs != 0)
			p_trigger (UA->r_prcs, ETYPE_USER, RSEVENT);
		((byte*)(UA->r_buffer)) [0] = b;
		UA->r_istate = IRQ_R_LEN;
		RTNI;

	case IRQ_R_LEN:

		b = RBUF;
		if (b > UA->r_buffl - 4) {
			// Length field error, ignore bytes until we are
			// reset by the process
			UA->r_buffer [0] = 0xffff;
			// Wake up the process to reset things
			RISE_N_SHINE;
			if (UA->r_prcs != 0)
				p_trigger (UA->r_prcs, ETYPE_USER, RXEVENT);
			UA->r_istate = IRQ_R_OFF;
			RTNI;
		}
		UA->r_buffs = b;
		UA->r_buffp = 0;
		((byte*)(UA->r_buffer)) [1] = b;
		UA->r_istate = IRQ_R_PKT;
		RTNI;

	case IRQ_R_PKT:

		((byte*)(UA->r_buffer + 1)) [UA->r_buffp++] = RBUF;
		if (UA->r_buffp > UA->r_buffs && (UA->r_buffp & 1))
			// Read the first byte of checksum
			UA->r_istate = IRQ_R_CHK1;
		RTNI;

	case IRQ_R_CHK1:

		// Second byte of checksum
		((byte*)(UA->r_buffer + 1)) [UA->r_buffp++] = RBUF;
		// Complete packet
		RISE_N_SHINE;
		if (UA->r_prcs != 0)
			p_trigger (UA->r_prcs, ETYPE_USER, RXEVENT);
		UA->r_istate = IRQ_R_OFF;
		RTNI;
    }	

#endif

#if UART_TCV_MODE == UART_TCV_MODE_L
// ============================================================================
// The line variant ===========================================================
// ============================================================================

    b = RBUF;

    switch (UA->r_istate) {

	case IRQ_R_STRT:

		// Receive the first byte: ignore anything less than space
		if (b < 0x20)
			RTNI;

		// Network ID field not used, i.e., usable for payload
		UA->r_buffp = 1;
		*((byte*)(UA->r_buffer)) = b;
		UA->r_istate = IRQ_R_LIN;
		LEDIU (2, 1);
		RTNI;

	case IRQ_R_LIN:

		if (b == '\r' || b == '\n') {
			// Either one terminates the line, the other one
			// will be discarded at the beginning of next one
			RISE_N_SHINE;
			if (UA->r_prcs != 0)
				p_trigger (UA->r_prcs, ETYPE_USER, RXEVENT);
			UA->r_istate = IRQ_R_OFF;
		} else {
			if (UA->r_buffp < UA->r_buffl)
				// This will truncate longer lines
				((byte*)(UA->r_buffer)) [UA->r_buffp++] = b;
		}

	default:

		RTNI;

	
    }	

#endif
