#ifndef CRYSTAL_MODEL_H
#define CRYSTAL_MODEL_H
#include <stdbool.h>
#include "raylib.h"
#define MAX_NET_NODES 160
#define MAX_NET_FACETS 192
#define MAX_FACET_NODES 4
#define MAX_TRIANGLES 512
#define MAX_NET_EDGES (MAX_NET_FACETS * MAX_FACET_NODES)
typedef struct { Vector3 position; } CrystalNode;
typedef struct { int count; int node[MAX_FACET_NODES]; } CrystalFacet;
/* Incidence has no positions; rendering cannot redefine node identity. */
typedef struct { int node_count; CrystalFacet facets[MAX_NET_FACETS]; int facet_count; } CrystalNet;
typedef struct { CrystalNode nodes[MAX_NET_NODES]; } CrystalEmbedding;
typedef struct { CrystalNet net; CrystalEmbedding embedding; } CrystalSurface;
typedef struct { Vector3 a,b,c; } CrystalTriangle;
typedef struct { int a,b; } CrystalEdge;
bool validate_surface(const CrystalSurface *surface);
CrystalSurface build_halite_surface(void);
CrystalSurface build_quartz_surface(void);
CrystalSurface build_bismuth_surface(void);
bool triangulate_surface(const CrystalSurface *surface, CrystalTriangle out[MAX_TRIANGLES], int *count);
int collect_net_edges(const CrystalSurface *surface,CrystalEdge out[MAX_NET_EDGES]);
#endif
