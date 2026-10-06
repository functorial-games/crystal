#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <string.h>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "diagnostic_state.h"
#include "crystal_model.h"
#include "crystal_render.h"
#if defined(__ANDROID__)
#include <unistd.h>
#include <GLES2/gl2.h>
#include <android/log.h>
#endif

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
#ifndef CRYSTAL_SOURCE_COMMIT
#define CRYSTAL_SOURCE_COMMIT "UNKNOWN"
#endif
#ifndef CRYSTAL_PACKAGE_ID
#define CRYSTAL_PACKAGE_ID "UNKNOWN"
#endif

static Color tint = { 220, 232, 245, 255 };
static float auto_spin = 0.0f;

static int lua_set_color(lua_State *L) {
    double r = luaL_checknumber(L, 1);
    double g = luaL_checknumber(L, 2);
    double b = luaL_checknumber(L, 3);
    double a = luaL_optnumber(L, 4, 1.0);
    if (r < 0) r = 0; if (r > 1) r = 1;
    if (g < 0) g = 0; if (g > 1) g = 1;
    if (b < 0) b = 0; if (b > 1) b = 1;
    if (a < 0) a = 0; if (a > 1) a = 1;
    tint = (Color){
        (unsigned char)(r*255.0),
        (unsigned char)(g*255.0),
        (unsigned char)(b*255.0),
        (unsigned char)(a*255.0)
    };
    return 0;
}

static void json_safe(const char *input, char *output, size_t capacity) {
    size_t used=0;
    for (size_t i=0; input[i] && used+2<capacity; i++) {
        unsigned char c=(unsigned char)input[i];
        if (c=='"' || c=='\\') output[used++]='\\';
        output[used++]=(c<32)?'?':(char)c;
    }
    output[used]=0;
}

static void process_session(char *output, size_t capacity) {
    snprintf(output, capacity, "UNKNOWN");
#if defined(__ANDROID__)
    FILE *file=fopen("/proc/self/stat", "r");
    char stat[2048];
    if (file && fgets(stat, sizeof(stat), file)) {
        char *tail=strrchr(stat, ')');
        if (tail) {
            char *token=strtok(tail+1, " ");
            for (int field=3; token && field<22; field++) token=strtok(NULL, " ");
            if (token) snprintf(output, capacity, "%ld-%s", (long)getpid(), token);
        }
    }
    if (file) fclose(file);
#endif
}

static int lua_set_spin(lua_State *L) {
    auto_spin = (float)luaL_checknumber(L, 1);
    return 0;
}

