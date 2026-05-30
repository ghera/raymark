#pragma once

#include <raylib.h>
#include <stdio.h>
#include <string.h>

#if defined(PLATFORM_IOS)
#include "IOSBridge.h"
#endif

#define RESOURCE_PATH_MAX 1024

static inline void ResourcePath(const char* relativePath, char* out) {
#if defined(PLATFORM_IOS)
    const char* appDir = GetApplicationDirectory();
    const char* lastSlash = strrchr(relativePath, '/');
    const char* filename = (lastSlash != NULL) ? lastSlash + 1 : relativePath;
    snprintf(out, RESOURCE_PATH_MAX, "%s%s", appDir, filename);
#else
    snprintf(out, RESOURCE_PATH_MAX, "%s", relativePath);
#endif
}
