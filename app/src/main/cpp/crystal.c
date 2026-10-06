#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#ifndef CRYSTAL_KIND
#define CRYSTAL_KIND 0
#endif

#ifndef CRYSTAL_TITLE
#define CRYSTAL_TITLE "Crystal"
#endif

#ifndef CRYSTAL_EMULATOR_TEST
#define CRYSTAL_EMULATOR_TEST 0
#endif

#define MAX_NET_NODES 160
#define MAX_NET_FACETS 192
#define MAX_FACET_NODES 4
#define MAX_TRIANGLES 512
#define MAX_NET_EDGES (MAX_NET_FACETS * MAX_FACET_NODES)

typedef struct {
    Vector3 position;
} CrystalNode;

typedef struct {
    int count;
    int node[MAX_FACET_NODES];
} CrystalFacet;

typedef struct {
    CrystalNode nodes[MAX_NET_NODES];
    int nodeCount;
    CrystalFacet facets[MAX_NET_FACETS];
    int facetCount;
} CrystalNet;

typedef struct {
    Vector3 a;
    Vector3 b;
    Vector3 c;
} CrystalTriangle;

typedef struct {
    int a;
    int b;
} CrystalEdge;

static Color gTint = { 220, 232, 245, 255 };
static float gAutoSpin = 0.0f;

static int NetAddNode(CrystalNet *net, float x, float y, float z) {
    if (net->nodeCount >= MAX_NET_NODES) return -1;
    int id = net->nodeCount++;
    net->nodes[id].position = (Vector3){ x, y, z };
    return id;
}

static bool NetAddFacet3(CrystalNet *net, int a, int b, int c) {
    if (net->facetCount >= MAX_NET_FACETS) return false;
    CrystalFacet *f = &net->facets[net->facetCount++];
    f->count = 3;
    f->node[0] = a; f->node[1] = b; f->node[2] = c;
    return true;
}

static bool NetAddFacet4(CrystalNet *net, int a, int b, int c, int d) {
    if (net->facetCount >= MAX_NET_FACETS) return false;
    CrystalFacet *f = &net->facets[net->facetCount++];
    f->count = 4;
    f->node[0] = a; f->node[1] = b; f->node[2] = c; f->node[3] = d;
    return true;
}

static CrystalNet BuildHaliteNet(void) {
    CrystalNet net = {0};
    const float s = 1.35f;
    int n[8];
    n[0] = NetAddNode(&net, -s, -s, -s);
    n[1] = NetAddNode(&net,  s, -s, -s);
    n[2] = NetAddNode(&net,  s,  s, -s);
    n[3] = NetAddNode(&net, -s,  s, -s);
    n[4] = NetAddNode(&net, -s, -s,  s);
    n[5] = NetAddNode(&net,  s, -s,  s);
    n[6] = NetAddNode(&net,  s,  s,  s);
    n[7] = NetAddNode(&net, -s,  s,  s);

    NetAddFacet4(&net, n[0], n[1], n[2], n[3]);
    NetAddFacet4(&net, n[4], n[7], n[6], n[5]);
    NetAddFacet4(&net, n[0], n[4], n[5], n[1]);
    NetAddFacet4(&net, n[1], n[5], n[6], n[2]);
    NetAddFacet4(&net, n[2], n[6], n[7], n[3]);
    NetAddFacet4(&net, n[3], n[7], n[4], n[0]);
    return net;
}

static CrystalNet BuildQuartzNet(void) {
    CrystalNet net = {0};
    int upper[6], lower[6];
    const float radius = 1.25f;
    const float yUpper = 0.85f;
    const float yLower = -0.85f;

    for (int i = 0; i < 6; ++i) {
        float angle = (float)i * (2.0f*PI/6.0f) + PI/6.0f;
        float x = radius*cosf(angle);
        float z = radius*sinf(angle);
        upper[i] = NetAddNode(&net, x, yUpper, z);
        lower[i] = NetAddNode(&net, x, yLower, z);
    }

    int top = NetAddNode(&net, 0.0f, 1.85f, 0.0f);
    int bottom = NetAddNode(&net, 0.0f, -1.85f, 0.0f);

    for (int i = 0; i < 6; ++i) {
        int j = (i + 1) % 6;
        NetAddFacet4(&net, lower[i], lower[j], upper[j], upper[i]);
        NetAddFacet3(&net, upper[i], upper[j], top);
        NetAddFacet3(&net, lower[j], lower[i], bottom);
    }
    return net;
}

static void AddSquareRing(CrystalNet *net, float half, float y, int out[4]) {
    out[0] = NetAddNode(net, -half, y, -half);
    out[1] = NetAddNode(net,  half, y, -half);
    out[2] = NetAddNode(net,  half, y,  half);
    out[3] = NetAddNode(net, -half, y,  half);
}

