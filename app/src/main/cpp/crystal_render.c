#include "crystal_render.h"
#include "raymath.h"
Mesh build_raylib_mesh(const CrystalSurface *net) {
    CrystalTriangle triangles[MAX_TRIANGLES];
    int triangle_count = 0;
    Mesh mesh = {0};

    if (!triangulate_surface(net, triangles, &triangle_count) || triangle_count <= 0) {
        TraceLog(LOG_ERROR, "Crystal: net triangulation failed");
        return mesh;
    }

    mesh.triangle_count = triangle_count;
    mesh.vertexCount = triangle_count * 3;
    mesh.vertices = (float *)MemAlloc((size_t)mesh.vertexCount * 3u * sizeof(float));
    mesh.normals = (float *)MemAlloc((size_t)mesh.vertexCount * 3u * sizeof(float));

    for (int t = 0; t < triangle_count; ++t) {
        Vector3 p[3] = { triangles[t].a, triangles[t].b, triangles[t].c };
        Vector3 ab = Vector3Subtract(p[1], p[0]);
        Vector3 ac = Vector3Subtract(p[2], p[0]);
        Vector3 normal = Vector3Normalize(Vector3CrossProduct(ab, ac));

        for (int v = 0; v < 3; ++v) {
            int base = (t*3 + v)*3;
            mesh.vertices[base + 0] = p[v].x;
            mesh.vertices[base + 1] = p[v].y;
            mesh.vertices[base + 2] = p[v].z;
            mesh.normals[base + 0] = normal.x;
            mesh.normals[base + 1] = normal.y;
            mesh.normals[base + 2] = normal.z;
        }
    }

    UploadMesh(&mesh, false);
    return mesh;
}

void draw_net_edges(const CrystalSurface *net, const CrystalEdge *edges, int edge_count,
                         Matrix transform, Color color) {
    for (int e = 0; e < edge_count; ++e) {
        Vector3 a = Vector3Transform(net->embedding.nodes[edges[e].a].position, transform);
        Vector3 b = Vector3Transform(net->embedding.nodes[edges[e].b].position, transform);
        DrawLine3D(a, b, color);
    }
}

