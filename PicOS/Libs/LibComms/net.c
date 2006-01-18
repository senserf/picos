/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "tcvphys.h"
#include "plug_null.h"
#include "plug_tarp.h"

#if ETHERNET_DRIVER
#include "phys_ether.h"
#endif

#if UART_DRIVER > 1
#include "phys_uart.h"
#endif

#if RADIO_DRIVER
#include "phys_radio.h"
#endif

#if CC1000
#include "phys_cc1000.h"
#endif

#if CC1100
#include "phys_cc1100.h"
#endif

#if DM2100
#include "phys_dm2100.h"
#endif

#include "tarp.h"

int net_fd = -1;
int net_phys = -1;
int net_plug = -1;

int net_opt (int opt, address arg) {
	if (opt == PHYSOPT_PHYSINFO)
		return net_phys;

	if (opt == PHYSOPT_PLUGINFO)
		return net_plug;

	if (net_fd < 0)
		return -1; // if uninited, tcv_control traps on fd assertion

	return tcv_control (net_fd, opt, arg);
}

#if ETHERNET_DRIVER
static int ether_init (word);
#endif

#if UART_DRIVER > 1
static int uart_init (word);
#endif

#if RADIO_DRIVER
static int radio_init (word);
#endif

#if CC1000
static int cc1000_init (word);
#endif

#if CC1100
static int cc1100_init (word);
#endif

#if DM2100
static int dm2100_init (word);
#endif

#define NET_MAXPLEN		56
static const char myName[] = "net.c";

int net_init (word phys, word plug) {

	if (net_fd >= 0) {
		dbg_2 (0x1000); // net busy
		diag ("%s: Net busy", myName);
		return -1;
	}

	net_phys = phys;
	net_plug = plug;

	switch (phys) {
#if RADIO_DRIVER
	case INFO_PHYS_RADIO:
		return (net_fd = radio_init (plug));
#endif
#if CC1000
	case INFO_PHYS_CC1000:
		return (net_fd = cc1000_init (plug));
#endif
#if CC1100
	case INFO_PHYS_CC1100:
		return (net_fd = cc1100_init (plug));
#endif
#if DM2100
	case INFO_PHYS_DM2100:
		return (net_fd = dm2100_init (plug));
#endif
#if ETHERNET_DRIVER
	case INFO_PHYS_ETHER:
		return (net_fd = ether_init (plug));
#endif
#if UART_DRIVER > 1
	case INFO_PHYS_UART:
		return (net_fd = uart_init (plug));
#endif
	default:
		dbg_2 (0x9000 | phys);
		diag ("%s: ? phys %d", myName, phys);
		return (net_fd = -1);
	}
}

#if RADIO_DRIVER
static int radio_init (word plug) {
	int fd;

	phys_radio (0, 0, NET_MAXPLEN);

	if (plug == INFO_PLUG_TARP)
		tcv_plug (0, &plug_tarp);
	else
		tcv_plug (0, &plug_null);

	if ((fd = tcv_open (NONE, 0, 0)) < 0) {
		diag ("%s: Cannot open tcv radio interface", myName);
		return -1;
	}

	tcv_control (fd, PHYSOPT_TXON, NULL); // just for now
	tcv_control (fd, PHYSOPT_RXON, NULL);
	return fd;
}
#endif

