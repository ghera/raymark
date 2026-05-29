#pragma once

typedef struct Background Background;

Background* LoadBackground(void);
void UpdateBackground(const Background* background, float time, int width, int height);
void DrawBackground(const Background* background, int width, int height);
void UnloadBackground(Background* background);
