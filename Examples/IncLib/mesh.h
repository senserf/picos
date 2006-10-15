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
