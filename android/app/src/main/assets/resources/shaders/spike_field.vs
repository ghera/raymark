#version $GLSL_VERSION

in vec3 vertexPosition;
in mat4 instanceTransform;

uniform float time;
uniform int gridSize;
uniform sampler2D texture0;
uniform mat4 mvp;

out float spikeHeight;

const float TAU = 6.283185;

uint hash(uint a) {
    a -= (a << 6);
    a ^= (a >> 17);
    a -= (a << 9);
    a ^= (a << 4);
    a -= (a << 3);
    a ^= (a << 10);
    a ^= (a >> 15);
    return a;
}

float hash01(uint seed) {
    return float(hash(seed)) / 4294967295.0;
}

float randomRange(uint seed, float minimum, float maximum) {
    return mix(minimum, maximum, hash01(seed));
}

const float GROUND_HEIGHT_MIN = 0.18;
const float GROUND_HEIGHT_MAX = 0.30;

const float PI_HEIGHT_MIN = 0.40;
const float PI_HEIGHT_MAX = 0.55;

float groundWobble(uint seed) {
    float phaseA = hash01(seed * 541u) * TAU;
    float frequencyA = randomRange(seed * 107u, 0.45, 0.80);
    float amplitudeA = 0.13;
    float waveA = sin(time * TAU * frequencyA + phaseA);

    float phaseB = hash01(seed * 449u) * TAU;
    float frequencyB = randomRange(seed * 277u, 1.05, 1.55);
    float amplitudeB = 0.07;
    float waveB = sin(time * TAU * frequencyB + phaseB);

    return 1.0 + waveA * amplitudeA + waveB * amplitudeB;
}

float piWobble(uint seed) {
    float phaseA = (hash01(seed * 599u) - 0.5) * 0.45;
    float frequencyA = 0.11;
    float amplitudeA = 0.12;
    float waveA = sin(time * TAU * frequencyA + phaseA);

    float phaseB = hash01(seed * 769u) * TAU;
    float frequencyB = randomRange(seed * 367u, 2.20, 3.30);
    float amplitudeB = 0.035;
    float waveB = sin(time * TAU * frequencyB + phaseB);

    return 1.0 + waveA * amplitudeA + waveB * amplitudeB;
}

void main() {
    uint instanceSeed = uint(gl_InstanceID) + 1u;

    int row = gl_InstanceID / gridSize;
    int column = gl_InstanceID % gridSize;
    vec2 gridUV = vec2(float(row), float(column)) / float(gridSize - 1);

    float groundHeight = randomRange(instanceSeed * 13u, GROUND_HEIGHT_MIN, GROUND_HEIGHT_MAX);
    groundHeight *= groundWobble(instanceSeed * 23u);

    float piHeight = randomRange(instanceSeed * 17u, PI_HEIGHT_MIN, PI_HEIGHT_MAX);
    piHeight *= piWobble(instanceSeed * 47u);

    float maskAlpha = texture(texture0, gridUV).a;
    float piMask = smoothstep(0.4, 0.6, maskAlpha);

    float randomHeight = mix(groundHeight, piHeight, piMask);

    vec3 localPosition = vertexPosition;
    localPosition.y *= randomHeight;

    spikeHeight = localPosition.y;

    gl_Position = mvp * instanceTransform * vec4(localPosition, 1.0);
}
