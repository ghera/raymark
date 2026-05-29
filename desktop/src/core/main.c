#include <math.h>
#include <raylib.h>

#include "background.h"
#include "spike_field.h"

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
#if defined(PLATFORM_MOBILE)
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "Raymark");
#else
    InitWindow(1280, 720, "Raymark");
#endif

    SetTargetFPS(0);

    Background* background = LoadBackground();
    SpikeField* field = LoadSpikeField();

    float time = 0.0f;
    float angle = 0.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
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
        DrawFPS(40, 40);
        EndDrawing();
    }

    UnloadSpikeField(field);
    UnloadBackground(background);
    CloseWindow();
    return 0;
}
