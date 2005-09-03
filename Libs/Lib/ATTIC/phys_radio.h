#ifndef	__pg_phys_radio_h
#define	__pg_phys_radio_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define	RADIO_DEF_BUF_LEN	32	/* Default buffer length */
/*
 * These values are merely defaults changeable with tcv_control
 */
#define	RADIO_DEF_TCVSENSE	4	/* Pre-Tx activity sense time */

#define	RADIO_POST_SPACE	1	/* Milliseconds */

void phys_radio (int, int, int);

#endif
