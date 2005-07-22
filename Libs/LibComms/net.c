/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
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

#if CHIPCON
#include "phys_chipcon.h"
#endif

#if DM2100
#include "phys_dm2100.h"
#endif

#include "tarp.h"
#include "diag.h"
/*
#include "phys_sim.h"
*/

extern int net_fd;
extern int net_phys;
extern int net_plug;

int net_opt (int opt, address arg) {
	if (opt == PHYSOPT_PHYSINFO)
		return net_phys;

	if (opt == PHYSOPT_PLUGINFO)
		return net_plug;

	if (net_fd < 0)
		return -1; // if uninited, tcv_control traps on fd assertion

	return tcv_control (net_fd, opt, arg);
}

static int ether_init (word);
static int uart_init (word);
static int radio_init (word);
static int chipcon_init (word);
static int dm2100_init (word);
// static int sim_init (word);

#define	NET_MAXPLEN		128
static const char * myName = "net_init";

int net_fd   = -1;
int net_phys = -1;
int net_plug = -1;

int net_init (word phys, word plug) {

	if (net_fd >= 0) {
		diag ("%s: Net busy", myName);
		return -1;
	}
	
	net_phys = phys;
	net_plug = plug;

	switch (phys) {

	case INFO_PHYS_RADIO:
		return (net_fd = radio_init (plug));

	case INFO_PHYS_CHIPCON:
		return (net_fd = chipcon_init (plug));

	case INFO_PHYS_DM2100:
		return (net_fd = dm2100_init (plug));

	case INFO_PHYS_ETHER:
		return (net_fd = ether_init (plug));

	case INFO_PHYS_UART:
		return (net_fd = uart_init (plug));
/*
	case INFO_PHYS_SIM:
		return (net_fd = sim_init (plug));
*/
	default:
		diag ("%s: Unknown phys", myName);
		return (net_fd = -1);
	}
}

static int radio_init (word plug) {
#if !RADIO_DRIVER

	diag ("%s: Radio driver missing", myName);
	return -1;

#else
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

#endif
}

static int chipcon_init (word plug) {
#if !CHIPCON
	diag ("%s: Chipcon phys missing", myName);
	return -1;
#else
	int fd;
	word opts[2] = {4, 2}; // checksum + power
	phys_chipcon (0, 0, NET_MAXPLEN, 192);

	if (plug == INFO_PLUG_TARP)
		tcv_plug (0, &plug_tarp);
	else
		tcv_plug (0, &plug_null);

	if ((fd = tcv_open (NONE, 0, 0)) < 0) {
		diag ("%s: Cannot open chipcon interface", myName);
		return -1;
	}

	tcv_control (fd, PHYSOPT_SETPARAM, opts);
	opts[0] = 5; opts[1] = 1; // get rssi
	tcv_control (fd, PHYSOPT_SETPARAM, opts);
	tcv_control (fd, PHYSOPT_TXON, NULL);
	tcv_control (fd, PHYSOPT_RXON, NULL);
	return fd;

#endif
}

static int dm2100_init (word plug) {
#if !DM2100
	diag ("%s: DM2100 phys missing", myName);
	return -1;
#else
	int fd;
	word opts[2] = {4, 2}; // checksum + power
	phys_dm2100 (0, 0, NET_MAXPLEN);

	if (plug == INFO_PLUG_TARP)
		tcv_plug (0, &plug_tarp);
	else
		tcv_plug (0, &plug_null);

	if ((fd = tcv_open (NONE, 0, 0)) < 0) {
		diag ("%s: Cannot open dm2100 interface", myName);
		return -1;
	}

	tcv_control (fd, PHYSOPT_SETPARAM, opts);
	opts[0] = 5; opts[1] = 1; // get rssi
	tcv_control (fd, PHYSOPT_SETPARAM, opts);
	tcv_control (fd, PHYSOPT_TXON, NULL);
	tcv_control (fd, PHYSOPT_RXON, NULL);
	return fd;
#endif
}
	
static int ether_init (word plug) {

#if !ETHERNET_DRIVER

	diag ("%s: Ethernet driver missing", myName);
	return -1;

#else

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

#endif

}

static int uart_init (word plug) {

#if UART_DRIVER != 2

	diag ("%s: Uart_b driver missing", myName);
	return -1;

#else

// #define	NET_CHANNEL_MODE	UART_PHYS_MODE_EMU
// or
#define	NET_CHANNEL_MODE	UART_PHYS_MODE_DIRECT
#define	NET_RATE			19200

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

#endif

}

