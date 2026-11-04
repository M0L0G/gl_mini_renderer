#version 330 core

in vec3 vWorldPosition;
in vec3 vNormal;
in vec2 vUV;

uniform sampler2D uAlbedo;
uniform sampler2DShadow uShadowMap;
uniform bool uUseTexture;
uniform vec3 uBaseColor;
uniform vec3 uSpecularColor;
uniform float uShininess;

uniform vec3 uViewPos;
uniform vec3 uLightDir;      // direction the light travels, from sun to scene
uniform vec3 uLightColor;
uniform float uLightIntensity;
uniform vec3 uAmbientColor;
uniform vec3 uFogColor;
uniform float uFogDensity;

uniform bool uShadowEnabled;
uniform mat4 uLightSpaceMatrix;
uniform vec2 uShadowTexelSize;

out vec4 FragColor;

float directionalShadow(vec4 lightSpacePosition, vec3 normal, vec3 lightDirection)
{
    vec3 projected = lightSpacePosition.xyz / lightSpacePosition.w;
    projected = projected * 0.5 + 0.5;
    if (projected.z > 1.0 || projected.z < 0.0 ||
        projected.x < 0.0 || projected.x > 1.0 ||
        projected.y < 0.0 || projected.y > 1.0) {
        return 1.0;
    }

    float bias = max(0.0018 * (1.0 - dot(normal, lightDirection)), 0.00055);
    float sum = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 offset = vec2(float(x), float(y)) * uShadowTexelSize;
            sum += texture(uShadowMap, vec3(projected.xy + offset, projected.z - bias));
        }
    }
    return sum / 9.0;
}

void main()
{
    vec3 albedo = uUseTexture ? texture(uAlbedo, vUV).rgb * uBaseColor : uBaseColor;

    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uViewPos - vWorldPosition);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float shadow = 1.0;
    if (uShadowEnabled) {
        shadow = directionalShadow(uLightSpaceMatrix * vec4(vWorldPosition, 1.0), N, L);
    }

    vec3 ambient = uAmbientColor * albedo;
    vec3 diffuse = uLightColor * uLightIntensity * albedo * NdotL * shadow;

    float specularStrength = 0.0;
    if (NdotL > 0.0) {
        specularStrength = pow(max(dot(N, H), 0.0), uShininess) * shadow;
    }
    vec3 specular = uLightColor * uLightIntensity * uSpecularColor
                  * specularStrength * NdotL;

    vec3 color = ambient + diffuse + specular;

    // Distance fog hides the far ground horizon and unifies it with the sky.
    float fogDistance = length(vWorldPosition - uViewPos) * uFogDensity;
    float fogFactor = 1.0 - exp(-fogDistance * fogDistance);
    color = mix(color, uFogColor, clamp(fogFactor, 0.0, 1.0));

    FragColor = vec4(color, 1.0);
}
