#version 330 core

layout (location = 0) in vec3 position;

uniform mat4 xform;

void main() {
    gl_Position = xform * vec4(position, 1.0);
}