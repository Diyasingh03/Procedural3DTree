#version 130

in vec3 position;
in vec3 normal;
in vec2 texCoord;

out vec3 fragNormal;
out vec2 fragTexCoord;
out vec3 fragPosition;

uniform mat4 mvp;
uniform mat4 model;
uniform vec3 cameraPosition;

void main()
{
    // Transform position to world space
    vec4 worldPos = model * vec4(position, 1.0);
    fragPosition = vec3(worldPos);
    
    // Transform normal to world space without translation
    fragNormal = mat3(model) * normal;
    
    // Pass texture coordinates
    fragTexCoord = texCoord;
    
    // Final position
    gl_Position = mvp * vec4(position, 1.0);
} 