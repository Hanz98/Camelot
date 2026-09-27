#version 450
// Headlight Lambert shading: light comes from the camera so nothing is ever
// fully black, plus a small ambient term.
layout(set = 0, binding = 0) uniform CameraUbo {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec4 eye;
} camera;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 n = normalize(fragNormal);
    vec3 l = normalize(camera.eye.xyz - fragWorldPos);
    float diffuse = max(dot(n, l), 0.0);
    float ambient = 0.25;
    outColor = vec4(fragColor.rgb * (ambient + (1.0 - ambient) * diffuse), fragColor.a);
}
