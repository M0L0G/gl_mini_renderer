#version 330 core

in vec2 vUV;

uniform sampler2D uSceneColor;
uniform int uMode;
uniform float uExposure;

out vec4 FragColor;

void main()
{
    vec3 hdr = texture(uSceneColor, vUV).rgb;

    // Simple filmic tone mapping in linear space, then gamma-correct output.
    vec3 mapped = 1.0 - exp(-hdr * uExposure);
    vec3 color = pow(clamp(mapped, 0.0, 1.0), vec3(1.0 / 2.2));

    if (uMode == 1) {
        float gray = dot(color, vec3(0.299, 0.587, 0.114));
        color = vec3(gray);
    } else if (uMode == 2) {
        color = 1.0 - color;
    }

    FragColor = vec4(color, 1.0);
}
