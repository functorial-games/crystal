#define CRYSTAL_HOST_TEST 1
#include "../app/src/main/cpp/crystal.c"
#include <assert.h>
int main(void) {
    CrystalNet net=BuildHaliteNet();
    CrystalTriangle triangles[MAX_TRIANGLES];
    CrystalEdge edges[MAX_NET_EDGES];
    int count=0;
    assert(ValidateNet(&net));
    assert(net.nodeCount==8 && net.facetCount==6);
    assert(NetToTriangles(&net,triangles,&count) && count==12);
    int edgeCount=CollectNetEdges(&net,edges);
    assert(edgeCount==12);
    int degree[8]={0};
    for (int i=0;i<edgeCount;i++) {
        int a=edges[i].a,b=edges[i].b;
        degree[a]++; degree[b]++;
        Vector3 p=net.nodes[a].position,q=net.nodes[b].position;
        int different=(p.x!=q.x)+(p.y!=q.y)+(p.z!=q.z);
        assert(different==1);
        assert(fabsf(Vector3Distance(p,q)-2.7f)<0.00001f);
        int incidence=0;
        for(int f=0;f<net.facetCount;f++) for(int v=0;v<4;v++) {
            int x=net.facets[f].node[v],y=net.facets[f].node[(v+1)%4];
            if ((x==a&&y==b)||(x==b&&y==a)) incidence++;
        }
        assert(incidence==2);
    }
    for (int i=0;i<8;i++) assert(degree[i]==3);
    CrystalNet bad=net; bad.facets[0].node[0]=999;
    assert(!ValidateNet(&bad) && CollectNetEdges(&bad,edges)==-1);
    bad=net; bad.nodes[0].position.x=NAN;
    assert(!ValidateNet(&bad) && !NetToTriangles(&bad,triangles,&count));
    /* Valid geometry plus a bad transform remains valid CPU geometry. A wrong
     * image alone cannot distinguish this from the invalid-index mutant. */
    Matrix wrong=MatrixTranslate(10000.0f,0,0);
    Vector3 projected=Vector3Transform(net.nodes[0].position,wrong);
    assert(ValidateNet(&net) && projected.x>9990.0f);
    return 0;
}
