#ifndef __pg_lcdg_images_params_h__
#define __pg_lcdg_images_params_h__

#define	LCDG_IM_PAGESIZE	8192
// Max chunks per image
#define	LCDG_IM_MAXCHUNKS	720
// Image chunk length (includes the chunk number, which has to be stored)
#define	LCDG_IM_CHUNKLEN	(OEP_CHUNKLEN + 2)
// Chunks per page (they never cross page boundaries)
#define	LCDG_IM_CHUNKSPP	(LCDG_IM_PAGESIZE / LCDG_IM_CHUNKLEN)
// Label length
#define	LCDG_IM_LABLEN		(LCDG_IM_CHUNKLEN - 18)
// Max pages per image
#define	LCDG_IM_MAXPPI	5

// Errors (kind of compatible with OEP status values)
#define	LCDG_IMGERR_HANDLE	32
#define	LCDG_IMGERR_NOMEM	OEP_STATUS_NOMEM
#define	LCDG_IMGERR_GARBAGE	33
#define	LCDG_IMGERR_NOSPACE	34
#define	LCDG_IMGERR_FAIL	35

#endif
