/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "sysio.h"
#include "tcvphys.h"
#include "plug_null.h"
#include "plug_tarp.h"
#include "tarp.h"
#include "net.h"

#ifndef __SMURPH__
#include "encrypt.h"
#endif

static const lword enkeys [12] = {
	0xbabadead, 0x12345678, 0x98765432, 0x6754a6cd,
	0xbacaabeb, 0xface0fed, 0xabadef01, 0x23456789,
	0xfedcba98, 0x76543210, 0xdadabcba, 0x90987654 };

int net_fd   = -1;
int net_phys = -1;
int net_plug = -1;

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
word net_pxopts = 0x7000; // DEF_NET_PXOPTS;
#endif

int net_opt (int opt, address arg) {

	if (opt == PHYSOPT_PHYSINFO)
		return net_phys;

	if (opt == PHYSOPT_PLUGINFO)
		return net_plug;

	if (net_fd < 0)
		return -1; // if uninited, tcv_control traps on fd assertion

	return tcv_control (net_fd, opt, arg);
}

int net_qera (int d) {

	if (net_fd < 0)
		return -1;
	return tcv_erase (net_fd, d);
}

int net_qsize (int d) {

	if (net_fd < 0)
		return -1;
	return tcv_qsize (net_fd, d);
}

#if ETHERNET_DRIVER
static int ether_init (word);
#endif

#if UART_TCV
static int uart_init (word);
#endif

#if CC1000
static int cc1000_init (word);
#endif

#if CC1100
static int cc1100_init (word);
#endif

#if CC1350_RF
static int cc1350_init (word);
#endif

#if CC2420
static int cc2420_init (word);
#endif

#if DM2200
static int dm2200_init (word);
#endif

#ifdef	myName
#undef myName
#endif

#define	myName "net.c"

int net_init (word phys, word plug) {

	if (net_fd >= 0) {
		dbg_2 (0x1000); // net busy
		diag ("%s: Net busy", myName);
		return -1;
	}

	net_phys = phys;
	net_plug = plug;

	switch (phys) {
#if CC1000
	case INFO_PHYS_CC1000:
		return (net_fd = cc1000_init (plug));
#endif
#if CC1350_RF
	case INFO_PHYS_CC1350:
		return (net_fd = cc1350_init (plug));
#endif
#if CC1100
	case INFO_PHYS_CC1100:
		return (net_fd = cc1100_init (plug));
#endif
#if CC2420
	case INFO_PHYS_CC2420:
		return (net_fd = cc2420_init (plug));
#endif
#if DM2200
	case INFO_PHYS_DM2200:
		return (net_fd = dm2200_init (plug));
#endif
#if ETHERNET_DRIVER
	case INFO_PHYS_ETHER:
		return (net_fd = ether_init (plug));
#endif
#if UART_TCV
	case INFO_PHYS_UART:
		return (net_fd = uart_init (plug));
#endif
	default:
		dbg_2 (0x9000 | phys);
		diag ("%s: ? phys %d", myName, phys);
		return (net_fd = -1);
	}
}

#if CC1000
static int cc1000_init (word plug) {
	int fd;
	// opts removed word opts[2] = {4, 2}; // checksum + power
	phys_cc1000 (0, NET_MAXPLEN, 192);

	if (plug == INFO_PLUG_TARP)
		tcv_plug (0, &plug_tarp);
	else
		tcv_plug (0, &plug_null);

	if ((fd = tcv_open (WNONE, 0, 0)) < 0) {
		diag ("%s: Cannot open CC1000 if", myName);
		return -1;
	}

	tcv_control (fd, PHYSOPT_TXON, NULL);
	tcv_control (fd, PHYSOPT_RXON, NULL);
	return fd;
}
#endif

