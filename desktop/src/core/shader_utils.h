#pragma once

#include <raylib.h>

static inline const char* GetShaderGlslVersion(void) {
#if defined(PLATFORM_MOBILE) || defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
    return "300 es";
#else
    return "330 core";
#endif
}

static inline char* LoadShaderTextWithGlslVersion(const char* file_name) {
    char* shader_text = LoadFileText(file_name);
    if (shader_text == 0) {
        return 0;
    }

    char* patched_shader_text = TextReplaceAlloc(shader_text, "$GLSL_VERSION", GetShaderGlslVersion());
    UnloadFileText(shader_text);

    return patched_shader_text;
}

static inline Shader LoadShaderWithGlslVersion(const char* vertex_file_name, const char* fragment_file_name) {
    char* vertex_shader_text = vertex_file_name != 0 ? LoadShaderTextWithGlslVersion(vertex_file_name) : 0;
    char* fragment_shader_text = fragment_file_name != 0 ? LoadShaderTextWithGlslVersion(fragment_file_name) : 0;

    Shader shader = LoadShaderFromMemory(vertex_shader_text, fragment_shader_text);

    if (vertex_shader_text != 0) {
        MemFree(vertex_shader_text);
    }

    if (fragment_shader_text != 0) {
        MemFree(fragment_shader_text);
    }

    return shader;
}
