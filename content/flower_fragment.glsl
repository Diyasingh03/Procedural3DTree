#version 130

in vec3 fragNormal;
in vec2 fragTexCoord;
in vec3 fragPosition;

out vec4 fragColor;

uniform sampler2D textureSampler;
uniform vec3 flowerColor;
uniform vec3 lightPosition;
uniform vec3 cameraPosition;

void main()
{
    // Normalize vectors
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(lightPosition - fragPosition);
    vec3 viewDir = normalize(cameraPosition - fragPosition);
    
    // Calculate lighting components
    float ambient = 0.6;  // Increased ambient light
    float diffuse = max(dot(normal, lightDir), 0.0);
    
    // Add specular highlight
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);  // Increased shininess
    float specular = 0.4 * spec;  // Increased specular intensity
    
    // Sample texture
    vec4 texColor = texture(textureSampler, fragTexCoord);
    
    // Add some color variation based on position
    float colorVariation = 0.1 * sin(fragPosition.x * 10.0) * cos(fragPosition.z * 10.0);
    vec3 variedColor = flowerColor * (1.0 + colorVariation);
    
    // Combine lighting with flower color and texture
    vec3 finalColor = (ambient + diffuse + specular) * variedColor * texColor.rgb;
    
    // Make flowers fully opaque
    float alpha = 1.0;  // Fully opaque
    
    // Output final color
    fragColor = vec4(finalColor, alpha);
} 