#if CC1100
static int cc1100_init (word plug) {
	int fd;
	phys_cc1100 (0, NET_MAXPLEN);
	if (plug == INFO_PLUG_TARP)
		tcv_plug (0, &plug_tarp);
	else
		tcv_plug (0, &plug_null);
	if ((fd = tcv_open (WNONE, 0, 0)) < 0) {
		dbg_2 (0x2000); // Cannot open cc1100
		diag ("%s: Cannot open CC1100 if", myName);
		return -1;
	}
	tcv_control (fd, PHYSOPT_TXON, NULL);
	tcv_control (fd, PHYSOPT_RXON, NULL);
	return fd;
}
#endif

#if CC1350_RF
static int cc1350_init (word plug) {
	int fd;
	phys_cc1350 (0, NET_MAXPLEN);
	if (plug == INFO_PLUG_TARP)
		tcv_plug (0, &plug_tarp);
	else
		tcv_plug (0, &plug_null);
	if ((fd = tcv_open (WNONE, 0, 0)) < 0) {
		dbg_2 (0x2000); // Cannot open cc1100
		diag ("%s: Cannot open CC1350 if", myName);
		return -1;
	}
	tcv_control (fd, PHYSOPT_TXON, NULL);
	tcv_control (fd, PHYSOPT_RXON, NULL);
	return fd;
}
#endif

#if CC2420
static int cc2420_init (word plug) {
	int fd;
	phys_cc2420 (0, NET_MAXPLEN);
	if (plug == INFO_PLUG_TARP)
		tcv_plug (0, &plug_tarp);
	else
		tcv_plug (0, &plug_null);
	if ((fd = tcv_open (WNONE, 0, 0)) < 0) {
		dbg_2 (0x2000); // Cannot open cc2420
		diag ("%s: Cannot open CC2420 if", myName);
		return -1;
	}
	tcv_control (fd, PHYSOPT_TXON, NULL);
	tcv_control (fd, PHYSOPT_RXON, NULL);
	return fd;
}
#endif

#if DM2200
static int dm2200_init (word plug) {
	int fd;
	phys_dm2200 (0, NET_MAXPLEN);

	if (plug == INFO_PLUG_TARP)
		tcv_plug (0, &plug_tarp);
	else
		tcv_plug (0, &plug_null);

	if ((fd = tcv_open (WNONE, 0, 0)) < 0) {
		diag ("%s: Cannot open dm2200 interface", myName);
		return -1;
	}

	tcv_control (fd, PHYSOPT_TXON, NULL);
	tcv_control (fd, PHYSOPT_RXON, NULL);
	return fd;
}
#endif

#if ETHERNET_DRIVER
static int ether_init (word plug) {
	char c;
	int  fd;

	phys_ether (0, 0, NET_MAXPLEN); // phys_plug (0), raw mode (0)
	c = 1;
	ion (ETHERNET, CONTROL, &c, ETHERNET_CNTRL_MULTI);

	if (plug == INFO_PLUG_TARP)
		tcv_plug (0, &plug_tarp);
	else
		tcv_plug (0, &plug_null);

	// state, phid, plid (indices, not ids)
	if ((fd = tcv_open (WNONE, 0, 0)) < 0) {
		diag ("%s: Cannot open tcv ethernet interface", myName);
		return -1;
	}
	tcv_control (fd, PHYSOPT_TXON, NULL);
	tcv_control (fd, PHYSOPT_RXON, NULL);
	return fd;
}
#endif

