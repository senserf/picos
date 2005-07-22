/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "tcvphys.h"
#include "plug_null.h"
#include "plug_tarp.h"
#include "phys_ether.h"
#include "phys_uart.h"
#include "phys_radio.h"
#include "phys_chipcon.h"
/*
#include "phys_sim.h"
*/
static int ether_init (word);
static int uart_init (word);
static int radio_init (word);
static int chipcon_init (word);
static int sim_init (word);

#define	NET_MAXPLEN		256
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
	phys_chipcon (0, 0, NET_MAXPLEN, 768);

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

static int sim_init (word plug) {

#if !ECOG_SIM

	diag ("%s: SimNet missing", myName);
	return -1;

#else

	int  fd;

	phys_sim (0, NET_MAXPLEN);

	if (plug == INFO_PLUG_TARP)
		tcv_plug (0, &plug_tarp);
	else
		tcv_plug (0, &plug_null);

	if ((fd = tcv_open (NONE, 0, 0)) < 0) {
		diag ("%s: Cannot open tcv simnet interface", myName);
		return -1;
	}
	tcv_control (fd, PHYSOPT_TXON, NULL);
	tcv_control (fd, PHYSOPT_RXON, NULL);
	return fd;

#endif

}
