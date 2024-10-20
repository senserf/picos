/*
	Copyright 1995-2020 Pawel Gburzynski

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

#ifndef __mesh_h__
#define __mesh_h__

// This file contains type definition for a station with the same number of
// input and output ports configurable into meshes, e.g., deflection networks
// flood networks, etc. This type is used in defining MNA switches.

station MeshNode virtual {
  Port **IPorts, **OPorts;
  Long *Neighbors, *BackNeighbors;
  int Order;
  void configure (int);
};

typedef int (*CFType) (Long, Long, DISTANCE&);

void initMesh (RATE, CFType, Long);

#endif
