#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 Tangent;
in vec3 Bitangent;

out vec4 FragColor;

uniform sampler2D texture1;
uniform sampler2D normalMap;
uniform bool isLeaf;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    // Normal mapping
    vec3 normal = normalize(Normal);
    if (!isLeaf) {
        vec3 tangent = normalize(Tangent);
        vec3 bitangent = normalize(Bitangent);
        mat3 TBN = mat3(tangent, bitangent, normal);
        
        vec3 normalMapSample = texture(normalMap, TexCoord).rgb * 2.0 - 1.0;
        normal = normalize(TBN * normalMapSample);
    }

    // Lighting
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    
    float ambientStrength = 0.1;
    float diffuseStrength = max(dot(normal, lightDir), 0.0);
    float specularStrength = 0.5;
    float shininess = 32.0;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    
    vec3 ambient = ambientStrength * vec3(1.0);
    vec3 diffuse = diffuseStrength * vec3(1.0);
    vec3 specular = specularStrength * spec * vec3(1.0);
    
    // Texture sampling
    vec4 texColor = texture(texture1, TexCoord);
    if (isLeaf) {
        // Add some translucency for leaves
        float backlight = max(dot(normal, -lightDir), 0.0);
        texColor.rgb += backlight * vec3(0.2, 0.4, 0.1);
    }
    
    // Final color
    vec3 result = (ambient + diffuse + specular) * texColor.rgb;
    FragColor = vec4(result, texColor.a);
}