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

#ifndef __mesh_c__
#define __mesh_c__

// This file contains code for configuring meshes, i.e., switched networks
// in which stations are interconnected by point-to-point unidirectional
// channels. It is assumed that each station has the same number of input 
// and output ports.

static CFType Connectivity;   // User-supplied connectivity function
static Long NP,               // The number of nodes
            NC;               // The number of nodes built so far
static RATE TR;               // Transmission rate (per output port)

static MeshNode **Nodes;

void initMesh (RATE r, CFType cn, Long np) {
  TR = r;
  NP = np;
  NC = 0;
  Connectivity = cn;
  Nodes = new MeshNode* [NP];
};

static void buildMesh (), checkConnectivity ();

void MeshNode::configure (int order) {
  int i;
  Order = order;
  IPorts = new Port* [Order];
  OPorts = new Port* [Order];
  Neighbors = new Long [Order];
  BackNeighbors = new Long [Order];
  for (i = 0; i < Order; i++) {
    IPorts [i] = NULL;  // Flag: unallocated yet
    OPorts [i] = NULL;
    Neighbors [i] = BackNeighbors [i] = NONE;
  }
  Nodes [NC++] = this;
  if (NC == NP) {
    buildMesh ();
    checkConnectivity ();
    delete Nodes;
    TheStation = idToStation (NP-1);
  }
};

void buildMesh () {
  Long i, j, nc;
  int fp1, fp2;
  MeshNode *S1, *S2;
  DISTANCE lg;
  PLink *lk;
  Port *p1, *p2;
  for (i = 0; i < NP; i++) {
    S1 = Nodes [i];
    for (j = 0; j < NP; j++) {
      // Note that in some mesh networks (e.g., MNA) it is legal to connect
      // a node to itself. Thus the diagonal is not explicitly excluded. It
      // should be handled prperly by the user-supplied connectivity
      // function.
      for (nc = Connectivity (i, j, lg); nc; nc--) {
        // There may be multiple connections between the same nodes
        S2 = Nodes [j];
        lk = create PLink (2);
        for (fp1 = 0; fp1 < S1->Order; fp1++)
          // Find the first free output port at S1
          if (S1->OPorts [fp1] == NULL) break;
        Assert (fp1 < S1->Order,
          form ("Station %1d: no more output ports", i));
        for (fp2 = 0; fp2 < S2->Order; fp2++)
          // Find the first free input port at S2
          if (S2->IPorts [fp2] == NULL) break;
        Assert (fp2 < S2->Order,
          form ("Station %1d: no more input ports", j));
        // Create the ports
        p1 = S1->OPorts [fp1] = create (i) Port (TR);
        p2 = S2->IPorts [fp2] = create (j) Port;
        p1 -> connect (lk);
        p2 -> connect (lk);
        p1 -> setDTo (p2, lg);
        S1 -> Neighbors [fp1] = j;
	S2 -> BackNeighbors [fp2] = i;
      }
    }
  }
};
         
void checkConnectivity () {
  // Checks whether there are no dangling ports
  Long i;
  int p;
  MeshNode *S;
  for (i = 0; i < NP; i++) {
    S = Nodes [i];
    for (p = 0; p < S->Order; p++) {
      Assert (S->IPorts [p] != NULL,
        form ("Station %1d, input port %1d left dangling", i, p));
      Assert (S->OPorts [p] != NULL,
        form ("Station %1d, output port %1d left dangling", i, p));
      Assert (S->Neighbors [p] != NONE,
        form ("Station %1d, output port %1d - no neighbor", i, p));
      Assert (S->BackNeighbors [p] != NONE,
        form ("Station %1d, output port %1d - no back neighbor", i, p));
    }
  }
};

#endif
