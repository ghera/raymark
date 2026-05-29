#include "spike_field.h"

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <string.h>

#include "shader_utils.h"

struct SpikeField {
    Mesh mesh;
    Material material;
    Matrix* transforms;
    int instances;
    int time_location;
};

#define GRID_SIZE 256
#define INSTANCE_COUNT (GRID_SIZE * GRID_SIZE)

#define GRID_WORLD_SIZE 2.0f
#define SPIKE_WIDTH (GRID_WORLD_SIZE / GRID_SIZE)

#define SPIKE_MESH_TRIANGLE_COUNT 4
#define SPIKE_MESH_VERTEX_COUNT (SPIKE_MESH_TRIANGLE_COUNT * 3)

static const float SPIKE_MESH_VERTICES[SPIKE_MESH_VERTEX_COUNT * 3] = {
    -0.5f,
    0.0f,
    0.0f,
    0.5f,
    0.0f,
    0.0f,
    -0.5f,
    1.0f,
    0.0f,

    -0.5f,
    1.0f,
    0.0f,
    0.5f,
    0.0f,
    0.0f,
    0.5f,
    1.0f,
    0.0f,

    0.0f,
    0.0f,
    -0.5f,
    0.0f,
    0.0f,
    0.5f,
    0.0f,
    1.0f,
    -0.5f,

    0.0f,
    1.0f,
    -0.5f,
    0.0f,
    0.0f,
    0.5f,
    0.0f,
    1.0f,
    0.5f,
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

static Material BuildSpikeMaterial(void) {
    Shader shader = LoadShaderWithGlslVersion("resources/shaders/spike_field.vs", "resources/shaders/spike_field.fs");
    shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(shader, "instanceTransform");
    int gridSize = GRID_SIZE;
    SetShaderValue(shader, GetShaderLocation(shader, "gridSize"), &gridSize, SHADER_UNIFORM_INT);

    Texture mask = LoadTexture("resources/mask.png");
    SetTextureFilter(mask, TEXTURE_FILTER_BILINEAR);

    Material material = LoadMaterialDefault();
    material.shader = shader;
    SetMaterialTexture(&material, MATERIAL_MAP_DIFFUSE, mask);

    return material;
}

static Matrix* BuildSpikeTransforms(void) {
    Matrix* transforms = MemAlloc(sizeof(Matrix) * INSTANCE_COUNT);

    for (size_t idx = 0; idx < INSTANCE_COUNT; idx++) {
        int row = idx / GRID_SIZE;
        int column = idx % GRID_SIZE;

        float x = -GRID_WORLD_SIZE / 2.0f + row * SPIKE_WIDTH + SPIKE_WIDTH / 2.0f;
        float z = -GRID_WORLD_SIZE / 2.0f + column * SPIKE_WIDTH + SPIKE_WIDTH / 2.0f;

        transforms[idx] = MatrixMultiply(
            MatrixScale(SPIKE_WIDTH, 1.0f, SPIKE_WIDTH),
            MatrixTranslate(x, 0.0f, z));
    }

    return transforms;
}

SpikeField* LoadSpikeField(void) {
    SpikeField* field = MemAlloc(sizeof(SpikeField));

    field->mesh = BuildSpikeMesh();
    field->material = BuildSpikeMaterial();
    field->transforms = BuildSpikeTransforms();
    field->instances = INSTANCE_COUNT;
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
    MemFree(field);
}