#if CC1000
static int cc1000_init (word plug) {
	int fd;
	// opts removed word opts[2] = {4, 2}; // checksum + power
	phys_cc1000 (0, NET_MAXPLEN, 192);

	if (plug == INFO_PLUG_TARP)
		tcv_plug (0, &plug_tarp);
	else
		tcv_plug (0, &plug_null);

	if ((fd = tcv_open (NONE, 0, 0)) < 0) {
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
	if ((fd = tcv_open (NONE, 0, 0)) < 0) {
		dbg_2 (0x2000); // Cannot open cc1100
		diag ("%s: Cannot open CC1100 if", myName);
		return -1;
	}
	tcv_control (fd, PHYSOPT_TXON, NULL);
	tcv_control (fd, PHYSOPT_RXON, NULL);
	return fd;
}
#endif

#if DM2100
static int dm2100_init (word plug) {
	int fd;
	phys_dm2100 (0, NET_MAXPLEN);

	if (plug == INFO_PLUG_TARP)
		tcv_plug (0, &plug_tarp);
	else
		tcv_plug (0, &plug_null);

	if ((fd = tcv_open (NONE, 0, 0)) < 0) {
		diag ("%s: Cannot open dm2100 interface", myName);
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
	if ((fd = tcv_open (NONE, 0, 0)) < 0) {
		diag ("%s: Cannot open tcv ethernet interface", myName);
		return -1;
	}
	tcv_control (fd, PHYSOPT_TXON, NULL);
	tcv_control (fd, PHYSOPT_RXON, NULL);
	return fd;
}
#endif

#if UART_DRIVER == 2
static int uart_init (word plug) {

// #define	NET_CHANNEL_MODE	UART_PHYS_MODE_EMU
// or
#define NET_CHANNEL_MODE	UART_PHYS_MODE_DIRECT
#define NET_RATE			19200

	static word p [6]; // keep all 6? (does io write anything there?) ** check
	int  fd;

	p [0] = (word) NET_RATE;
	ion (UART_B, CONTROL, (char*)p, UART_CNTRL_RATE);
	phys_uart (0, UART_B, NET_CHANNEL_MODE, NET_MAXPLEN);

	if (plug == INFO_PLUG_TARP)
		tcv_plug (0, &plug_tarp);
	else
		tcv_plug (0, &plug_null);

	if ((fd = tcv_open (NONE, 0, 0)) < 0) {
		diag ("%s: Cannot open tcv uart_b interface", myName);
		return -1;
	}
	tcv_control (fd, PHYSOPT_TXON, NULL);
	tcv_control (fd, PHYSOPT_RXON, NULL);
	return fd;

#undef	NET_CHANNEL_MODE
#undef	NET_RATE
}
#endif

// 7 words, 62 is the shortest frame
#define ether_min_frame		62
#define ether_offset(len)	((len) + 7)
#define ether_ptype		0x6007

int net_rx (word state, char ** buf_ptr, address rssi_ptr) {
	address packet;
	word size;

	if (buf_ptr == NULL || net_fd < 0) {
		dbg_2 (0x3000); // net_rx: no buffer or not net
		diag ("%s: no buffer or not net",  myName);
		return -1;
	}

	packet = tcv_rnp (state, net_fd);
	if ((size = tcv_left (packet)) == 0) // only if state is NONE (?)
		return 0;

	if (rssi_ptr) {
#if CC1100 || CC1000 || DM2100
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
#if UART_DRIVER == 2
	case INFO_PHYS_UART:
		if (*buf_ptr == NULL)
			*buf_ptr = (char *)umalloc(size);
		memcpy(*buf_ptr, (char *)packet, size);
		tcv_endp(packet);
		return size;
#endif
	case INFO_PHYS_RADIO:
	case INFO_PHYS_CC1000:
	case INFO_PHYS_CC1100: // sid, entropy, rssi
	case INFO_PHYS_DM2100:
		size -= 6;
		if (size == 0 || size > NET_MAXPLEN) {
			tcv_endp(packet);
			return -1;
		}
			
		if (*buf_ptr == NULL)
			*buf_ptr = (char *)umalloc(size);
		if (*buf_ptr == NULL)
			size = 0;
		else
			memcpy(*buf_ptr, (char *)(packet +1), size);
		tcv_endp(packet);
		return size;

	default:
		dbg_2 (0x4000); // Unknown phys
		diag ("%s: Unknown phys", myName);
		return -1;
	}

	return -1;
}

// 7 words, 15 = 14 head + 1 tail bytes (needed), 62 is the shortest frame
#define ether_min_frame		62
#define ether_offset(len)	((len) + 7)
#define ether_ptype			0x6007
#define ether_len(len)		((62 >= ((len) + 15)) ? 62 : ((len) + 15))

#if CC1100
// 2 - station id, 2 - entropy, 2 - rssi, even
#define radio_len(len)  (((len) + 6 +1) & 0xfffe)
#else
// 2 - header, 4 - crc, +3 mod 4 == 0
// (if the length is not divisible by 4, things may happen)
#define radio_len(len)		(((len) + 2+ 4 + 3) & 0xfffc)
#endif

int net_tx (word state, char * buf, int len) {
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
		dbg_8 (0x1000); // xmt off
		diag ("xmt off");
		return -1;
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
	  case INFO_PHYS_RADIO:
	  case INFO_PHYS_CC1000:
	  case INFO_PHYS_CC1100:
	  case INFO_PHYS_DM2100:
		packet = tcv_wnp (state, net_fd, radio_len(len));
		if (packet == NULL) {
			return -1;
		}
		//packet[0] = 0; not needed any more
		memcpy (packet + 1,  buf, len);
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

