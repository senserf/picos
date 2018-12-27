/*
	Copyright 1995-2018, 2019 Pawel Gburzynski

	This file is part of SMURPH/SIDE.

	SMURPH/SIDE is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	SMURPH/SIDE is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SMURPH/SIDE. If not, see <https://www.gnu.org/licenses/>.
*/

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
