/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

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
