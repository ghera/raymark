#pragma once

#include <raylib.h>

#define REFERENCE_HEIGHT 720

typedef struct SpikeField SpikeField;

SpikeField* LoadSpikeField(int gridSize);
void UpdateSpikeField(const SpikeField* field, float time);
void DrawSpikeField(const SpikeField* field, Camera3D camera);
void UnloadSpikeField(SpikeField* field);
