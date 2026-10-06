#include <assert.h>
#include <math.h>
#include "../app/src/main/cpp/diagnostic_state.h"
int main(void) {
    CrystalZoom z;
    CrystalZoomReset(&z);
    CrystalZoomTouches(&z,1,7,-1,0);
    CrystalZoomTouches(&z,2,7,9,100);
    assert(fabsf(z.distance-7.65f)<0.001f);
    CrystalZoomTouches(&z,2,9,7,200);
    assert(fabsf(z.distance-3.825f)<0.001f);
    CrystalZoomTouches(&z,1,9,-1,0);
    CrystalZoomTouches(&z,2,9,12,300);
    assert(fabsf(z.distance-3.825f)<0.001f);
    CrystalZoomTouches(&z,2,9,12,0.5f);
    CrystalZoomTouches(&z,2,9,12,100);
    assert(fabsf(z.distance-3.825f)<0.001f);
    CrystalZoomTouches(&z,2,9,12,50);
    assert(fabsf(z.distance-7.65f)<0.001f);
    CrystalZoomApply(&z,1000);
    assert(z.distance==18.0f && z.maximum_hit);
    CrystalZoomApply(&z,0.01f);
    assert(z.distance==2.6f && z.minimum_hit);
    CrystalZoomApply(&z,NAN);
    assert(z.distance==2.6f);
    CrystalZoomReset(&z);
    assert(!z.pinching && z.distance==7.65f);
    /* Projection-size oracle independent of input-state implementation. At
     * fixed fovy and viewport, doubling depth halves projected size. */
    for (int width=576; width<=1152; width+=576) {
        float near_size=(float)width/(2.0f*tanf(43.0f*0.5f*0.01745329252f)*7.65f);
        float far_size=(float)width/(2.0f*tanf(43.0f*0.5f*0.01745329252f)*15.3f);
        assert(fabsf(far_size/near_size-0.5f)<0.0001f);
    }
    return 0;
}
