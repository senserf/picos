#ifndef	__channel_h__
#define	__channel_h__

// Shadowing channel model with directional hooks. Look into Examples/IncLib
// for files wchan.h, wchan.cc, wchansh.h, wchansh.cc, which contain the model.

#include "angles.h"
#include "wchansh.h"

double angle (double, double, double, double);
Long initChannel ();

inline double tag_to_angle (IPointer tag) {

	// Extract IPointer as float (for angle extraction from Tag)

	return *((float*)&tag);
}

inline IPointer angle_to_tag (float ang) {

	// Store float as IPointer (for angle storage in Tag)

	return *((IPointer*)&ang);
}

#endif
