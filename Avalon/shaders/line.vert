#version 450
// Line list: every two vertices form one segment. Camera from set 0,
// per-object model matrix and tint from push constants (80 bytes, the
// MeshPushConstants layout).
layout(set = 0, binding = 0) uniform CameraUbo {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec4 eye;
} camera;

layout(push_constant) uniform Push {
    mat4 model;
    vec4 tint;
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = camera.viewProj * push.model * vec4(inPosition, 1.0);
    fragColor = inColor * push.tint;
}
