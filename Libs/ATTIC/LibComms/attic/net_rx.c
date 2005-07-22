/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

//nclude "tcvphys.h"
//#include "plug_null.h"
//#include "phys_ether.h"
//#include "phys_uart.h"
//nclude "phys_sim.h"


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
		if (net_phys == INFO_PHYS_CHIPCON)
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
