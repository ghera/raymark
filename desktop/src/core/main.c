#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(MIN)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#if !defined(MAX)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
#define PLATFORM_MOBILE
#endif

#if defined(PLATFORM_IOS)
#include <libGLESv2/GLES2/gl2.h>
#elif defined(GRAPHICS_API_OPENGL_ES3)
#include <GLES3/gl3.h>
#elif defined(GRAPHICS_API_OPENGL_ES2)
#if defined(PLATFORM_DESKTOP) || defined(PLATFORM_DESKTOP_GLFW) || defined(PLATFORM_DESKTOP_SDL)
#include "external/glad_gles2.h"
#else
#include <GLES2/gl2.h>
#endif
#elif defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_43)
#include "external/glad.h"
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <rlgl.h>

#include "background.h"
#include "resource_path.h"
#include "spike_field.h"

#if defined(PLATFORM_ANDROID)
#include "raymob.h"
#endif

#if defined(PLATFORM_IOS)
#include "IOSBridge.h"
#endif

#define FPS_STATS_WARMUP_SECONDS 3.0
#define FPS_STATS_WINDOW_SIZE 1000
#define FPS_STATS_MIN_SAMPLES 100
#define FPS_STATS_UPDATE_INTERVAL 0.25

typedef struct RenderInfo {
    bool loaded;
    char gpu_model[256];
    char gl_version[128];
    char raylib_renderer[64];
} RenderInfo;

typedef struct FpsStatsResult {
    bool ready;
    double avg;
    double low1pct;
    double low01pct;
} FpsStatsResult;

typedef struct FpsStats {
    double elapsed_time;
    double refresh_time;
    unsigned int sample_count;
    unsigned int write_idx;
    float frame_times[FPS_STATS_WINDOW_SIZE];
    FpsStatsResult cached_result;
} FpsStats;

static const char* RaylibRendererName(int version) {
    switch (version) {
        case 0:
            return "Software";
        case RL_OPENGL_11:
            return "OpenGL 1.1";
        case RL_OPENGL_21:
            return "OpenGL 2.1";
        case RL_OPENGL_33:
            return "OpenGL 3.3";
        case RL_OPENGL_43:
            return "OpenGL 4.3";
        case RL_OPENGL_ES_20:
            return "OpenGL ES 2.0";
        case RL_OPENGL_ES_30:
            return "OpenGL ES 3.0";
        default:
            return "Unknown";
    }
}

static void UpdateRenderInfo(RenderInfo* info) {
    if (info->loaded) {
        return;
    }

    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* version = (const char*)glGetString(GL_VERSION);

    if (renderer == NULL || version == NULL) {
        return;
    }

    snprintf(info->gpu_model,
             sizeof(info->gpu_model),
             "%s%s%s",
             vendor != NULL ? vendor : "",
             vendor != NULL ? " " : "",
             renderer);
    snprintf(info->gl_version, sizeof(info->gl_version), "%s", version);
    snprintf(info->raylib_renderer, sizeof(info->raylib_renderer), "%s", RaylibRendererName(rlGetVersion()));
    info->loaded = true;
}

static void DrawRenderInfo(const RenderInfo* info, int x, int y, int fontSize, int lineHeight) {
    if (!info->loaded) {
        DrawText("GPU: waiting...", x, y, fontSize, RAYWHITE);
        return;
    }

    DrawText(TextFormat("GPU: %s", info->gpu_model), x, y, fontSize, RAYWHITE);
    DrawText(TextFormat("raylib: %s", info->raylib_renderer), x, y + lineHeight, fontSize, RAYWHITE);
    DrawText(TextFormat("driver: %s", info->gl_version), x, y + lineHeight * 2, fontSize, RAYWHITE);
}

static int CompareFrameTimes(const void* a, const void* b) {
    const float left = *(const float*)a;
    const float right = *(const float*)b;

    return (left > right) - (left < right);
}

