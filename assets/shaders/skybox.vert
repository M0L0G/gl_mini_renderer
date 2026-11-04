#version 330 core

layout(location = 0) in vec3 aPosition;

uniform mat4 uViewRotation;
uniform mat4 uProj;

out vec3 vDirection;

void main()
{
    vDirection = aPosition;
    vec4 clipPosition = uProj * uViewRotation * vec4(aPosition, 1.0);
    gl_Position = clipPosition.xyww;
}
