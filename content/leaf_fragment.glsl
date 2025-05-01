#version 330

layout(location = 0) out vec4 color;

uniform sampler2D textureSampler;
uniform float sssBacksideAmount;
uniform vec4 lightColor;
uniform vec3 lightPosition;
uniform vec3 cameraPosition;
uniform vec3 leafColor;

in vec3 vPosition;
in vec3 vNormal;
in vec4 vColor;
in vec4 vTCoord;

void main() 
{
    vec3 lightDir = normalize(lightPosition-vPosition);
    vec3 camDir = normalize(cameraPosition-vPosition);
    vec3 normal = normalize(vNormal);

    // Ordinary phong diffuse model with fake SSS
    float angleContribution = dot(normal, lightDir);
    float directLightDot = clamp(angleContribution, 0.0, 1.0);
    float sssLightDot = abs(angleContribution);
    float backSideFactor = abs(clamp(angleContribution, -1.0f, 0.0f));
    float directLightContribution = mix(directLightDot, 0.75 + 0.25*sssLightDot, sssBacksideAmount);
    float lightStrength = lightColor.a;
    vec3 diffuseLight = lightStrength * directLightContribution * lightColor.rgb;
    
    vec3 ambientLight = vec3(0.2);
    vec3 specularLight = vec3(0.0); // Specular not yet implemented
    vec3 totalLightContribution = ambientLight + diffuseLight + specularLight;

    // Use the leafColor uniform directly
    color = vec4(leafColor * totalLightContribution, 1.0);
}
