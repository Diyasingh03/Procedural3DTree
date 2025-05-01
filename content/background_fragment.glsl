#version 330

layout(location = 0) out vec4 color;

in vec3 vPosition;
in vec3 vNormal;
in vec4 vColor;
in vec4 vTCoord;
in vec2 groundUV;

void main()
{
    // Original sky gradient
    vec4 color1 = vec4(0.5, 0.8, 0.9, 1.0);
    vec4 color2 = vec4(0.1, 0.2, 0.3, 1.0);
    vec4 skyColor = mix(color1, color2, vTCoord.y);

    // Ground texture for the mesh
    if (vPosition.y <= 0.01) { // If it's the ground mesh
        float grassPattern = sin(groundUV.x * 50.0) * sin(groundUV.y * 50.0);
        float grassDetail = sin(groundUV.x * 200.0) * sin(groundUV.y * 200.0);
        
        vec3 grassColor = mix(
            vec3(0.2, 0.4, 0.1),  // Dark green
            vec3(0.3, 0.5, 0.2),  // Light green
            grassPattern * 0.5 + 0.5
        );
        
        // Add fine detail
        grassColor += vec3(0.05) * grassDetail;
        
        // Add some brown patches
        float brownPatches = smoothstep(0.6, 0.8, grassPattern);
        grassColor = mix(grassColor, vec3(0.3, 0.2, 0.1), brownPatches * 0.15);
        
        // Add distance fade
        float distanceFade = smoothstep(0.0, 5.0, length(vPosition.xz));
        vec3 fadeColor = vec3(0.2, 0.3, 0.2); // Darker green in the distance
        grassColor = mix(grassColor, fadeColor, distanceFade * 0.3);
        
        color = vec4(grassColor * 0.8, 1.0); // Slightly darken the grass
    } else {
        color = skyColor;
    }
}