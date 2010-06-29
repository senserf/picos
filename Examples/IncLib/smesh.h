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