static FpsStatsResult CalculateFpsStatsResult(FpsStats* stats) {
    FpsStatsResult result = {0};
    unsigned int n = stats->sample_count;

    if (n < FPS_STATS_MIN_SAMPLES) {
        return result;
    }

    if (n > FPS_STATS_WINDOW_SIZE) {
        n = FPS_STATS_WINDOW_SIZE;
    }

    // Ring buffer: oldest sample is at write_idx when full, at 0 when not yet full
    unsigned int start = (n >= FPS_STATS_WINDOW_SIZE) ? stats->write_idx : 0;

    // Copy frame times into temp buffer for sorting
    float sorted[FPS_STATS_WINDOW_SIZE];
    for (unsigned int i = 0; i < n; i++) {
        sorted[i] = stats->frame_times[(start + i) % FPS_STATS_WINDOW_SIZE];
    }

    qsort(sorted, n, sizeof(sorted[0]), CompareFrameTimes);

    double total = 0.0;
    for (unsigned int i = 0; i < n; i++) {
        total += sorted[i];
    }

    result.avg = (double)n / total;

    // 1% low: average of worst 1% frames (slowest at end after sort)
    {
        unsigned int count = n / 100;
        if (count < 1) count = 1;
        double sum = 0.0;
        for (unsigned int i = 0; i < count; i++) {
            sum += sorted[n - 1 - i];
        }
        result.low1pct = (double)count / sum;
    }

    // 0.1% low: average of worst 0.1% frames
    {
        unsigned int count = n / 1000;
        if (count < 1) count = 1;
        double sum = 0.0;
        for (unsigned int i = 0; i < count; i++) {
            sum += sorted[n - 1 - i];
        }
        result.low01pct = (double)count / sum;
    }

    result.ready = true;
    return result;
}

static void UpdateFpsStats(FpsStats* stats, float frameTime) {
    if (frameTime <= 0.0f) {
        return;
    }

    stats->elapsed_time += frameTime;

    if (stats->elapsed_time < FPS_STATS_WARMUP_SECONDS) {
        return;
    }

    stats->frame_times[stats->write_idx] = frameTime;
    stats->write_idx = (stats->write_idx + 1) % FPS_STATS_WINDOW_SIZE;

    if (stats->sample_count < FPS_STATS_WINDOW_SIZE) {
        stats->sample_count++;
    }

    stats->refresh_time += frameTime;

    if (!stats->cached_result.ready || stats->refresh_time >= FPS_STATS_UPDATE_INTERVAL) {
        stats->cached_result = CalculateFpsStatsResult(stats);
        stats->refresh_time = 0.0;
    }
}

static void DrawFpsStats(const FpsStats* stats, int x, int y, int fontSize, int lineHeight) {
    const FpsStatsResult result = stats->cached_result;

    if (!result.ready) {
        DrawText("FPS avg: ---", x, y, fontSize, LIME);
        DrawText("1% low: ---", x, y + lineHeight, fontSize, ORANGE);
        DrawText("0.1% low: ---", x, y + lineHeight * 2, fontSize, RED);
        return;
    }

    DrawText(TextFormat("FPS avg: %.0f", result.avg), x, y, fontSize, LIME);
    DrawText(TextFormat("1%% low: %.0f", result.low1pct), x, y + lineHeight, fontSize, ORANGE);
    DrawText(TextFormat("0.1%% low: %.0f", result.low01pct), x, y + lineHeight * 2, fontSize, RED);
}

static void UnloadFpsStats(FpsStats* stats) {
    stats->sample_count = 0;
}

RenderInfo renderInfo = {0};
FpsStats fpsStats = {0};
Background* background;
SpikeField* field;
float rotationTime = 0.0f;
float angle = 0.0f;
int safeX = 20;
int safeTopY = 20;
int safeBottomY = 20;
float textScale = 1.0f;

void ready() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
#if defined(PLATFORM_MOBILE)
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "Raymark");
#else
    InitWindow(1280, 720, "Raymark");