// 7 words, 62 is the shortest frame
#define ether_min_frame		62
#define ether_offset(len)	((len) + 7)
#define	ether_ptype		0x6007

extern int net_fd;
extern int net_phys;

int net_rx (word state, char ** buf_ptr, address rssi_ptr) {

	static const char * myName = "net_rx";
	address packet;
	word size;

	if (buf_ptr == NULL) {
		diag ("%s: NULL buf addr", myName);
		return -1;
	}
	
	if (net_fd < 0) {
		diag ("%s: No net", myName);
		return -1;
	}
/* option?
	if (!(tcv_control(net_fd, PHYSOPT_STATUS, NULL) & 1)) {
		diag ("%s: Rx off", myName);
		return -1;
	}
*/
	packet = tcv_rnp (state, net_fd); // if silence, will hang and go to state if not NONE
	// size = tcv_left (packet);
	if ((size = tcv_left (packet)) == 0) // this may be only if state is nonblocking NONE (?)
		return 0;

	if (rssi_ptr) {
		if (net_phys == INFO_PHYS_CHIPCON ||
			net_phys == INFO_PHYS_DM2100)
			// ?? under a specific option?? check***
			*rssi_ptr = packet[(size >> 1) -1];
		else
			*rssi_ptr = 0;
	}

	switch (net_phys) {

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

	case INFO_PHYS_UART:
		if (*buf_ptr == NULL)
			*buf_ptr = (char *)umalloc(size);
		memcpy(*buf_ptr, (char *)packet, size);
		tcv_endp(packet);
		return size;

	case INFO_PHYS_RADIO:
	case INFO_PHYS_CHIPCON:
	case INFO_PHYS_DM2100:
		size -= 6;
		if (*buf_ptr == NULL)
			*buf_ptr = (char *)umalloc(size);
		memcpy(*buf_ptr, (char *)(packet +1), size);
		tcv_endp(packet);
		return size;
		 
	default:
		diag ("%s: Unknown phys", myName);
		return -1;
	}

	return -1;
}

// 7 words, 15 = 14 head + 1 tail bytes (needed), 62 is the shortest frame
#define ether_min_frame		62
#define ether_offset(len)	((len) + 7)
#define	ether_ptype			0x6007
#define ether_len(len)	 	((62 >= ((len) + 15)) ? 62 : ((len) + 15))

// 2 - header, 4 - crc, +3 mod 4 == 0
// (if the length is not divisible by 4, strange things may happen)
#define radio_len(len)		(((len) + 2+ 4 + 3) & 0xfffc)
extern int net_fd;
extern int net_phys;
extern int net_plug;

int net_tx (word state, char * buf, int len) {

	static const word ether_maddr [3] = { 0xe1aa, 0xbbcc, 0xddee};
	static const char * myName = "net_tx";
	address packet;

	if (buf == NULL || len <= 0) {
		diag ("%s: Crap in", myName);
		return -1;
	}
	
	if (net_fd < 0) {
		diag ("%s: No net", myName);
		return -1;
	}

	if (!(tcv_control (net_fd, PHYSOPT_STATUS, NULL) & 2)) {
		diag ("%s: Tx off", myName);
		return -1;
	}

	if (net_phys == INFO_PHYS_ETHER) {
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

	} else if (net_phys == INFO_PHYS_RADIO ||
			net_phys == INFO_PHYS_CHIPCON ||
			net_phys == INFO_PHYS_DM2100) {
		packet = tcv_wnp (state, net_fd, radio_len(len)); 
		if (packet == NULL) {
			net_diag (D_DEBUG, "%s: wnp failed", myName);
			return -1;
		}
		net_diag (D_DEBUG, "%s: tcv_wnp len (%u->%u)", myName,
			len, radio_len(len)); 
		
		packet[0] = 0; // not really needed in this mode?
		memcpy (packet + 1,  buf, len);

	} else {
		packet = tcv_wnp (state, net_fd, len);
		if (packet == NULL) {
			net_diag (D_DEBUG, "%s: wnp failed (2)", myName);
			return -1;
		}
		memcpy ((char*)packet, buf, len);
	}
	tcv_endp (packet);
	return 0;
}

extern int net_fd;
extern int net_phys;
extern int net_plug;
int net_close (word state) {

	static const char * myName = "net_close";
	int rc;

	if (net_fd < 0) {
		diag ("%s: Net not open", myName);
		return -1;
	}
	
	if ((rc = tcv_close(state, net_fd)) != 0)
		diag ("%s: Close err %d", myName, rc);

	// better (?) reset things in case of an error, too
	net_fd = -1;
	net_phys = -1;
	net_plug = -1;
	return rc;
}


