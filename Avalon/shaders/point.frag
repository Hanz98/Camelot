#version 450
// Round points: discard fragments outside the unit circle of the sprite.
layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    vec2 centred = gl_PointCoord * 2.0 - 1.0;
    if (dot(centred, centred) > 1.0) {
        discard;
    }
    outColor = fragColor;
}
