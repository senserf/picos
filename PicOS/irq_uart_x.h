/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#if UART_TCV

    // The non-persistent variant

    switch (UA->x_istate) {

	case IRQ_X_OFF:

		// Stop the automaton
		UART_STOP_XMITTER;
		RTNI;

	case IRQ_X_STRT:

	    	// Transmitting the "preamble"
		XBUF = 0x55;
		UA->x_istate = IRQ_X_LEN;
		RTNI;

	case IRQ_X_LEN:

		// Transmitting the length
		XBUF = (UA->x_buffl - 4);
		UA->x_buffp = 0;
		UA->x_istate = IRQ_X_PKT;
		RTNI;

	case IRQ_X_PKT:

		XBUF = ((byte*)(UA->x_buffer)) [UA->x_buffp++];

		if (UA->x_buffp == UA->x_buffl) {
			// All done
			RISE_N_SHINE;
			if (UA->x_prcs != 0)
				p_trigger (UA->x_prcs, ETYPE_USER, TXEVENT);
			UART_STOP_XMITTER;
		}

		RTNI;
    }

#else

    // The non-persistent variant

    switch (UA->x_istate) {

	case IRQ_X_OFF:

		// Stop the automaton
		UART_STOP_XMITTER;
		RTNI;

	case IRQ_X_STRT:

	    	// Transmitting header byte
		XBUF = (UA->v_flags & 0x3);
		UA->x_istate = IRQ_X_LEN;
		RTNI;

	case IRQ_X_LEN:

		// Transmitting the length
		XBUF = (UA->x_buffl);
		UA->x_istate = IRQ_X_PKT;
		RTNI;

	case IRQ_X_PKT:

		// Transmitting the message
		if ((UA->x_buffp < UA->x_buffl) || ((UA->x_buffp & 1))) {
			XBUF = ((byte*)(UA->x_buffer)) [UA->x_buffp++];
			RTNI;
		}

		XBUF = UA->x_chk0;
		UA->x_istate = IRQ_X_CH1;
		RTNI;

	case IRQ_X_CH1:

		XBUF = UA->x_chk1;
		UA->x_istate = IRQ_X_STOP;
		RTNI;

	case IRQ_X_STOP:

		RISE_N_SHINE;
		if (UA->x_prcs != 0)
			p_trigger (UA->x_prcs, ETYPE_USER, TXEVENT);
		UART_STOP_XMITTER;
		RTNI;
    }

#endif
