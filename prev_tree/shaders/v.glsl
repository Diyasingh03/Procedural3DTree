#version 330 core

layout(location = 0) in vec3 position; // Vertex position
layout(location = 1) in vec3 color;    // Color attribute for leaves/branches

uniform mat4 xform; // Transformation matrix

out vec3 fragColor;

void main() {
    fragColor = color;
    gl_Position = xform * vec4(position, 1.0); // Transform position
}
