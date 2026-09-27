#version 450
// Unlit lines: the interpolated vertex colour (already tinted) is the output.
layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = fragColor;
}