static void AddRingBand(CrystalNet *net, const int outer[4], const int inner[4]) {
    for (int i = 0; i < 4; ++i) {
        int j = (i + 1) % 4;
        NetAddFacet4(net, outer[i], outer[j], inner[j], inner[i]);
    }
}

static CrystalNet BuildBismuthNet(void) {
    CrystalNet net = {0};
    const int levels = 6;
    const float startHalf = 1.70f;
    const float step = 0.22f;
    const float drop = 0.22f;

    int outer[4];
    AddSquareRing(&net, startHalf, 0.78f, outer);

    int outsideBottom[4];
    AddSquareRing(&net, startHalf, -0.95f, outsideBottom);
    for (int i = 0; i < 4; ++i) {
        int j = (i + 1) % 4;
        NetAddFacet4(&net, outsideBottom[i], outsideBottom[j], outer[j], outer[i]);
    }
    NetAddFacet4(&net, outsideBottom[3], outsideBottom[2], outsideBottom[1], outsideBottom[0]);

    float currentY = 0.78f;
    float currentHalf = startHalf;

    for (int level = 0; level < levels; ++level) {
        float innerHalf = currentHalf - step;
        int innerTop[4];
        AddSquareRing(&net, innerHalf, currentY, innerTop);
        AddRingBand(&net, outer, innerTop);

        float nextY = currentY - drop;
        int innerBottom[4];
        AddSquareRing(&net, innerHalf, nextY, innerBottom);
        for (int i = 0; i < 4; ++i) {
            int j = (i + 1) % 4;
            NetAddFacet4(&net, innerTop[i], innerTop[j], innerBottom[j], innerBottom[i]);
        }

        memcpy(outer, innerBottom, sizeof(outer));
        currentHalf = innerHalf;
        currentY = nextY;
    }

    NetAddFacet4(&net, outer[0], outer[1], outer[2], outer[3]);
    return net;
}

static bool NetToTriangles(const CrystalNet *net, CrystalTriangle out[MAX_TRIANGLES], int *triangleCount) {
    int count = 0;
    for (int f = 0; f < net->facetCount; ++f) {
        const CrystalFacet *facet = &net->facets[f];
        if (facet->count < 3 || facet->count > MAX_FACET_NODES) return false;
        for (int i = 0; i < facet->count; ++i) {
            if (facet->node[i] < 0 || facet->node[i] >= net->nodeCount) return false;
        }
        for (int i = 1; i + 1 < facet->count; ++i) {
            if (count >= MAX_TRIANGLES) return false;
            out[count++] = (CrystalTriangle){
                net->nodes[facet->node[0]].position,
                net->nodes[facet->node[i]].position,
                net->nodes[facet->node[i + 1]].position
            };
        }
    }
    *triangleCount = count;
    return true;
}

