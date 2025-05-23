#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec4 vertexColor;
layout(location = 3) in vec4 vertexTCoord;

uniform mat4 mvp;

out vec3 vPosition;
out vec3 vNormal;
out vec4 vColor;
out vec4 vTCoord;
out vec2 groundUV;

void main()
{
    gl_Position = mvp * vec4(vertexPosition, 1.0);
    vPosition = vertexPosition;
    vNormal = vertexNormal;
    vColor = vertexColor;
    vTCoord = vertexTCoord;
    groundUV = vertexPosition.xz * 0.5; // Scale UV coordinates for ground texture
}