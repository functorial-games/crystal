# Functorial C repair / qualification — 2026-10-06

**STRUCTURAL REPAIR REQUIRED:** this active branch is a static material-template
viewer. It does not implement the constitutive growth model in `net-model.md`.
ICK application compilation is separately blocked on Bionic nullability and
Android availability annotations, actually reproduced at API 21.

`CrystalNet` now owns stable node count and facet incidence; `CrystalEmbedding`
owns positions. `CrystalSurface` composes those distinct values. Material
templates, incidence validation, triangulation and net-edge derivation live in
`crystal_model.c`. The model header exposes those useful constructions.

`crystal_render.c` derives raylib vertex/normal buffers from a const surface and
draws derived edges. Camera/diagnostic state and Lua appearance policy stay in
the native host. All repository-owned executable functions are snake_case;
raylib and Lua's externally defined APIs retain their mandated spellings.
Private node/facet and ring-construction helpers are not exported.

Executed: pinned NDK/raylib/Lua host geometry and diagnostic tests, including
all three materials and bitwise unchanged surfaces after triangulation/edge
derivation. The maintained native producer recipe compiles the new translation
units, links them into each existing variant, and records all application
source digests. Existing producer admission, package, signer, ABI and SDK
checks were not removed. The protected producer was not executed here; native
tests are not an authenticated APK or accepted device run.

The missing mathematical decision is the constitutive map: the document gives
an illustrative angular blend, not a selected law and complete redistribution
state transition. The actual viewer has neither retained material coordinates
nor evaluated normal-frame growth fields. Its three material templates are
not a simulation of those laws. No empty growth wrappers or invented integration
model were added to obtain a uniform classification.

AICI, Cat Food, Kitchen, Flexible Pipes and Android-NDK were inspected for stage
ownership: **OUT OF SCOPE / NO DEFECT FOUND** for mathematical C restructuring
in this repair. Their existing protected producer/packaging boundaries remain
authoritative; unavailable admission/signing/device evidence was not bypassed.
