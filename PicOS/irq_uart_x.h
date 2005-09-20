/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

    switch (UA->x_istate) {

	case IRQ_X_OFF:

		// Stop the automaton
		UART_STOP_XMITTER;
		return;

	case IRQ_X_STRT:

	    	// Transmitting header byte
		XBUF = (UA->v_flags & 0x3);
		UA->x_istate = IRQ_X_LEN;
		return;

	case IRQ_X_LEN:

		// Transmitting the length
		XBUF = (UA->x_buffl);
		UA->x_istate = IRQ_X_PKT;
		return;

	case IRQ_X_PKT:

		// Transmitting the message
		if ((UA->x_buffp < UA->x_buffl) || ((UA->x_buffp & 1))) {
			XBUF = ((byte*)(UA->x_buffer)) [UA->x_buffp++];
			return;
		}

		XBUF = UA->x_chk0;
		UA->x_istate = IRQ_X_CH1;
		return;

	case IRQ_X_CH1:

		XBUF = UA->x_chk1;
		UA->x_istate = IRQ_X_STOP;
		return;

	case IRQ_X_STOP:

		RISE_N_SHINE;
		if (UA->x_prcs != 0)
			p_trigger (UA->x_prcs, ETYPE_USER, TXEVENT);
		UART_STOP_XMITTER;
		return;
    }
