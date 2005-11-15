/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "tarp.h"
#include "diag.h"

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

int net_tx (word state, char * buf, int len, word retry, bool urgent) {

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
			net_phys == INFO_PHYS_CHIPCON) {
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
	if (net_plug == INFO_PLUG_TARP) {
		tarp_retry = retry;
		tarp_urgent = urgent;
	}
	tcv_endp (packet);
	return 0;
}

