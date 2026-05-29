#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(PLATFORM_IOS)
#include <libGLESv2/GLES3/gl3.h>
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
#include "spike_field.h"

#if defined(PLATFORM_ANDROID)
#include "raymob.h"
#endif

#define FPS_STATS_WARMUP_SECONDS 3.0
#define FPS_STATS_INITIAL_CAPACITY 1024
#define FPS_STATS_MIN_SAMPLES 30
#define FPS_STATS_TRIM_RATIO 0.05
#define FPS_STATS_UPDATE_INTERVAL 0.25

typedef struct RenderInfo {
    bool loaded;
    char gpu_model[256];
    char gl_version[128];
    char raylib_renderer[64];
} RenderInfo;

typedef struct FpsStatsResult {
    bool ready;
    double min;
    double max;
    double avg;
} FpsStatsResult;

typedef struct FpsStats {
    double elapsed_time;
    double refresh_time;
    unsigned int sample_count;
    unsigned int sample_capacity;
    float* frame_times;
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

static void DrawRenderInfo(const RenderInfo* info, int x, int y, float textScale) {
    const int fontSize = (int)(20.0f * textScale / 5.0f + 0.5f) * 5;
    const int lineHeight = (int)(25.0f * textScale / 5.0f + 0.5f) * 5;

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

static bool ReserveFpsStatsSamples(FpsStats* stats, unsigned int requiredCapacity) {
    if (requiredCapacity <= stats->sample_capacity) {
        return true;
    }

    unsigned int newCapacity = stats->sample_capacity > 0 ? stats->sample_capacity * 2 : FPS_STATS_INITIAL_CAPACITY;

    while (newCapacity < requiredCapacity) {
        newCapacity *= 2;
    }

    float* newFrameTimes = realloc(stats->frame_times, newCapacity * sizeof(stats->frame_times[0]));

    if (newFrameTimes == NULL) {
        return false;
    }

    stats->frame_times = newFrameTimes;
    stats->sample_capacity = newCapacity;
    return true;
}

static FpsStatsResult CalculateFpsStatsResult(FpsStats* stats) {
    FpsStatsResult result = {0};

    if (stats->sample_count < FPS_STATS_MIN_SAMPLES) {
        return result;
    }

    qsort(stats->frame_times, stats->sample_count, sizeof(stats->frame_times[0]), CompareFrameTimes);

    unsigned int trim = (unsigned int)((double)stats->sample_count * FPS_STATS_TRIM_RATIO);

    if (trim * 2 >= stats->sample_count) {
        trim = 0;
    }

    const unsigned int start = trim;
    const unsigned int end = stats->sample_count - trim;
    double totalTime = 0.0;

    for (unsigned int i = start; i < end; i++) {
        totalTime += stats->frame_times[i];
    }

    if (totalTime <= 0.0) {
        return result;
    }

    result.min = 1.0 / stats->frame_times[end - 1];
    result.max = 1.0 / stats->frame_times[start];
    result.avg = (double)(end - start) / totalTime;
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

    if (!ReserveFpsStatsSamples(stats, stats->sample_count + 1)) {
        return;
    }

    stats->frame_times[stats->sample_count] = frameTime;
    stats->sample_count++;
    stats->refresh_time += frameTime;

    if (!stats->cached_result.ready || stats->refresh_time >= FPS_STATS_UPDATE_INTERVAL) {
        stats->cached_result = CalculateFpsStatsResult(stats);
        stats->refresh_time = 0.0;
    }
}

static void DrawFpsStats(const FpsStats* stats, int x, int y, float textScale) {
    const int fontSize = (int)(20.0f * textScale / 5.0f + 0.5f) * 5;
    const FpsStatsResult result = stats->cached_result;

    if (!result.ready) {
        DrawText("FPS min: -- max: -- avg: --", x, y, fontSize, LIME);
        return;
    }

    DrawText(TextFormat("FPS min: %.0f max: %.0f avg: %.0f", result.min, result.max, result.avg), x, y, fontSize, LIME);
}

static void UnloadFpsStats(FpsStats* stats) {
    free(stats->frame_times);
    stats->frame_times = NULL;
    stats->sample_count = 0;
    stats->sample_capacity = 0;
}

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
#if defined(PLATFORM_MOBILE)
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "Raymark");
#else
    InitWindow(1280, 720, "Raymark");
#endif

    SetTargetFPS(0);

    RenderInfo renderInfo = {0};
    UpdateRenderInfo(&renderInfo);
    FpsStats fpsStats = {0};

    Background* background = LoadBackground();
    SpikeField* field = LoadSpikeField();

    int physW = GetScreenWidth();
    int physH = GetScreenHeight();
    int shortSide = physW < physH ? physW : physH;
    float textScale = (float)shortSide / 720.0f;

#if defined(PLATFORM_ANDROID)
    float pxRatio = (float)GetRenderWidth() / (float)physW;
    int safeX = GetSafeAreaLeft() > 0 ? (int)(GetSafeAreaLeft() * pxRatio + 20.0f) : 40;
    int safeY = GetSafeAreaTop() > 0 ? (int)(GetSafeAreaTop() * pxRatio + 10.0f) : 40;
#else
    int safeX = 40;
    int safeY = 40;
#endif

    float time = 0.0f;
    float angle = 0.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        UpdateFpsStats(&fpsStats, dt);
        time += dt;
        angle += dt * 20.0f;

        UpdateBackground(background, time, GetRenderWidth(), GetRenderHeight());
        UpdateSpikeField(field, time);

        float radius = 2.8f;
        float height = 1.8f;
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
        DrawBackground(background, GetRenderWidth(), GetRenderHeight());
        DrawSpikeField(field, cam);
        DrawFpsStats(&fpsStats, safeX, safeY, textScale);
        UpdateRenderInfo(&renderInfo);
        DrawRenderInfo(&renderInfo, safeX, safeY + (int)(30.0f * textScale), textScale);
        EndDrawing();
    }

    UnloadSpikeField(field);
    UnloadBackground(background);
    UnloadFpsStats(&fpsStats);
    CloseWindow();
    return 0;
}
