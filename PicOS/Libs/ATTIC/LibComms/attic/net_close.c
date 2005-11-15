/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

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


