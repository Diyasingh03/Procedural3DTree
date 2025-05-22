#version 330 core

out vec4 fragColor;

void main() {
    // Sky gradient from light blue to white
    float gradient = gl_FragCoord.y / 600.0;  // Assuming 600px height
    fragColor = mix(vec4(0.5, 0.7, 1.0, 1.0), vec4(1.0, 1.0, 1.0, 1.0), gradient);
}
