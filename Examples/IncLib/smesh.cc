#ifndef __smesh_c__
#define __smesh_c__

// This file contains code for configuring meshes, i.e., switched networks
// in which stations are interconnected by point-to-point unidirectional
// channels. It is assumed that each station has the same number of input 
// and output ports.

// In contrast to mesh.h/mesh.cc, this topology is symmetric in the sense
// that a direct link from A to B implies a direct link from B to A. The
// incoming and outgoing port numbers to the same neighbor are the same.

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
  for (i = 0; i < Order; i++) {
    IPorts [i] = NULL;  // Flag: unallocated yet
    OPorts [i] = NULL;
    Neighbors [i] = NONE;
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
  PLink *lkf, *lkb;
  Port *po1, *pi1, *po2, *pi2;
  for (i = 0; i < NP; i++) {
    S1 = Nodes [i];
    for (j = i; j < NP; j++) {
      // Note that in some mesh networks (e.g., MNA) it is legal to connect
      // a node to itself. Thus the diagonal is not explicitly excluded. It
      // should be handled prperly by the user-supplied connectivity
      // function.
      for (nc = Connectivity (i, j, lg); nc; nc--) {
        // There may be multiple connections between the same nodes
        S2 = Nodes [j];
	// Forward link from S1 to S2
        lkf = create PLink (2);
	// Backward link from S2 to S1
        lkb = create PLink (2);
        for (fp1 = 0; fp1 < S1->Order; fp1++)
          // Find the first free pair of ports at S1
          if (S1->OPorts [fp1] == NULL) break;
        Assert (fp1 < S1->Order,
          form ("Station %1d: no more ports", i));
        for (fp2 = 0; fp2 < S2->Order; fp2++)
          // Find the first free pair of ports at S2
          if (S2->IPorts [fp2] == NULL) break;
        Assert (fp2 < S2->Order,
          form ("Station %1d: no more ports", j));
        // Create the ports
        po1 = S1->OPorts [fp1] = create (i) Port (TR);
        po2 = S2->OPorts [fp2] = create (j) Port (TR);
        pi1 = S1->IPorts [fp1] = create (i) Port;
        pi2 = S2->IPorts [fp2] = create (j) Port;
	po2 -> connect (lkf);
	pi1 -> connect (lkf);
	po1 -> connect (lkb);
	pi2 -> connect (lkb);
	po2 -> setDTo (pi1, lg);
	po1 -> setDTo (pi2, lg);
        S1 -> Neighbors [fp1] = j;
	S2 -> Neighbors [fp2] = i;
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
    }
  }
};

#endif
