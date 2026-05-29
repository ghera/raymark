#pragma once

#include <raylib.h>

typedef struct SpikeField SpikeField;

SpikeField* LoadSpikeField(void);
void UpdateSpikeField(const SpikeField* field, float time);
void DrawSpikeField(const SpikeField* field, Camera3D camera);
void UnloadSpikeField(SpikeField* field);
