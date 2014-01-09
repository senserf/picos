/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2014                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#if UART_TCV_MODE == UART_TCV_MODE_N
// ============================================================================
// The non-persistent packet variant ==========================================
// ============================================================================

    switch (UA->x_istate) {

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
		// Fall through

	case IRQ_X_OFF:

		UART_STOP_XMITTER;
		RTNI;
    }

#endif

#if UART_TCV_MODE == UART_TCV_MODE_P
// ============================================================================
// The persistent packet variant ==============================================
// ============================================================================

    switch (UA->x_istate) {

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

	case IRQ_X_OFF:

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

	case IRQ_X_OFF:

		UART_STOP_XMITTER;
		RTNI;
    }

#endif

#if UART_TCV_MODE == UART_TCV_MODE_E
// ============================================================================
// STX-ETX-DLE ================================================================
// ============================================================================

    byte b;

    switch (UA->x_istate) {

	case IRQ_X_STRT:

		// Sending STX, initializing checksum (r_buffs unused by the
		// receiver)
		UA->r_buffs = 0x02 ^ 0x03;
		UA->x_buffp = 0;
		XBUF_STORE (0x02);
		UA->x_istate = IRQ_X_PKT;
		RTNI;

	case IRQ_X_PKT:

		// Sending the payload
		if (UA->x_buffp == UA->x_buffl) {
			// No more payload, the parity byte
			if (UA->r_buffs != 0x02 && UA->r_buffs != 0x03 &&
			    UA->r_buffs != 0x10)
				goto SendPar;

			// Need to escape the parity byte
			XBUF_STORE (0x10);
			UA->x_istate = IRQ_X_PAE;
		} else {
			// A payload byte
			b = ((byte*)(UA->x_buffer)) [UA->x_buffp];
			if (b == 0x02 || b == 0x03 || b == 0x10) {
				// Escape
				XBUF_STORE (0x10);
				UA->x_istate = IRQ_X_ESC;
			} else {
SendByte:
				XBUF_STORE (b);
				UA->x_buffp++;
				UA->r_buffs ^= b;
			}
		}
		RTNI;

	case IRQ_X_ESC:

		// DLE escape within the payload
		b = ((byte*)(UA->x_buffer)) [UA->x_buffp];
		UA->x_istate = IRQ_X_PKT;
		goto SendByte;

	case IRQ_X_PAE:

		// Parity byte escaped, send the parity byte and quit
SendPar:
		XBUF_STORE (UA->r_buffs);
		UA->x_istate = IRQ_X_ETX;
		RTNI;

	case IRQ_X_ETX:

		// ETX
		XBUF_STORE (0x03);
		UA->x_istate = IRQ_X_STOP;
		RTNI;

	case IRQ_X_STOP:

		RISE_N_SHINE;
		if (UA->x_prcs != 0)
			p_trigger (UA->x_prcs, TXEVENT);

	case IRQ_X_OFF:

		UART_STOP_XMITTER;
}

#endif
