#ifndef __rwpmm_h__
#define __rwpmm_h__

// Random waypoint mobility model for wireless networks

#ifndef	RWPMM_MAX_STEPS
#define	RWPMM_MAX_STEPS		10000		// Max number of steps per leg
#endif

#ifndef	RWPMM_TARGET_STEP
#define	RWPMM_TARGET_STEP	0.25		// 25 centimeters
#endif

#ifndef	RWPMM_MIN_STEPS
#define	RWPMM_MIN_STEPS		4		// Even a tiny move is no jump
#endif

#define	RWPMM_MODE_RANDOM		0	// Random point
#define	RWPMM_MODE_CIRCUMFERENCE	1	// Around the rectangle
#define	RWPMM_MODE_DIAGONAL		2	// Cross through diagonals

#define	RWPMM_MODE_MAX			RWPMM_MODE_DIAGONAL

process RWPMover;

struct rwpmm_pool_s {

	struct rwpmm_pool_s *next, *prev;

	RWPMover *Thread;
	Transceiver *RFM;
};

typedef	struct rwpmm_pool_s rwpmm_pool_t;

Boolean rwpmmMoving (Transceiver*);
Boolean rwpmmStop (Transceiver*);
void rwpmmStart (Long,			// Node Id
		 Transceiver*,		// The transceiver
			double,		// X0  ** The
			double, 	// Y0  ** bounding
			double, 	// X1  ** rectangle
			double,		// Y1  ** 
#if ZZ_R3D
			double,		// in 3d, we have X0,Y0,Z0 ...
			double,
#endif
			double, 	// Mns ** minimum speed
			double,		// Mxs ** maximum speed
			double,		// Mnp ** minimum pause
			double,		// Mxp ** maximum pause
			double h = 0.0,	// Tim ** total time (inf if 0)
			int m = 0	// Mode
		);

typedef void (*rwpmm_notifier_t)(Long);

void rwpmmSetNotifier (rwpmm_notifier_t);

process RWPMover {

	rwpmm_pool_t *ME;
	Transceiver *TR;
	Long NID;

	double	X0, Y0, X1, Y1;			// Bounding rectangle
#if ZZ_R3D
	double	Z0, Z1;
#endif
	double	MINSP, MAXSP,			// Speed
		MINPA, MAXPA;			// Pause

	double	CX, CY,				// Current coordinates
		TX, TY;				// Target coordinates
#if ZZ_R3D
	double	CZ, TZ;
#endif
	TIME	Until,				// Total time for the roaming
		TLeft;				// Left for the present leg

	Long	Count;				// Number of steps

	int	Mode;

	states { NextLeg, Advance };

	void setup (Long,			// Node number
			Transceiver*,		// The transceiver
				double,		// X0  ** 
				double, 	// Y0  ** bounding
				double, 	// X1  ** rectangle
				double,		// Y1  **
#if ZZ_R3D
				double,
				double,
#endif
				double, 	// Mns ** minimum speed
				double,		// Mxs ** maximum speed
				double,		// Mnp ** minimum pause
				double,		// Mxp ** maximum pause
				double,		// Tim ** total time (inf if 0)
				int		// Mode
					);
	perform;
};

#endif
