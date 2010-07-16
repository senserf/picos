/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#if UART_TCV_MODE == UART_TCV_MODE_N
// ============================================================================
// The non-persistent packet variant ==========================================
// ============================================================================

    switch (UA->x_istate) {

	case IRQ_X_OFF:

		// Stop the automaton
		UART_STOP_XMITTER;
		RTNI;

	case IRQ_X_STRT:

	    	// Transmitting the "preamble"
		XBUF_STORE (0x55);
		UA->x_istate = IRQ_X_LEN;
		RTNI;

	case IRQ_X_LEN:

		// Transmitting the length
		XBUF_STORE (UA->x_buffl - 4);
		UA->x_buffp = 0;
		UA->x_istate = IRQ_X_PKT;
		RTNI;

	case IRQ_X_PKT:

		XBUF_STORE (((byte*)(UA->x_buffer)) [UA->x_buffp++]);

		if (UA->x_buffp == UA->x_buffl)
			// Last byte - wait for the last interrupt, which will
			// indicate that the UART is in fact done
			UA->x_istate = IRQ_X_STOP;
		RTNI;

	case IRQ_X_STOP:

		// All done
		RISE_N_SHINE;
		if (UA->x_prcs != 0)
			p_trigger (UA->x_prcs, TXEVENT);
		UART_STOP_XMITTER;
		RTNI;
    }

#endif

#if UART_TCV_MODE == UART_TCV_MODE_P
// ============================================================================
// The persistent packet variant ==============================================
// ============================================================================

    switch (UA->x_istate) {

	case IRQ_X_OFF:

		// Stop the automaton
		UART_STOP_XMITTER;
		RTNI;

	case IRQ_X_STRT:

	    	// Transmitting header byte
		XBUF_STORE (UA->x_buffh);
		UA->x_istate = IRQ_X_LEN;
		RTNI;

	case IRQ_X_LEN:

		// Transmitting the length
		XBUF_STORE (UA->x_buffc);
		UA->x_istate = IRQ_X_PKT;
		RTNI;

	case IRQ_X_PKT:

		// Transmitting the message
		if (UA->x_buffp < UA->x_buffl) {
			XBUF_STORE (((byte*)(UA->x_buffer)) [UA->x_buffp++]);
			RTNI;
		}

		XBUF_STORE (UA->x_chk0);
		UA->x_istate = IRQ_X_CH1;
		RTNI;

	case IRQ_X_CH1:

		XBUF_STORE (UA->x_chk1);
		UA->x_istate = IRQ_X_STOP;
		RTNI;

	case IRQ_X_STOP:

		RISE_N_SHINE;
		if (UA->x_prcs != 0)
			p_trigger (UA->x_prcs, TXEVENT);
		UART_STOP_XMITTER;
		RTNI;
    }

#endif

#if UART_TCV_MODE == UART_TCV_MODE_L
// ============================================================================
// The line variant ===========================================================
// ============================================================================

    byte b;   

    switch (UA->x_istate) {

	case IRQ_X_OFF:

		// Stop the automaton
		UART_STOP_XMITTER;
		RTNI;

	case IRQ_X_STRT:

		// NetID field used for payload
		UA->x_buffp = 0;
		UA->x_istate = IRQ_X_LIN;

	case IRQ_X_LIN:

		if (UA->x_buffp == UA->x_buffl) {
			// Transmit CRLF
Eol:
			XBUF_STORE ('\r');
			UA->x_istate = IRQ_X_EOL;
		} else {
			b = ((byte*)(UA->x_buffer)) [UA->x_buffp++];
			if (b == '\0')
				// Null byte also terminates the string
				goto Eol;
			XBUF_STORE (b);
		}
		RTNI;

	case IRQ_X_EOL:

		XBUF_STORE ('\n');
		UA->x_istate = IRQ_X_STOP;
		RTNI;

	case IRQ_X_STOP:

		// All done
		RISE_N_SHINE;
		if (UA->x_prcs != 0)
			p_trigger (UA->x_prcs, TXEVENT);
		UART_STOP_XMITTER;
		RTNI;
    }

#endif
