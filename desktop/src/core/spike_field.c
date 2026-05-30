#include "spike_field.h"

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <string.h>

#include "resource_path.h"
#include "shader_utils.h"

struct SpikeField {
    Mesh mesh;
    Material material;
    Matrix* transforms;
    int instances;
    int gridSize;
    int time_location;
};

#define GRID_WORLD_SIZE 2.0f
#define SPIKE_MESH_TRIANGLE_COUNT 4
#define SPIKE_MESH_VERTEX_COUNT (SPIKE_MESH_TRIANGLE_COUNT * 3)

static const float SPIKE_MESH_VERTICES[SPIKE_MESH_VERTEX_COUNT * 3] = {
    -0.5f, 0.0f, 0.0f,
     0.5f, 0.0f, 0.0f,
    -0.5f, 1.0f, 0.0f,

    -0.5f, 1.0f, 0.0f,
     0.5f, 0.0f, 0.0f,
     0.5f, 1.0f, 0.0f,

     0.0f, 0.0f, -0.5f,
     0.0f, 0.0f,  0.5f,
     0.0f, 1.0f, -0.5f,

     0.0f, 1.0f, -0.5f,
     0.0f, 0.0f,  0.5f,
     0.0f, 1.0f,  0.5f,
};

static Mesh BuildSpikeMesh(void) {
    Mesh mesh = {0};
    mesh.vertexCount = SPIKE_MESH_VERTEX_COUNT;
    mesh.triangleCount = SPIKE_MESH_TRIANGLE_COUNT;
    mesh.vertices = MemAlloc(sizeof(SPIKE_MESH_VERTICES));
    memcpy(mesh.vertices, SPIKE_MESH_VERTICES, sizeof(SPIKE_MESH_VERTICES));
    UploadMesh(&mesh, false);
    return mesh;
}

static Material BuildSpikeMaterial(int gridSize) {
    char vsPath[RESOURCE_PATH_MAX];
    char fsPath[RESOURCE_PATH_MAX];

    ResourcePath("resources/shaders/spike_field.vs", vsPath);
    ResourcePath("resources/shaders/spike_field.fs", fsPath);

    Shader shader = LoadShaderWithGlslVersion(vsPath, fsPath);
    shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(shader, "instanceTransform");
    SetShaderValue(shader, GetShaderLocation(shader, "gridSize"), &gridSize, SHADER_UNIFORM_INT);

    char maskPath[RESOURCE_PATH_MAX];
    ResourcePath("resources/mask.png", maskPath);
    Texture mask = LoadTexture(maskPath);
    SetTextureFilter(mask, TEXTURE_FILTER_BILINEAR);

    Material material = LoadMaterialDefault();
    material.shader = shader;
    SetMaterialTexture(&material, MATERIAL_MAP_DIFFUSE, mask);

    return material;
}

static Matrix* BuildSpikeTransforms(int gridSize) {
    int count = gridSize * gridSize;
    Matrix* transforms = MemAlloc(sizeof(Matrix) * count);
    float spikeWidth = GRID_WORLD_SIZE / gridSize;

    for (int idx = 0; idx < count; idx++) {
        int row = idx / gridSize;
        int column = idx % gridSize;

        float x = -GRID_WORLD_SIZE / 2.0f + row * spikeWidth + spikeWidth / 2.0f;
        float z = -GRID_WORLD_SIZE / 2.0f + column * spikeWidth + spikeWidth / 2.0f;

        transforms[idx] = MatrixMultiply(
            MatrixScale(spikeWidth, 1.0f, spikeWidth),
            MatrixTranslate(x, 0.0f, z));
    }

    return transforms;
}

SpikeField* LoadSpikeField(int gridSize) {
    SpikeField* field = MemAlloc(sizeof(SpikeField));
    field->gridSize = gridSize;
    field->instances = gridSize * gridSize;
    field->mesh = BuildSpikeMesh();
    field->material = BuildSpikeMaterial(gridSize);
    field->transforms = BuildSpikeTransforms(gridSize);
    field->time_location = GetShaderLocation(field->material.shader, "time");
    return field;
}

void UpdateSpikeField(const SpikeField* field, float time) {
    SetShaderValue(field->material.shader, field->time_location, &time, SHADER_UNIFORM_FLOAT);
}

void DrawSpikeField(const SpikeField* field, Camera3D camera) {
    rlDisableBackfaceCulling();
    BeginMode3D(camera);
    DrawMeshInstanced(field->mesh, field->material, field->transforms, field->instances);
    EndMode3D();
    rlEnableBackfaceCulling();
}

void UnloadSpikeField(SpikeField* field) {
    UnloadMaterial(field->material);
    UnloadMesh(field->mesh);
    MemFree(field->transforms);
    MemFree(field);
}
