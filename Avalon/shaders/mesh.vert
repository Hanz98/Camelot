#version 450
// Lit mesh: camera from set 0, per-object model matrix and tint from push
// constants (80 bytes, within the 128-byte minimum).
layout(set = 0, binding = 0) uniform CameraUbo {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec4 eye;
} camera;

layout(push_constant) uniform Push {
    mat4 model;
    vec4 color;
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragColor;

void main() {
    vec4 world = push.model * vec4(inPosition, 1.0);
    gl_Position = camera.viewProj * world;
    fragWorldPos = world.xyz;
    fragNormal = mat3(push.model) * inNormal;
    fragColor = inColor * push.color;
}
