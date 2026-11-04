#version 330 core

in vec3 vDirection;

uniform samplerCube uSky;

out vec4 FragColor;

void main()
{
    FragColor = vec4(texture(uSky, normalize(vDirection)).rgb, 1.0);
}