static Mesh BuildRaylibMesh(const CrystalNet *net) {
    CrystalTriangle triangles[MAX_TRIANGLES];
    int triangleCount = 0;
    Mesh mesh = {0};

    if (!NetToTriangles(net, triangles, &triangleCount) || triangleCount <= 0) {
        TraceLog(LOG_ERROR, "Crystal: net triangulation failed");
        return mesh;
    }

    mesh.triangleCount = triangleCount;
    mesh.vertexCount = triangleCount * 3;
    mesh.vertices = (float *)MemAlloc((size_t)mesh.vertexCount * 3u * sizeof(float));
    mesh.normals = (float *)MemAlloc((size_t)mesh.vertexCount * 3u * sizeof(float));

    for (int t = 0; t < triangleCount; ++t) {
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

static int CollectNetEdges(const CrystalNet *net, CrystalEdge out[MAX_NET_EDGES]) {
    int edgeCount = 0;

    for (int f = 0; f < net->facetCount; ++f) {
        const CrystalFacet *facet = &net->facets[f];
        for (int i = 0; i < facet->count; ++i) {
            int a = facet->node[i];
            int b = facet->node[(i + 1) % facet->count];
            if (a > b) {
                int tmp = a;
                a = b;
                b = tmp;
            }

            bool duplicate = false;
            for (int e = 0; e < edgeCount; ++e) {
                if (out[e].a == a && out[e].b == b) {
                    duplicate = true;
                    break;
                }
            }

            if (!duplicate && edgeCount < MAX_NET_EDGES) {
                out[edgeCount++] = (CrystalEdge){ a, b };
            }
        }
    }

    return edgeCount;
}

static void DrawNetEdges(const CrystalNet *net, const CrystalEdge *edges, int edgeCount,
                         Matrix transform, Color color) {
    for (int e = 0; e < edgeCount; ++e) {
        Vector3 a = Vector3Transform(net->nodes[edges[e].a].position, transform);
        Vector3 b = Vector3Transform(net->nodes[edges[e].b].position, transform);
        DrawLine3D(a, b, color);
    }
}

static int LuaSetColor(lua_State *L) {
    double r = luaL_checknumber(L, 1);
    double g = luaL_checknumber(L, 2);
    double b = luaL_checknumber(L, 3);
    double a = luaL_optnumber(L, 4, 1.0);
    if (r < 0) r = 0; if (r > 1) r = 1;
    if (g < 0) g = 0; if (g > 1) g = 1;
    if (b < 0) b = 0; if (b > 1) b = 1;
    if (a < 0) a = 0; if (a > 1) a = 1;
    gTint = (Color){
        (unsigned char)(r*255.0),
        (unsigned char)(g*255.0),
        (unsigned char)(b*255.0),
        (unsigned char)(a*255.0)
    };
    return 0;
}

static int LuaSetSpin(lua_State *L) {
    gAutoSpin = (float)luaL_checknumber(L, 1);
    return 0;
}

static void ConfigureWithLua(void) {
    lua_State *L = luaL_newstate();
    if (!L) return;
    luaL_openlibs(L);

    lua_newtable(L);
    lua_pushcfunction(L, LuaSetColor);
    lua_setfield(L, -2, "set_color");
    lua_pushcfunction(L, LuaSetSpin);
    lua_setfield(L, -2, "set_auto_spin");
    lua_setglobal(L, "crystal");

#if CRYSTAL_KIND == 0
    const char *script =
        "crystal.set_color(0.72, 0.88, 1.00, 1.0)\n"
        "crystal.set_auto_spin(0.10)\n";
#elif CRYSTAL_KIND == 1
    const char *script =
        "crystal.set_color(0.82, 0.72, 0.96, 1.0)\n"
        "crystal.set_auto_spin(0.08)\n";
#else
    const char *script =
        "crystal.set_color(0.88, 0.55, 0.30, 1.0)\n"
        "crystal.set_auto_spin(0.06)\n";
#endif

    if (luaL_dostring(L, script) != LUA_OK) {
        TraceLog(LOG_WARNING, "Crystal Lua config failed: %s", lua_tostring(L, -1));
    }
    lua_close(L);
}

static CrystalNet BuildMaterialNet(void) {
#if CRYSTAL_KIND == 0
    return BuildHaliteNet();
#elif CRYSTAL_KIND == 1
    return BuildQuartzNet();
#else
    return BuildBismuthNet();
#endif
}

int main(void) {
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(576, 1152, CRYSTAL_TITLE);
    SetTargetFPS(60);

    ConfigureWithLua();

#if CRYSTAL_EMULATOR_TEST
    gAutoSpin = 0.0f;
    TraceLog(LOG_INFO, "Crystal: emulator diagnostics enabled");
#endif

    CrystalNet net = BuildMaterialNet();
    Mesh mesh = BuildRaylibMesh(&net);
    Model model = LoadModelFromMesh(mesh);
    CrystalEdge edges[MAX_NET_EDGES];
    int edgeCount = CollectNetEdges(&net, edges);
    TraceLog(LOG_INFO, "Crystal: kind=%d nodes=%d facets=%d edges=%d",
             CRYSTAL_KIND, net.nodeCount, net.facetCount, edgeCount);

    Camera3D camera = {0};
    camera.position = (Vector3){ 4.3f, 3.2f, 5.4f };
    camera.target = (Vector3){ 0.0f, -0.15f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 43.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    float pitch = -0.18f;
    float yaw = 0.55f;
    int renderMode = 2;  // 0 = solid, 1 = net edges, 2 = both
    bool pointerWasDown = false;
    bool pointerConsumed = false;
    Vector2 previousPointer = {0};
    bool pinching = false;
    float previousPinchDistance = 0.0f;
    Vector3 cameraDirection = Vector3Normalize(Vector3Subtract(camera.position, camera.target));
    float cameraDistance = Vector3Distance(camera.position, camera.target);

    while (!WindowShouldClose()) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        Rectangle resetRect = { 20.0f, (float)sh - 92.0f, 150.0f, 58.0f };
        Rectangle wireRect = { (float)sw - 170.0f, (float)sh - 92.0f, 150.0f, 58.0f };

        int touchCount = GetTouchPointCount();
        bool pointerDown = false;
        Vector2 pointer = {0};

        if (touchCount >= 2) {
            Vector2 firstTouch = GetTouchPosition(0);
            Vector2 secondTouch = GetTouchPosition(1);
            float pinchDistance = Vector2Distance(firstTouch, secondTouch);

            if (pinching && previousPinchDistance > 1.0f && pinchDistance > 1.0f) {
                float ratio = pinchDistance / previousPinchDistance;
                cameraDistance = Clamp(cameraDistance / ratio, 2.6f, 10.0f);
            }

            previousPinchDistance = pinchDistance;
            pinching = true;
            pointerWasDown = false;
            pointerConsumed = false;
        } else {
            if (pinching) {
                pinching = false;
                previousPinchDistance = 0.0f;
                pointerWasDown = false;
                pointerConsumed = false;
            }

            if (touchCount == 1) {
                pointerDown = true;
                pointer = GetTouchPosition(0);
            } else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                pointerDown = true;
                pointer = GetMousePosition();
            }

            if (pointerDown && !pointerWasDown) {
                pointerConsumed = false;
                if (CheckCollisionPointRec(pointer, resetRect)) {
                    pitch = -0.18f;
                    yaw = 0.55f;
                    cameraDistance = Vector3Distance((Vector3){ 4.3f, 3.2f, 5.4f }, camera.target);
                    pointerConsumed = true;
                } else if (CheckCollisionPointRec(pointer, wireRect)) {
                    renderMode = (renderMode + 1) % 3;
                    pointerConsumed = true;
                }
                previousPointer = pointer;
            } else if (pointerDown && pointerWasDown && !pointerConsumed) {
                Vector2 delta = Vector2Subtract(pointer, previousPointer);
                yaw += delta.x * 0.010f;
                pitch += delta.y * 0.010f;
                pitch = Clamp(pitch, -1.45f, 1.45f);
                previousPointer = pointer;
            }

            pointerWasDown = pointerDown;
            if (!pointerDown) pointerConsumed = false;
        }

        yaw += gAutoSpin * GetFrameTime();
        model.transform = MatrixMultiply(MatrixRotateX(pitch), MatrixRotateY(yaw));
        camera.position = Vector3Add(camera.target, Vector3Scale(cameraDirection, cameraDistance));

        BeginDrawing();
        ClearBackground((Color){ 18, 20, 25, 255 });

        BeginMode3D(camera);
        DrawGrid(12, 0.5f);
        if (renderMode != 1) {
            rlDisableBackfaceCulling();
            DrawModel(model, Vector3Zero(), 1.0f, gTint);
            rlEnableBackfaceCulling();
        }
        if (renderMode != 0) {
            if (renderMode == 2) rlDisableDepthTest();
            rlSetLineWidth(2.0f);
            DrawNetEdges(&net, edges, edgeCount, model.transform, (Color){ 245, 248, 252, 255 });
            rlSetLineWidth(1.0f);
            if (renderMode == 2) rlEnableDepthTest();
        }
        EndMode3D();

        DrawRectangle(0, 0, sw, 82, Fade(BLACK, 0.55f));
        DrawText(CRYSTAL_TITLE, 20, 18, 28, RAYWHITE);
        DrawText("C + Lua + raylib visual prototype", 20, 50, 16, LIGHTGRAY);

        DrawRectangleRec(resetRect, Fade(DARKGRAY, 0.92f));
        DrawRectangleLinesEx(resetRect, 2.0f, GRAY);
        DrawText("RESET", (int)resetRect.x + 31, (int)resetRect.y + 18, 20, RAYWHITE);

        DrawRectangleRec(wireRect, Fade(DARKGRAY, 0.92f));
        DrawRectangleLinesEx(wireRect, 2.0f, GRAY);
        const char *viewLabel = (renderMode == 0) ? "SOLID" : (renderMode == 1) ? "NET" : "BOTH";
        DrawText(viewLabel, (int)wireRect.x + 42, (int)wireRect.y + 18, 18, RAYWHITE);

        DrawText("drag: rotate   pinch: zoom", 20, sh - 126, 17, LIGHTGRAY);

#if CRYSTAL_EMULATOR_TEST
        DrawRectangle(0, 88, sw, 94, Fade(BLACK, 0.60f));
        DrawText(TextFormat("CI diag: kind=%d nodes=%d facets=%d edges=%d",
                            CRYSTAL_KIND, net.nodeCount, net.facetCount, edgeCount),
                 20, 96, 16, LIME);
        DrawText(TextFormat("mode=%s yaw=%.3f pitch=%.3f dist=%.3f",
                            viewLabel, yaw, pitch, cameraDistance),
                 20, 122, 16, LIME);
        DrawText(TextFormat("screen=%dx%d projection=perspective", sw, sh),
                 20, 148, 16, LIME);
#endif

        EndDrawing();
    }

    UnloadModel(model);
    CloseWindow();
    return 0;
}
