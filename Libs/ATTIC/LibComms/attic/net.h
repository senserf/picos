#ifndef __net_h
#define	__net_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

int net_init (word, word);
int net_close (word);
int net_opt (int, address);
int net_rx (word, char **, address);
int net_tx (word, char *, int, word, bool);

#endif
