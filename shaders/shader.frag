#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 finalColor = fragColor;

    if (fragColor.g > fragColor.r + 0.1 && fragColor.g > fragColor.b + 0.1) {
        float gradient = clamp((fragPos.y - 15.0) / 30.0, 0.0, 1.0);
        float brightness = mix(0.7, 1.25, gradient);
        finalColor *= brightness;
    }

    outColor = vec4(finalColor, 1.0);
}
