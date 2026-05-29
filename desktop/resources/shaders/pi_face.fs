#version $GLSL_VERSION

#ifdef GL_ES
precision mediump float;
#endif

in vec2 fragTexCoord;
in vec3 fragNormal;

uniform float time;

out vec4 fragColor;

const float TAU = 6.283185;

// --- pi-field hash functions (ported from spike_field.vs) ---

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

// --- pi wobble functions ---

float groundWobble(uint seed, float t) {
    float phaseA = hash01(seed * 541u) * TAU;
    float frequencyA = randomRange(seed * 107u, 0.45, 0.80);
    float amplitudeA = 0.13;
    float waveA = sin(t * TAU * frequencyA + phaseA);

    float phaseB = hash01(seed * 449u) * TAU;
    float frequencyB = randomRange(seed * 277u, 1.05, 1.55);
    float amplitudeB = 0.07;
    float waveB = sin(t * TAU * frequencyB + phaseB);

    return 1.0 + waveA * amplitudeA + waveB * amplitudeB;
}

float piWobble(uint seed, float t) {
    float phaseA = (hash01(seed * 599u) - 0.5) * 0.45;
    float frequencyA = 0.11;
    float amplitudeA = 0.12;
    float waveA = sin(t * TAU * frequencyA + phaseA);

    float phaseB = hash01(seed * 769u) * TAU;
    float frequencyB = randomRange(seed * 367u, 2.20, 3.30);
    float amplitudeB = 0.035;
    float waveB = sin(t * TAU * frequencyB + phaseB);

    return 1.0 + waveA * amplitudeA + waveB * amplitudeB;
}

// --- height coloring (ported from spike_field.fs) ---

const float GRADIENT_HEIGHT_MIN = 0.05;
const float GRADIENT_HEIGHT_MAX = 0.60;
const float GRADIENT_STOP_MAGENTA = 0.36;
const float GRADIENT_STOP_RED_ORANGE = 0.68;

const vec3 deepPurple = vec3(0.11, 0.05, 0.30);
const vec3 magenta = vec3(0.66, 0.08, 0.55);
const vec3 redOrange = vec3(0.96, 0.26, 0.11);
const vec3 paleYellow = vec3(1.00, 0.91, 0.36);

vec3 heightColor(float height) {
    float t = clamp((height - GRADIENT_HEIGHT_MIN) / (GRADIENT_HEIGHT_MAX - GRADIENT_HEIGHT_MIN), 0.0, 1.0);

    if (t < GRADIENT_STOP_MAGENTA) {
        return mix(deepPurple, magenta, t / GRADIENT_STOP_MAGENTA);
    }
    if (t < GRADIENT_STOP_RED_ORANGE) {
        float localT = (t - GRADIENT_STOP_MAGENTA) / (GRADIENT_STOP_RED_ORANGE - GRADIENT_STOP_MAGENTA);
        return mix(magenta, redOrange, localT);
    }
    float localT = (t - GRADIENT_STOP_RED_ORANGE) / (1.0 - GRADIENT_STOP_RED_ORANGE);
    return mix(redOrange, paleYellow, localT);
}

// --- pi mask (from mask.png simulation) ---

float piMask(vec2 uv) {
    // uv in [0,1], approximate the pi symbol
    vec2 p = uv - 0.5;
    // Pi symbol: two vertical legs + horizontal bar on top
    float bar = smoothstep(0.12, 0.14, abs(p.y - 0.15)) * (1.0 - smoothstep(0.08, 0.10, abs(p.x)));
    float legL = smoothstep(0.12, 0.14, abs(p.x + 0.18)) * (1.0 - smoothstep(0.40, 0.42, abs(p.y + 0.05)));
    float legR = smoothstep(0.12, 0.14, abs(p.x - 0.18)) * (1.0 - smoothstep(0.40, 0.42, abs(p.y + 0.05)));
    return clamp(bar + legL + legR, 0.0, 1.0);
}

const float GROUND_HEIGHT_MIN = 0.18;
const float GROUND_HEIGHT_MAX = 0.30;
const float PI_HEIGHT_MIN = 0.40;
const float PI_HEIGHT_MAX = 0.55;

// --- per-face hue shift ---

vec3 hueShift(vec3 color, float angle) {
    float cosA = cos(angle);
    float sinA = sin(angle);
    mat3 m = mat3(
        cosA + (1.0 - cosA) / 3.0,
        (1.0 - cosA) / 3.0 - sinA * 0.57735,
        (1.0 - cosA) / 3.0 + sinA * 0.57735,
        (1.0 - cosA) / 3.0 + sinA * 0.57735,
        cosA + (1.0 - cosA) / 3.0,
        (1.0 - cosA) / 3.0 - sinA * 0.57735,
        (1.0 - cosA) / 3.0 - sinA * 0.57735,
        (1.0 - cosA) / 3.0 + sinA * 0.57735,
        cosA + (1.0 - cosA) / 3.0
    );
    return m * color;
}

void main() {
    // Flip Y for correct orientation
    vec2 uv = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);

    // Grid setup: 256x256 spikes
    vec2 gridUV = uv * 256.0;
    ivec2 cell = ivec2(floor(gridUV));
    uint seed = uint(cell.y * 256 + cell.x + 1);

    // Compute heights
    float groundHeight = randomRange(seed * 13u, GROUND_HEIGHT_MIN, GROUND_HEIGHT_MAX);
    groundHeight *= groundWobble(seed * 23u, time);

    float piH = randomRange(seed * 17u, PI_HEIGHT_MIN, PI_HEIGHT_MAX);
    piH *= piWobble(seed * 47u, time);

    float mask = piMask(uv);
    float spikeHeight = mix(groundHeight, piH, mask);

    // Color based on height
    vec3 color = heightColor(spikeHeight);

    // Per-face hue shift for variety
    vec3 n = normalize(fragNormal);
    float hueAngle = 0.0;

    if (n.x > 0.5)       hueAngle = 0.0;
    else if (n.x < -0.5) hueAngle = 1.047;
    else if (n.y > 0.5)  hueAngle = 2.094;
    else if (n.y < -0.5) hueAngle = 3.142;
    else if (n.z > 0.5)  hueAngle = 4.189;
    else if (n.z < -0.5) hueAngle = 5.236;

    color = hueShift(color, hueAngle);

    fragColor = vec4(color, 1.0);
}
