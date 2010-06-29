#ifndef __lcdg_dispman_node_data_h__
#define __lcdg_dispman_node_data_h__

		// Top object displayed
lcdg_dm_obj_t	*LCDG_DM_TOP	__sinit (NULL),
		// The front of the list of displayed objects
		*LCDG_DM_HEAD	__sinit (NULL);

byte		LCDG_DM_STATUS	__sinit (0);	// Error status

		// Wallpaper image handle
__STATIC word	lcdg_dm_wph	__sinit (0);

#endif
