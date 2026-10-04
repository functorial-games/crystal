# Crystal

Interactive crystal growth on a fixed-budget geometric net.

Crystal reuses the useful separation from `flower`: topology, material state, camera/input state and drawing buffers are distinct. The game does **not** treat rendering nodes as atoms. A fixed or nearly fixed set of surface nodes samples a growing shape; material-specific local rules determine which directions, angles and facets are preferred.

The first draft material laws live in `materials/`, with the common model in `docs/net-model.md`.

Initial materials:

- alpha quartz — trigonal, chiral;
- halite — cubic;
- bismuth — rhombohedral A7, with an optional non-equilibrium hopper/terrace rule;
- pyrite — cubic/diploidal, with cube/pyritohedron/octahedron face families.

The crystallographic labels are anchors for symmetry. Numerical gameplay weights are deliberately marked as uncalibrated until they are backed by measurement or a growth model.
