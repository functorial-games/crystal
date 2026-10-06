#define CRYSTAL_HOST_TEST 1
#include "../app/src/main/cpp/crystal_model.h"
#include "raymath.h"
#include <assert.h>
#include <string.h>
int main(void) {
    CrystalSurface net=build_halite_surface();
    CrystalTriangle triangles[MAX_TRIANGLES];
    CrystalEdge edges[MAX_NET_EDGES];
    int count=0;
    assert(validate_surface(&net));
    assert(net.net.node_count==8 && net.net.facet_count==6);
    assert(triangulate_surface(&net,triangles,&count) && count==12);
    int edge_count=collect_net_edges(&net,edges);
    assert(edge_count==12);
    int degree[8]={0};
    for (int i=0;i<edge_count;i++) {
        int a=edges[i].a,b=edges[i].b;
        degree[a]++; degree[b]++;
        Vector3 p=net.embedding.nodes[a].position,q=net.embedding.nodes[b].position;
        int different=(p.x!=q.x)+(p.y!=q.y)+(p.z!=q.z);
        assert(different==1);
        assert(fabsf(Vector3Distance(p,q)-2.7f)<0.00001f);
        int incidence=0;
        for(int f=0;f<net.net.facet_count;f++) for(int v=0;v<4;v++) {
            int x=net.net.facets[f].node[v],y=net.net.facets[f].node[(v+1)%4];
            if ((x==a&&y==b)||(x==b&&y==a)) incidence++;
        }
        assert(incidence==2);
    }
    for (int i=0;i<8;i++) assert(degree[i]==3);
    CrystalSurface bad=net; bad.net.facets[0].node[0]=999;
    assert(!validate_surface(&bad) && collect_net_edges(&bad,edges)==-1);
    bad=net; bad.embedding.nodes[0].position.x=NAN;
    assert(!validate_surface(&bad) && !triangulate_surface(&bad,triangles,&count));
    /* Valid geometry plus a bad transform remains valid CPU geometry. A wrong
     * image alone cannot distinguish this from the invalid-index mutant. */
    Matrix wrong=MatrixTranslate(10000.0f,0,0);
    Vector3 projected=Vector3Transform(net.embedding.nodes[0].position,wrong);
    assert(validate_surface(&net) && projected.x>9990.0f);
    CrystalSurface materials[]={build_halite_surface(),build_quartz_surface(),build_bismuth_surface()};
    for (int material=0;material<3;material++) {
        CrystalSurface saved=materials[material];
        assert(validate_surface(&materials[material]));
        assert(triangulate_surface(&materials[material],triangles,&count));
        assert(collect_net_edges(&materials[material],edges)>0);
        assert(memcmp(&saved,&materials[material],sizeof(saved))==0);
    }
    return 0;
}
