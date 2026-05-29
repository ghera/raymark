# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.22.1)

# Set standards for C and C++
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

# Set the project name based on the name given on the gradle.properties
project("${APP_LIB_NAME}")

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384")

set(CORE_DIR "${CMAKE_SOURCE_DIR}/../../../../../desktop/src/core")
set(RAYLIB_DIR "${CMAKE_SOURCE_DIR}/../../../../../raylib")

# Include raylib and raymob as a subdirectories
add_subdirectory(${CMAKE_SOURCE_DIR}/raymob)
add_subdirectory(${RAYLIB_DIR} ${CMAKE_BINARY_DIR}/raylib)
target_compile_definitions(raylib PRIVATE RLGL_ENABLE_GLES_DEFAULT_HIGHP_PRECISION=1)

# Fetch all source files for your project (recursively), excluding 'deps' source files
file(GLOB_RECURSE SOURCES "${CORE_DIR}/*.c" "${CORE_DIR}/*.cpp" "${CORE_DIR}/*.hpp")
list(FILTER SOURCES EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/raymob/.*")

include_directories(${CORE_DIR})
include_directories(${ANDROID_NDK}/sources/android/native_app_glue/)

# Add android_native_app_glue.c to the source files
list(APPEND SOURCES ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

# Create a shared library with game source files
add_library(${APP_LIB_NAME} SHARED ${SOURCES})

# Define compiler macros for the library
target_compile_definitions(${APP_LIB_NAME} PRIVATE PLATFORM_ANDROID PLATFORM_MOBILE)

# Apply flags depending on the build type
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_definitions(${APP_LIB_NAME} PRIVATE _DEBUG DEBUG)
endif()

# Include raylib and raymob header files
target_include_directories(${APP_LIB_NAME} PRIVATE "${RAYLIB_DIR}")
target_include_directories(${APP_LIB_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/raymob")
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CORE_DIR})

# Link required libraries to the native application
target_link_libraries(${APP_LIB_NAME} raylib raymoblib)