#if UART_TCV
static int uart_init (word plug) {

// #define	NET_CHANNEL_MODE	UART_PHYS_MODE_EMU
// or
// #define 	NET_CHANNEL_MODE	UART_PHYS_MODE_DIRECT

#define NET_RATE			192
#define	WHICH_UART			0

	int  fd;

	// uart_init_p [0] = (word) NET_RATE;
	// ion (UART_B, CONTROL, (char*)uart_init_p, UART_CNTRL_RATE);
	// Select the first uart (0)
	phys_uart (0, NET_MAXPLEN, WHICH_UART);

	if (plug == INFO_PLUG_TARP)
		tcv_plug (0, &plug_tarp);
	else
		tcv_plug (0, &plug_null);

	if ((fd = tcv_open (WNONE, 0, 0)) < 0) {
		diag ("%s: Cannot open tcv uart_b interface", myName);
		return -1;
	}
#ifdef	NET_RATE
	{ word rt; 
		rt = NET_RATE;
		tcv_control (fd, PHYSOPT_SETRATE, &rt);
	}
#endif
	tcv_control (fd, PHYSOPT_TXON, NULL);
	tcv_control (fd, PHYSOPT_RXON, NULL);
	return fd;

#undef	NET_CHANNEL_MODE
#undef	NET_RATE
}
#endif

int net_rx (word state, char ** buf_ptr, address rssi_ptr, byte encr) {

	address packet;
	word size;
	byte in_key;

	if (buf_ptr == NULL || net_fd < 0) {
		dbg_2 (0x3000); // net_rx: no buffer or not net
		diag ("%s: no buffer or not net",  myName);
		return -1;
	}

	packet = tcv_rnp (state, net_fd);
	if (!packet || (size = tcv_left (packet)) == 0)
	       	// only if state is NONE (non-blocking)
		return 0;

	if (rssi_ptr) {
#if CC1100 || CC1000 || DM2200 || CC2420 || CC1350_RF
		*rssi_ptr = packet[(size >> 1) -1];
#else
		*rssi_ptr = 0;
#endif
	}

	switch (net_phys) {
#if ETHERNET_DRIVER
	case INFO_PHYS_ETHER:
		if (packet [6] != ether_ptype || size < ether_min_frame) {
			diag ("%s: Bad packet", myName);
			tcv_endp (packet);
			if (state == NONE)
				return 0;
			else
				proceed (state);
		}
		size -= (ether_offset(0) <<1);
		if (*buf_ptr == NULL)
			*buf_ptr = (char *)umalloc(size);
		memcpy(*buf_ptr, (char *)ether_offset(packet), size);
		tcv_endp(packet);
		return size;
#endif

#if UART_TCV
	case INFO_PHYS_UART:
		if (*buf_ptr == NULL)
			*buf_ptr = (char *)umalloc(size);
		memcpy(*buf_ptr, (char *)packet, size);
		tcv_endp(packet);
		return size;
#endif

	case INFO_PHYS_CC1000:
	case INFO_PHYS_CC1100: // sid, entropy, rssi
	case INFO_PHYS_CC1350:
	case INFO_PHYS_CC2420:
	case INFO_PHYS_DM2200:
		size = phys_to_payload (size);
		if (size == 0 || size > NET_MAXPLEN) {
			tcv_endp(packet);
			return -1;
		}

		if (*buf_ptr == NULL)
			*buf_ptr = (char *)umalloc(size);
		if (*buf_ptr == NULL)
			size = 0;
		else {
			// packet: {sid, header, payload, 0/1B, entropy, rssi}
			in_key = in_header(packet +1, msg_type);
			if (!msg_isClear(in_key)) {
			    in_key = tarp_mEncr (in_key);
			    if ((encr & 4) && (encr & 3) != in_key) {
				tcv_endp(packet);
				dbg_8 (0x4000 | in_key); // strict & mismatch 
				return 0;
			    }
			    if (in_key) {
			 	decrypt (packet + 1 + (sizeof(headerType) >> 1),
				  (size + NET_FRAME_LENGTH - 2 - 
				    sizeof(headerType) - 2) >> 1,
				      &enkeys[(in_key -1) << 2]);
				in_key = *(((byte *)enkeys +
			 	    ((in_key -1) << 4)) +
				    (in_header(packet + 1, msg_type) & 0x0F));
				if (*((byte *)packet + size +
				  NET_FRAME_LENGTH -1 -2 -1) != in_key) {
					tcv_endp(packet);
					dbg_8 (0x5000 | in_key);
					return 0;
				}
			    }
			}
			memcpy(*buf_ptr, (char *)(packet +1), size);
			**buf_ptr &= TARP_MSGTYPE_MASK;
		}
		tcv_endp(packet);
		return size;

	default:
		dbg_2 (0x4000); // Unknown phys
		diag ("%s: Unknown phys", myName);
		return -1;
	}

	return -1;
}

