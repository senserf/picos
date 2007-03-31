#ifndef	__channel_h__
#define	__channel_h__

// Shadowing channel model with directional hooks. Look into Examples/IncLib
// for files wchan.h, wchan.cc, wchansh.h, wchansh.cc, which contain the model.

#include "wchansh.h"

double angle (double, double, double, double);
Long initChannel ();

inline double adiff (double a1, double a2) {
	// Absolute angle difference between 0 and PI
	double da = fabs (a1 - a2);
	if (da > M_PI)
		da -= M_PI;
	return da;
}

inline double tag_to_angle (IPointer tag) {

	// Extract IPointer as float (for angle extraction from Tag)

	return *((float*)&tag);
}

inline IPointer angle_to_tag (float ang) {

	// Store float as IPointer (for angle storage in Tag)

	return *((IPointer*)&ang);
}
	
#endif
