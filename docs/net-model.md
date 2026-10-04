# Fixed-budget crystal net

## Scope

This is a **mesoscopic surface model**, not an atom-by-atom crystal lattice.

The useful part inherited from `flower` is the separation between:

- a fixed-budget topological net;
- current embedded geometry;
- a material law;
- input/camera state;
- drawing buffers.

The rendering/simulation nodes sample the crystal surface. They are not Si, O, Na, Cl, Bi, Fe or S atoms.

## State

A first implementation can stay close to Flower's data model:

```c
typedef struct { float x, y, z; } CrystalPoint;

typedef struct {
    uint16_t first;
    uint16_t second;
    float rest_length;
    float stiffness;
} CrystalLink;

typedef struct {
    CrystalPoint position[CRYSTAL_VERTICES];
    CrystalPoint material_coordinate[CRYSTAL_VERTICES];
    uint16_t triangle[CRYSTAL_TRIANGLES * 3];
    CrystalLink link[CRYSTAL_MAX_EDGES];
    int link_count;
} CrystalNet;
```

Unlike Flower's open sheet, the default Crystal net should be a closed genus-zero triangular surface around a small seed.

The **node count is invariant during ordinary growth**. If sampling becomes poor, redistribute nodes tangentially and/or flip edges while preserving the surface topology and node budget. A flat facet should need few samples; edges, corners and terraces should get more.

## Material frame

Every crystal instance carries an orientation frame `R`. Triangle normals are transformed into that frame before the material law is evaluated:

```
normal_in_crystal = transpose(R) * normal_in_world
```

This makes material orientation part of state without baking it into the mesh.

## Growth law

For the first prototype, use kinetic faceting rather than an atomistic simulation.

Each material defines a direction-dependent outward speed

```
v = v(normal_in_crystal)
```

and optional local penalties for link/angle distortion. Slow-growing normal families persist as visible facets; faster directions disappear from the exterior.

A simple draft law is:

```
v(n) = baseline_speed * min_family blend(relative_speed[family],
                                          baseline_speed,
                                          angular_distance(n, family))
```

where `family` means the full symmetry orbit of one crystallographic face family.

The numeric speeds in `materials/*.toml` are **gameplay seeds, not measured growth rates**. Their purpose is to make the correct symmetry compete visibly. Replace them when we have a calibrated kinetic or surface-energy model.

## Local relaxation

One simulation step can be split into:

1. compute triangle normals;
2. evaluate the material growth speed on each triangle;
3. accumulate outward displacement to vertices;
4. relax links and local angles;
5. redistribute nodes tangentially toward curvature, edges and terraces;
6. optionally flip edges if triangle quality becomes poor;
7. never create or destroy nodes during ordinary growth.

This gives Crystal the same architectural idea as Flower while changing the constitutive law.

## Draft material-net vocabulary

Each `materials/*.toml` file records:

- crystallographic system, point group and space group as provenance;
- an optional real unit-cell ratio as a symmetry/shape reference;
- a small set of face families;
- **relative gameplay growth speeds** for those families;
- optional local rules such as chirality, terrace retention or striation;
- source links for the crystallographic facts.

The material files do not claim that the game net is the atomic lattice.

## Four useful first nets

### Halite: orthogonal/cubic control case

Local directional classes are the three Cartesian axes and their opposites:

```
             +z
              |
              o
              |
        -y -- o -- +y
             / \
           -x   +x
              |
             -z
```

The important visible family is `{100}`; `{111}` provides an octahedral competitor. Halite is a good "does the symmetry engine work?" material.

### Alpha quartz: trigonal + handedness

Use a distinguished `c` axis and three equivalent transverse direction classes, with their opposites giving the familiar six-sided prism envelope. The material frame has **threefold**, not true sixfold, symmetry.

A chiral flag chooses one of the enantiomorphic quartz structures. At the mesoscopic level, chirality can initially enter as a signed coupling between successive neighborhoods around the `c` axis:

```
handedness = +1: phase(k + 1) prefers phase(k) + 120 degrees
handedness = -1: phase(k + 1) prefers phase(k) - 120 degrees
```

That is a game-scale chirality operator, not an atomic reconstruction.

The main face competition is prism `{10-10}` against the positive and negative rhombohedra `{10-11}` and `{01-11}`. Quartz's pseudohexagonal appearance therefore emerges from a genuinely trigonal law.

### Bismuth: distorted-cubic/A7 + terraces

Ambient bismuth has the rhombohedral A7 structure. A useful coarse local picture has three nearer neighbors and three slightly farther neighbors, producing a strong trigonal axis and puckered/bilayer character.

The game net can represent that as two link classes:

```
three strong directional preferences
three weaker, offset directional preferences
```

Synthetic bismuth's spectacular hopper shapes should be a **kinetic option**, not confused with the equilibrium A7 lattice. A hopper rule can make exposed edges advance more readily than face centers and preserve completed ledges long enough to produce nested terraces.

### Pyrite: cubic without full halite symmetry

Pyrite is isometric but has diploidal point symmetry rather than halite's full `m-3m`. Its common morphology gives a useful three-way competition:

- cube `{100}`;
- pyritohedron `{210}`;
- octahedron `{111}`.

That makes pyrite a good test that "cubic crystal" does not collapse to one generic cube preset.

## Crystallographic anchors

These are provenance anchors, not simulation calibrations.

- Alpha quartz: trigonal class 32; low quartz occurs in the enantiomorphic space groups P3_121 and P3_221; representative room-temperature cell values are about a=4.913 Å, c=5.405 Å. Quartz commonly combines hexagonal-prism and trigonal-rhombohedral forms.
  - https://www.mindat.org/min-3337.html
  - https://rruff.info/uploads/ZK184_257.pdf
- Halite: isometric, point group m-3m, space group Fm-3m; representative a=5.6404 Å; cubic habit is normal and octahedral habit occurs.
  - https://www.mindat.org/min-1804.html
- Bismuth: ambient A7/rhombohedral structure, space group R-3m (#166); representative hexagonal-setting values are about a=4.55 Å, c=11.85 Å. Three nearer and three farther neighbors are a useful structural description of A7 Bi.
  - https://www.metallurgy.nist.gov/phase/solder/agbi.html
  - https://www.mindat.org/min-684.html
- Pyrite: isometric/diploidal, space group Pa-3; representative a=5.417 Å; cube, pyritohedron and octahedron are common morphological forms.
  - https://www.mindat.org/min-3314.html
