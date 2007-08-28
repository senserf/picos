#ifndef	__angles_h__
#define	__angles_h__

// Simple operation on agles intended for implementing directional wireless
// scenarios

#ifndef	EPSILON
#define	EPSILON	0.00000001
#endif

inline double angle (double XS, double YS, double XD, double YD) {

// This function calculates the angle from (XS,YS) to (XD,YD), which is
// between 0 and 2 PI, counting counterclokwise from East.

	double DX, DY, a;

	DX = XD - XS;
	DY = YD - YS;

	if (fabs (DX) < EPSILON)
		return DY < 0.0 ? M_PI + M_PI_2 : M_PI_2;
	else if (fabs (DY) < EPSILON)
		return DX < 0.0 ? M_PI : 0.0;
	else {
		// This is between -M_PI_2 and M_PI_2
		a = atan (DY / DX);
		if (DX < 0.0)
			return a + M_PI;
		if (DY < 0.0)
			return M_PI + M_PI + a;
		return a;
	}
}

inline double adiff (double a1, double a2) {
	// Absolute angle difference between 0 and PI
	double da = fabs (a1 - a2);
	if (da > M_PI)
		da = M_PI + M_PI - da;
	return da;
}

#endif