int net_tx (word state, char * buf, int len, byte encr) {

#if ETHERNET_DRIVER
	static const word ether_maddr [3] = { 0xe1aa, 0xbbcc, 0xddee};
#endif
	address packet;

	if (buf == NULL || len <= 0) {
		dbg_2 (0x5000); // net_tx: corrupted input
		diag ("%s: corrupted input", myName);
		return -1;
	}

	if (net_fd < 0) {
		dbg_2 (0x6000); // net_tx: no net
		diag ("%s: no net", myName);
		return -1;
	}

	if (!(tcv_control (net_fd, PHYSOPT_STATUS, NULL) & 2)) {
#if 0
application should decide if this is an error...
		dbg_8 (0x1000); // xmt off
		diag ("xmt off");
#endif
		return 1;
	}

	switch (net_phys) {
#if ETHERNET_DRIVER
	  case INFO_PHYS_ETHER:
		packet = tcv_wnp (state, net_fd, ether_len (len));
		if (packet == NULL) {	// state is NONE and we failed
			diag ("%s: wnp failed", myName);
			return -1;
		}
		/* PG make sure this isn't interpreted as length */
		packet [6] = ether_ptype;
		/* and that this is a multicast address */
		memcpy (packet + 0, ether_maddr, 6);
		memcpy ((char*)ether_offset(packet), buf, len);
		break;
#endif
	  case INFO_PHYS_CC1000:
	  case INFO_PHYS_CC1100:
	  case INFO_PHYS_CC1350:
	  case INFO_PHYS_CC2420:
	  case INFO_PHYS_DM2200:
		packet = tcv_wnp (state, net_fd, payload_to_phys (len));
		if (packet == NULL) {
			return -1;
		}
		packet[0] = net_id;
		memcpy (packet + 1,  buf, len);
		// always load entropy, just before rssi
		packet [(payload_to_phys (len) >> 1) - 2] = (word) entropy;

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
		packet [(payload_to_phys (len) >> 1) - 1] = net_pxopts;
#endif

		if ((encr & 3) && !msg_isClear(*buf)) {
			// add encr to msg_type
			*(byte *)(packet +1) |= encr << TARP_ENCRYPT_SHIFT;
			// load the "signature"
			*((byte *)packet + payload_to_phys (len) -4) =
				*((byte *)enkeys + (((encr & 3) -1) << 4) +
					(in_header(buf, msg_type) & 0x0F));
			encrypt (packet + 1 + (sizeof(headerType) >> 1),
			  (payload_to_phys (len) - 2 - sizeof(headerType) - 2)
			    >> 1, &enkeys[((encr & 3) -1) << 2]);
		}
		break;

	  default:
		dbg_2 (0x6000 | net_phys); // net_tx: corrupted phys
		diag ("%s: currupted phys %d", net_phys);
		return -1;
#if 0
		packet = tcv_wnp (state, net_fd, len);
		if (packet == NULL) {
			return -1;
		}
		memcpy ((char*)packet, buf, len);
#endif
	}
	tcv_endp (packet);
	return 0;
}

int net_close (word state) {

	int rc;
	if (net_fd < 0) {
		dbg_2 (0x7000); // net_close: not opened
		diag ("%s: not opened", myName);
		return -1;
	}

	if ((rc = tcv_close(state, net_fd)) != 0) {
		dbg_2 (0x8000 | rc); // net_close: tcv error
		diag ("%s: tcv_close err");
	}

	net_fd = -1;
	net_phys = -1;
	net_plug = -1;
	return rc;
}
