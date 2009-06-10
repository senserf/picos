#ifndef __lcdg_dispman_node_data_h__
#define __lcdg_dispman_node_data_h__

		// Top object displayed
lcdg_dm_obj_t	*_da (LCDG_DM_TOP)	__sinit (NULL),
		// The front of the list of displayed objects
		*_da (LCDG_DM_HEAD)	__sinit (NULL);

byte		_da (LCDG_DM_STATUS)	__sinit (0);	// Error status

		// Wallpaper image handle
__STATIC word	_da (lcdg_dm_wph)	__sinit (0);

#endif
