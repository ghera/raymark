#version $GLSL_VERSION

#ifdef GL_ES
precision mediump float;
#endif

uniform float time;
uniform vec2 resolution;

out vec4 fragColor;

const float TAU = 6.283185;

void main() {
    vec2 uv = gl_FragCoord.xy / resolution;

    vec3 darkPurple = vec3(0.105, 0.055, 0.165);
    vec3 deepDarkPurple = vec3(0.020, 0.035, 0.080);

    float frequency = 0.2;
    float phase = TAU / 2.0;
    float amplitude = 0.5;
    float wave = sin(time * TAU * frequency + phase) * amplitude; // [-0.5, 0.5]

    float t = smoothstep(0.0, 1.0, uv.y + wave);

    vec3 color = mix(deepDarkPurple, darkPurple, t);

    fragColor = vec4(color, 1.0);
}