#endif

    SetTargetFPS(0);

    UpdateRenderInfo(&renderInfo);

    int shortScreen = MIN(GetScreenWidth(), GetScreenHeight());
    textScale = (float)shortScreen / (float)REFERENCE_HEIGHT;

    int shortRender = MIN(GetRenderWidth(), GetRenderHeight());
    int gridSize = (int)(256.0f * shortRender / (float)REFERENCE_HEIGHT + 0.5f);
    gridSize = (gridSize / 4) * 4;
    gridSize = MAX(gridSize, 16);

    background = LoadBackground();
    field = LoadSpikeField(gridSize);

#if defined(PLATFORM_IOS)
    SafeAreaInsets insets = GetIOSSafeAreaInsets();
    safeX = insets.left + 10;
    safeTopY = insets.top + 10;
    safeBottomY = insets.bottom + 10;
#elif defined(PLATFORM_ANDROID)
    safeX = GetSafeAreaLeft() + 20;
    safeTopY = GetSafeAreaTop() + 20;
    safeBottomY = GetSafeAreaBottom() + 20;
#endif
}

#if !defined(PLATFORM_MOBILE)
static void ToggleFullscreenMode(void) {
    UnloadSpikeField(field);
    UnloadBackground(background);
    UnloadFpsStats(&fpsStats);

    ToggleFullscreen();

    // Recalc everything like in ready()
    int shortScreen = MIN(GetScreenWidth(), GetScreenHeight());
    textScale = (float)shortScreen / (float)REFERENCE_HEIGHT;

    int shortRender = MIN(GetRenderWidth(), GetRenderHeight());
    int gridSize = (int)(256.0f * shortRender / (float)REFERENCE_HEIGHT + 0.5f);
    gridSize = (gridSize / 4) * 4;
    gridSize = MAX(gridSize, 16);

    background = LoadBackground();
    field = LoadSpikeField(gridSize);

    rotationTime = 0.0f;
    angle = 0.0f;
    fpsStats = (FpsStats){0};
    renderInfo.loaded = false;
}
#endif

void update(bool viewSizeChanged) {
#if !defined(PLATFORM_MOBILE)
    if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))) {
        ToggleFullscreenMode();
    }
#endif

    float dt = GetFrameTime();
    UpdateFpsStats(&fpsStats, dt);
    rotationTime += dt;
    angle += dt * 20.0f;

    UpdateBackground(background, rotationTime, GetRenderWidth(), GetRenderHeight());
    UpdateSpikeField(field, rotationTime);

    float radius = 1.5f;
    float height = 2.2f;
    Camera3D cam = {
        .position = {
            cosf(angle * DEG2RAD) * radius,
            height,
            sinf(angle * DEG2RAD) * radius,
        },
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    BeginDrawing();
    ClearBackground(BLACK);
    DrawBackground(background, GetScreenWidth(), GetScreenHeight());
    DrawSpikeField(field, cam);
    int fontSize = (int)(20.0f * textScale / 10.0f + 0.5f) * 10;
    int lineHeight = (int)((float)fontSize * 1.2f);
    DrawFpsStats(&fpsStats, safeX, safeTopY, fontSize, lineHeight);
    DrawText(TextFormat("Resolution: %d x %d", GetRenderWidth(), GetRenderHeight()), safeX, safeTopY + lineHeight * 3, fontSize, SKYBLUE);
    UpdateRenderInfo(&renderInfo);
    DrawRenderInfo(&renderInfo, safeX, GetScreenHeight() - safeBottomY - lineHeight * 3, fontSize, lineHeight);
    EndDrawing();
}

void destroy() {
    UnloadSpikeField(field);
    UnloadBackground(background);
    UnloadFpsStats(&fpsStats);
    CloseWindow();
}

#if defined(PLATFORM_IOS)
void ios_ready() {
    ready();
}

void ios_update(bool viewSizeChanged) {
    update(viewSizeChanged);
}

void ios_destroy() {
    destroy();
}
#else
int main() {
    ready();

    while (!WindowShouldClose()) {
        update(false);
    }

    destroy();
    return 0;
}
#endif
