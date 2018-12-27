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

#ifndef __smesh_h__
#define __smesh_h__

// This file contains type definition for a station with the same number of
// input and output ports configurable into meshes, e.g., deflection networks
// flood networks, etc. This type is used in defining MNA switches.

// In contrast to mesh.h/mesh.cc, this topology is symmetric in the sense
// that a direct link from A to B implies a direct link from B to A. The
// incoming and outgoing port numbers to the same neighbor are the same.

station MeshNode virtual {
  Port **IPorts, **OPorts;
  Long *Neighbors;
  int Order;
  void configure (int);
};

typedef int (*CFType) (Long, Long, DISTANCE&);

void initMesh (RATE, CFType, Long);

#endif
