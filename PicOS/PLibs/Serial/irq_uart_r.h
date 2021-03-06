/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

    byte b;

#if UART_TCV_MODE == UART_TCV_MODE_N
// ============================================================================
// The non-persistent packet variant ==========================================
// ============================================================================

    switch (UA->r_istate) {

	case IRQ_R_OFF:

		// Absorb bytes and ignore them
		b = RBUF;
		break;

	case IRQ_R_STRT:

	    	// Receiving header byte
		b = RBUF;
		if (b != 0x55)
			// Waiting for the "preamble"
			break;
#if 0
		// Trigger 'reception started' event (for timeouts)
		RISE_N_SHINE;
		if (UA->r_prcs != 0)
			p_trigger (UA->r_prcs, RSEVENT);
#endif
		UA->r_istate = IRQ_R_LEN;
		break;

	case IRQ_R_LEN:

		b = RBUF;
		if ((b & 1) != 0 || b > UA->r_buffl - 4) {
			// Length field error, ignore bytes until we are
			// reset by the process
			UA->r_buffer [0] = 0xffff;
			goto RxEvent;
		}
		// The number of expected bytes
		UA->r_buffs = b + 4;
		UA->r_buffp = 0;
		UA->r_istate = IRQ_R_PKT;
		break;

	case IRQ_R_PKT:

		((byte*)(UA->r_buffer)) [UA->r_buffp++] = RBUF;
		if (UA->r_buffp == UA->r_buffs) {
			// All done
RxEvent:
			RISE_N_SHINE;
			if (UA->r_prcs != 0)
				p_trigger (UA->r_prcs, RXEVENT);
#ifdef UART_XMITTER_ON
			// Half duplex UART: notify the sender it can go
			if (UA->x_prcs != 0)
				p_trigger (UA->x_prcs, RDYEVENT);
#endif
			UA->r_istate = IRQ_R_OFF;
		}
		break;
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
		break;

	case IRQ_R_STRT:

	    	// Receiving header byte
		b = RBUF;
		if ((b & 0xfc) != 0)
			// Waiting for something that looks like a header
			// byte
			break;
		// Trigger 'reception started' event
		RISE_N_SHINE;
		if (UA->r_prcs != 0)
			p_trigger (UA->r_prcs, RSEVENT);
		((byte*)(UA->r_buffer)) [0] = b;
		UA->r_istate = IRQ_R_LEN;
		break;

	case IRQ_R_LEN:

		b = RBUF;
		if (b > UA->r_buffl - 4) {
			// Length field error, ignore bytes until we are
			// reset by the process
			UA->r_buffer [0] = 0xffff;
			// Wake up the process to reset things
			goto RxEvent;
		}
		UA->r_buffs = b;
		UA->r_buffp = 0;
		((byte*)(UA->r_buffer)) [1] = b;
		UA->r_istate = IRQ_R_PKT;
		break;

	case IRQ_R_PKT:

		((byte*)(UA->r_buffer + 1)) [UA->r_buffp++] = RBUF;
		if (UA->r_buffp > UA->r_buffs && (UA->r_buffp & 1))
			// Read the first byte of checksum
			UA->r_istate = IRQ_R_CHK1;
		break;

	case IRQ_R_CHK1:

		// Second byte of checksum
		((byte*)(UA->r_buffer + 1)) [UA->r_buffp++] = RBUF;
		// Complete packet
RxEvent:
		RISE_N_SHINE;
		if (UA->r_prcs != 0)
			p_trigger (UA->r_prcs, RXEVENT);
#ifdef UART_XMITTER_ON
		// Half duplex: notify the sender it can go
		if (UA->x_prcs != 0)
			p_trigger (UA->x_prcs, RDYEVENT);
#endif
		UA->r_istate = IRQ_R_OFF;
		break;
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
			break;

		// Network ID field not used, i.e., usable for payload
		UA->r_buffp = 1;
		*((byte*)(UA->r_buffer)) = b;
		UA->r_istate = IRQ_R_LIN;
		LEDIU (2, 1);
		break;

	case IRQ_R_LIN:

		if (b == '\r' || b == '\n') {
			// Either one terminates the line, the other one
			// will be discarded at the beginning of next one
			RISE_N_SHINE;
			if (UA->r_prcs != 0)
				p_trigger (UA->r_prcs, RXEVENT);
#ifdef UART_XMITTER_ON
			// Half duplex: notify the sender it can go
			if (UA->x_prcs != 0)
				p_trigger (UA->x_prcs, RDYEVENT);
#endif
			UA->r_istate = IRQ_R_OFF;
		} else {
			if (UA->r_buffp < UA->r_buffl)
				// This will truncate longer lines
				((byte*)(UA->r_buffer)) [UA->r_buffp++] = b;
		}

	// default:

		// Nothing
		// break;

	
    }	

#endif

#if UART_TCV_MODE == UART_TCV_MODE_E || UART_TCV_MODE == UART_TCV_MODE_F
// ============================================================================
// STX-ETX-DLE ================================================================
// ============================================================================

    b = RBUF;
    if (b == 0x10 && (UA->v_flags & UAFLG_ESCP) == 0) {

	UA->v_flags |= UAFLG_ESCP;
	// Ignore

    } else {

    	switch (UA->r_istate) {

		case IRQ_R_OFF:

			break;

		case IRQ_R_STRT:

			if (b == 0x02 && (UA->v_flags & UAFLG_ESCP) == 0) {
				// Expecting STX
				LEDIU (2, 1);
STX_rst:
				UA->r_buffp = 0;
				UA->r_istate = IRQ_R_PKT;
			}
			break;

		case IRQ_R_PKT:

			if ((UA->v_flags & UAFLG_ESCP) == 0) {
				// Unescaped
				if (b == 0x03) {
					// The end
					RISE_N_SHINE;
					if (UA->r_prcs != 0)
						p_trigger (UA->r_prcs, RXEVENT);
#ifdef UART_XMITTER_ON
					// Half-duplex
					if (UA->x_prcs != 0)
						p_trigger (UA->x_prcs,
							RDYEVENT);
#endif
					UA->r_istate = IRQ_R_OFF;
					break;
				}
				if (b == 0x02)
					// STX reset
					goto STX_rst;
			}
			if (UA->r_buffp >= UA->r_buffl) {
				// Too long, reset
				LEDIU (2, 0);
				UA->r_istate = IRQ_R_STRT;
				break;
			}
			((byte*)(UA->r_buffer)) [UA->r_buffp++] = b;
	    }

	    UA->v_flags &= ~UAFLG_ESCP;
    }

#endif
