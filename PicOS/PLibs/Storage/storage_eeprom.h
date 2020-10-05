/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __storage_eeprom_h__
#define	__storage_eeprom_h__

/*
 * This file is included by all storage_.....c files
 */

lword ee_size (Boolean *er, lword *eru) {

	if (er)
		*er = (Boolean) EE_ERASE_BEFORE_WRITE;

	if (eru)
		*eru = (lword) EE_ERASE_UNIT;

	return EE_SIZE;
}

void ee_init_erase () {

	ee_open ();
	ee_erase (WNONE, 0, 0);
	ee_close ();
}

#endif