static void configure_with_lua(void) {
    lua_State *L = luaL_newstate();
    if (!L) return;
    luaL_openlibs(L);

    lua_newtable(L);
    lua_pushcfunction(L, lua_set_color);
    lua_setfield(L, -2, "set_color");
    lua_pushcfunction(L, lua_set_spin);
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

static CrystalSurface build_material_surface(void) {
#if CRYSTAL_KIND == 0
    return build_halite_surface();
#elif CRYSTAL_KIND == 1
    return build_quartz_surface();
#else
    return build_bismuth_surface();
#endif
}

#if !defined(CRYSTAL_HOST_TEST)
int main(void) {
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(576, 1152, CRYSTAL_TITLE);
    SetTargetFPS(60);

    configure_with_lua();
    char launchSession[96];
    process_session(launchSession, sizeof(launchSession));
    rlSetClipPlanes(RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
    const char *glVendor="UNKNOWN", *glRenderer="UNKNOWN", *glVersion="UNKNOWN";
#if defined(__ANDROID__)
    glVendor=(const char *)glGetString(GL_VENDOR);
    glRenderer=(const char *)glGetString(GL_RENDERER);
    glVersion=(const char *)glGetString(GL_VERSION);
    if (!glVendor) glVendor="UNKNOWN";
    if (!glRenderer) glRenderer="UNKNOWN";
    if (!glVersion) glVersion="UNKNOWN";
#endif

    char safeVendor[256], safeRenderer[256], safeVersion[256];
    json_safe(glVendor, safeVendor, sizeof(safeVendor));
    json_safe(glRenderer, safeRenderer, sizeof(safeRenderer));
    json_safe(glVersion, safeVersion, sizeof(safeVersion));

#if CRYSTAL_EMULATOR_TEST
    auto_spin = 0.0f;
    TraceLog(LOG_INFO, "Crystal: emulator diagnostics enabled");
#endif

    CrystalSurface net = build_material_surface();
    Mesh mesh = build_raylib_mesh(&net);
    Model model = LoadModelFromMesh(mesh);
    CrystalEdge edges[MAX_NET_EDGES];
    int edge_count = collect_net_edges(&net, edges);
    if (edge_count < 0 || !validate_surface(&net)) {
        TraceLog(LOG_ERROR, "Crystal: invalid geometry; rendering withheld");
        UnloadModel(model); CloseWindow(); return 2;
    }
    TraceLog(LOG_INFO, "Crystal: kind=%d nodes=%d facets=%d edges=%d",
             CRYSTAL_KIND, net.net.node_count, net.net.facet_count, edge_count);

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
    CrystalZoom zoom;
    crystal_zoom_reset(&zoom);
    bool frozen = CRYSTAL_EMULATOR_TEST;
    bool showGrid = true;
    bool xrayEdges = true;
    unsigned long frame=0;
    int lastTouch=-1, lastMode=-1;
    float lastDistance=-1.0f;
    Vector3 cameraDirection = Vector3Normalize(Vector3Subtract(camera.position, camera.target));

    while (!WindowShouldClose()) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        Rectangle resetRect = { 20.0f, (float)sh - 92.0f, 150.0f, 58.0f };
        Rectangle wireRect = { (float)sw - 170.0f, (float)sh - 92.0f, 150.0f, 58.0f };
        Rectangle zoomInRect = {20.0f, (float)sh-164.0f, 76.0f, 52.0f};
        Rectangle zoomOutRect = {108.0f, (float)sh-164.0f, 76.0f, 52.0f};
        Rectangle freezeRect = {196.0f, (float)sh-164.0f, 104.0f, 52.0f};
        Rectangle gridRect = {312.0f, (float)sh-164.0f, 100.0f, 52.0f};
        Rectangle depthRect = {424.0f, (float)sh-164.0f, 130.0f, 52.0f};

        int touchCount = GetTouchPointCount();
        int observedPointer0=touchCount>0?GetTouchPointId(0):-1;
        int observedPointer1=touchCount>1?GetTouchPointId(1):-1;
        bool pointerDown = false;
        Vector2 pointer = {0};

        if (touchCount >= 2) {
            Vector2 firstTouch = GetTouchPosition(0);
            Vector2 secondTouch = GetTouchPosition(1);
            float pinchDistance = Vector2Distance(firstTouch, secondTouch);

            crystal_zoom_touches(&zoom, touchCount, GetTouchPointId(0), GetTouchPointId(1), pinchDistance);
            pointerWasDown = false;
            pointerConsumed = false;
        } else {
            if (zoom.pinching) {
                pointerWasDown = false;
                pointerConsumed = false;
            }
            crystal_zoom_touches(&zoom, touchCount, -1, -1, 0.0f);

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
                    crystal_zoom_reset(&zoom);
                    pointerConsumed = true;
                } else if (CheckCollisionPointRec(pointer, wireRect)) {
                    renderMode = (renderMode + 1) % 3;
                    pointerConsumed = true;
                } else if (CheckCollisionPointRec(pointer, zoomInRect)) {
                    crystal_zoom_apply(&zoom, zoom.distance/1.25f); pointerConsumed=true;
                } else if (CheckCollisionPointRec(pointer, zoomOutRect)) {
                    crystal_zoom_apply(&zoom, zoom.distance*1.25f); pointerConsumed=true;
                } else if (CheckCollisionPointRec(pointer, freezeRect)) {
                    frozen=!frozen; pointerConsumed=true;
                } else if (CheckCollisionPointRec(pointer, gridRect)) {
                    showGrid=!showGrid; pointerConsumed=true;
                } else if (CheckCollisionPointRec(pointer, depthRect)) {
                    xrayEdges=!xrayEdges; pointerConsumed=true;
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

        if (!frozen) yaw += auto_spin * GetFrameTime();
        model.transform = MatrixMultiply(MatrixRotateX(pitch), MatrixRotateY(yaw));
        camera.position = Vector3Add(camera.target, Vector3Scale(cameraDirection, zoom.distance));

        BeginDrawing();
        ClearBackground((Color){ 18, 20, 25, 255 });

        BeginMode3D(camera);
        if (showGrid) DrawGrid(12, 0.5f);
        if (renderMode != 1) {
            rlDisableBackfaceCulling();
            DrawModel(model, Vector3Zero(), 1.0f, tint);
            rlEnableBackfaceCulling();
        }
        if (renderMode != 0) {
            // Flush before changing state: deferred triangles/lines otherwise
            // inherit the depth state at flush, rather than at submission.
            rlDrawRenderBatchActive();
            if (xrayEdges) rlDisableDepthTest();
            rlSetLineWidth(2.0f);
            draw_net_edges(&net, edges, edge_count, model.transform, (Color){ 245, 248, 252, 255 });
            rlSetLineWidth(1.0f);
            rlDrawRenderBatchActive();
            if (xrayEdges) rlEnableDepthTest();
        }
        EndMode3D();

        DrawRectangle(0, 0, sw, 82, Fade(BLACK, 0.55f));
        DrawText(CRYSTAL_TITLE, 20, 18, 28, RAYWHITE);
        DrawText(TextFormat("%.12s  %s", CRYSTAL_SOURCE_COMMIT,
                 renderMode == 0 ? "SOLID" : renderMode == 1 ? "EDGES" : "SOLID+EDGES"), 20, 50, 16, LIGHTGRAY);
        DrawText(TextFormat("distance %.2f [%.1f,%.1f] clip %.2f/%.0f",
                 zoom.distance, zoom.minimum, zoom.maximum,
                 (double)RL_CULL_DISTANCE_NEAR, (double)RL_CULL_DISTANCE_FAR), 20, 88, 16, LIGHTGRAY);
        DrawText(TextFormat("touch %d ids %d/%d span %.1f zoom %.3f %s",
                 touchCount, zoom.first, zoom.second, zoom.span, zoom.applied,
                 frozen ? "FROZEN" : "SPIN"), 20, 110, 16, LIGHTGRAY);
        DrawText(TextFormat("geometry OK: %d nodes %d facets %d triangles %d edges",
                 net.net.node_count, net.net.facet_count, mesh.triangle_count, edge_count), 20, 132, 14, LIGHTGRAY);
        DrawText(TextFormat("%s / %s", glVendor, glRenderer), 20, 152, 12, LIGHTGRAY);

        DrawRectangleRec(resetRect, Fade(DARKGRAY, 0.92f));
        DrawRectangleLinesEx(resetRect, 2.0f, GRAY);
        DrawText("RESET", (int)resetRect.x + 31, (int)resetRect.y + 18, 20, RAYWHITE);

        DrawRectangleRec(wireRect, Fade(DARKGRAY, 0.92f));
        DrawRectangleLinesEx(wireRect, 2.0f, GRAY);
        const char *viewLabel = (renderMode == 0) ? "SOLID" : (renderMode == 1) ? "NET" : "BOTH";
        DrawText(viewLabel, (int)wireRect.x + 42, (int)wireRect.y + 18, 18, RAYWHITE);

        DrawRectangleRec(zoomInRect, DARKGRAY); DrawText("+", 48, sh-152, 24, RAYWHITE);
        DrawRectangleRec(zoomOutRect, DARKGRAY); DrawText("-", 136, sh-152, 24, RAYWHITE);
        DrawRectangleRec(freezeRect, DARKGRAY); DrawText(frozen ? "RESUME" : "FREEZE", 204, sh-146, 16, RAYWHITE);
        DrawRectangleRec(gridRect, DARKGRAY); DrawText("GRID", 326, sh-146, 16, RAYWHITE);
        DrawRectangleRec(depthRect, DARKGRAY); DrawText(xrayEdges ? "X-RAY" : "OCCLUDED", 432, sh-146, 14, RAYWHITE);
        DrawText("drag: rotate   pinch / +/-: zoom", 20, sh - 190, 17, LIGHTGRAY);

#if CRYSTAL_EMULATOR_TEST
        DrawRectangle(0, 88, sw, 94, Fade(BLACK, 0.60f));
        DrawText(TextFormat("CI diag: kind=%d nodes=%d facets=%d edges=%d",
                            CRYSTAL_KIND, net.net.node_count, net.net.facet_count, edge_count),
                 20, 96, 16, LIME);
        DrawText(TextFormat("mode=%s yaw=%.3f pitch=%.3f dist=%.3f",
                            viewLabel, yaw, pitch, zoom.distance),
                 20, 122, 16, LIME);
        DrawText(TextFormat("screen=%dx%d projection=perspective", sw, sh),
                 20, 148, 16, LIME);
#endif

        EndDrawing();
        frame++;
        if (touchCount != lastTouch || renderMode != lastMode ||
            fabsf(zoom.distance-lastDistance)>0.001f || frame%60==0) {
            char snapshot[1800];
            snprintf(snapshot, sizeof(snapshot),
                "{\"schema\":\"crystal-state-v1\",\"source_commit\":\"%s\",\"package\":\"%s\","
                "\"session\":\"%s\",\"replay_id\":\"frame-%lu\",\"screen_width\":\"%d\",\"screen_height\":\"%d\",\"gl_vendor\":\"%s\","
                "\"gl_renderer\":\"%s\",\"gl_version\":\"%s\",\"render_path\":\"gles2-explicit-lines\","
                "\"geometry_result\":\"PASS\",\"camera_distance\":\"%.5f\",\"camera_min\":\"%.3f\","
                "\"camera_max\":\"%.3f\",\"clip_near\":\"%.3f\",\"clip_far\":\"%.3f\","
                "\"touch_count\":\"%d\",\"pointer_ids\":\"%d,%d\",\"pinch_span\":\"%.5f\","
                "\"applied_zoom\":\"%.5f\",\"requested_distance\":\"%.5f\",\"yaw\":\"%.5f\","
                "\"pitch\":\"%.5f\",\"frozen\":\"%d\",\"render_mode\":\"%d\",\"xray\":\"%d\"}",
                CRYSTAL_SOURCE_COMMIT, CRYSTAL_PACKAGE_ID, launchSession, frame, sw, sh,
                safeVendor, safeRenderer, safeVersion, zoom.distance, zoom.minimum, zoom.maximum,
                (double)RL_CULL_DISTANCE_NEAR, (double)RL_CULL_DISTANCE_FAR, touchCount,
                observedPointer0, observedPointer1, zoom.span, zoom.applied, zoom.requested, yaw, pitch,
                frozen, renderMode, xrayEdges);
#if defined(__ANDROID__)
            __android_log_write(ANDROID_LOG_INFO, "CrystalState", snapshot);
#else
            TraceLog(LOG_INFO, "CrystalState: %s", snapshot);
#endif
            lastTouch=touchCount; lastMode=renderMode; lastDistance=zoom.distance;
        }
    }

    UnloadModel(model);
    CloseWindow();
    return 0;
}
#endif
