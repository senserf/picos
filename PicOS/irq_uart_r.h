/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

    byte b;

    switch (UA->r_istate) {

	case IRQ_R_OFF:

		// Absorb bytes and ignore them
		b = RBUF;
		return;

	case IRQ_R_STRT:

	    	// Receiving header byte
		b = RBUF;
		if ((b & 0xfc) != 0)
			// Waiting for something that looks like a header
			// byte
			return;
		// Trigger 'reception started' event
		RISE_N_SHINE;
		if (UA->r_prcs != 0)
			p_trigger (UA->r_prcs, ETYPE_USER, RSEVENT);
		((byte*)(UA->r_buffer)) [0] = b;
		UA->r_istate = IRQ_R_LEN;
		return;

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
			return;
		}
		UA->r_buffs = b;
		UA->r_buffp = 0;
		((byte*)(UA->r_buffer)) [1] = b;
		UA->r_istate = IRQ_R_PKT;
		return;

	case IRQ_R_PKT:

		((byte*)(UA->r_buffer + 1)) [UA->r_buffp++] = RBUF;
		if (UA->r_buffp > UA->r_buffs && (UA->r_buffp & 1))
			// Read the first byte of checksum
			UA->r_istate = IRQ_R_CHK1;
		return;

	case IRQ_R_CHK1:

		// Second byte of checksum
		((byte*)(UA->r_buffer + 1)) [UA->r_buffp++] = RBUF;
		// Complete packet
		RISE_N_SHINE;
		if (UA->r_prcs != 0)
			p_trigger (UA->r_prcs, ETYPE_USER, RXEVENT);
		UA->r_istate = IRQ_R_OFF;
		return;
    }	
