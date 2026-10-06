#ifndef CRYSTAL_DIAGNOSTIC_STATE_H
#define CRYSTAL_DIAGNOSTIC_STATE_H
#include <math.h>
#include <stdbool.h>

typedef struct {
    float distance, minimum, maximum, requested, span, applied;
    int first, second, touches;
    bool pinching, minimum_hit, maximum_hit;
} CrystalZoom;
static inline void crystal_zoom_reset(CrystalZoom *z) {
    *z = (CrystalZoom){.distance=7.65f, .minimum=2.6f, .maximum=18.0f,
        .requested=7.65f, .applied=1.0f, .first=-1, .second=-1};
}
static inline void crystal_zoom_apply(CrystalZoom *z, float requested) {
    if (!isfinite(requested) || requested <= 0.0f) return;
    float before = z->distance;
    z->requested=requested;
    z->minimum_hit=requested < z->minimum;
    z->maximum_hit=requested > z->maximum;
    z->distance=fminf(z->maximum, fmaxf(z->minimum, requested));
    z->applied=before/z->distance;
}
static inline void crystal_zoom_touches(CrystalZoom *z, int count, int first, int second, float span) {
    /* Pointer identity, rather than array order, establishes continuity. */
    if (first > second) { int temporary=first; first=second; second=temporary; }
    z->touches=count;
    if (count != 2 || !isfinite(span) || span <= 1.0f) {
        z->pinching=false; z->span=0.0f; z->first=-1; z->second=-1;
        z->applied=1.0f; return;
    }
    if (z->pinching && z->first==first && z->second==second && z->span>1.0f)
        crystal_zoom_apply(z, z->distance*z->span/span);
    else z->applied=1.0f;
    z->first=first; z->second=second; z->span=span; z->pinching=true;
}
#endif
