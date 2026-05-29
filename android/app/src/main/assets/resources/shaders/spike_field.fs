#version $GLSL_VERSION

#ifdef GL_ES
precision mediump float;
#endif

in float spikeHeight;

out vec4 fragColor;

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

void main() {
    vec3 color = heightColor(spikeHeight);

    fragColor = vec4(color, 1.0);
}
