#include "Renderer.h"

#include "Camera.h"
#include "GlCommon.h"
#include "Paths.h"
#include "Scene.h"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>

namespace gmin {
Renderer::~Renderer()
{
    shutdown();
}

void Renderer::init(const Scene& scene)
{
    litShader_.load(assetPath("shaders/lit.vert").string(),
                    assetPath("shaders/lit.frag").string());
    shadowShader_.load(assetPath("shaders/shadow.vert").string(),
                       assetPath("shaders/shadow.frag").string());
    shadowMap_.init(2048);
    postProcess_.init();
    // The sun sits opposite to the direction in which the light travels.
    skybox_.init(glm::normalize(-scene.light().direction));
    checkGLError("Renderer::init");
}

void Renderer::shutdown()
{
    skybox_.shutdown();
    postProcess_.shutdown();
    shadowMap_.shutdown();
    shadowShader_ = Shader();
    litShader_ = Shader();
}

void Renderer::resize(int width, int height)
{
    postProcess_.resize(width, height);
}

void Renderer::render(const Scene& scene, const Camera& camera, int width, int height,
                      const RenderSettings& settings)
{
    const DirectionalLight& light = scene.light();
    const glm::vec3 sceneCenter = scene.sceneCenter();
    const float shadowRadius = scene.shadowRadius();

    // Directional light shadow matrix: an orthographic camera positioned
    // opposite to the light travel direction, fitted around the scene bounds.
    const glm::vec3 lightPosition = sceneCenter - light.direction * 30.0f;
    const glm::mat4 lightView = glm::lookAt(lightPosition, sceneCenter,
                                            glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 lightProjection = glm::ortho(-shadowRadius, shadowRadius,
                                                 -shadowRadius, shadowRadius,
                                                 1.0f, 64.0f);
    lightSpaceMatrix_ = lightProjection * lightView;

    // --- shadow depth pass -------------------------------------------------
    if (settings.shadowsEnabled) {
        shadowMap_.begin();
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.5f, 2.5f);

        shadowShader_.use();
        shadowShader_.setMat4("uLightSpaceMatrix", lightSpaceMatrix_);
        for (const DrawItem& item : scene.items()) {
            if (item.mesh == nullptr || !item.castsShadow) {
                continue;
            }
            shadowShader_.setMat4("uModel", item.model);
            item.mesh->draw();
        }

        glDisable(GL_POLYGON_OFFSET_FILL);
        glCullFace(GL_BACK);
        shadowMap_.end(width, height);
    }

    // --- main scene pass into the HDR post-processing target -----------------
    postProcess_.begin();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    const glm::mat4 view = camera.viewMatrix();
    const glm::mat4 projection = camera.projectionMatrix();
    const glm::vec3 viewPosition = camera.position();

    skybox_.render(camera);

    glPolygonMode(GL_FRONT_AND_BACK, settings.wireframe ? GL_LINE : GL_FILL);

    litShader_.use();
    litShader_.setMat4("uView", view);
    litShader_.setMat4("uProj", projection);
    litShader_.setVec3("uViewPos", viewPosition);
    litShader_.setVec3("uLightDir", light.direction);
    litShader_.setVec3("uLightColor", light.color);
    litShader_.setFloat("uLightIntensity", light.intensity);
    litShader_.setVec3("uAmbientColor", light.ambientColor);
    litShader_.setVec3("uFogColor", glm::vec3(0.46f, 0.57f, 0.70f));
    litShader_.setFloat("uFogDensity", 0.022f);
    litShader_.setInt("uAlbedo", 0);
    litShader_.setInt("uShadowMap", 1);
    litShader_.setBool("uShadowEnabled", settings.shadowsEnabled);
    litShader_.setMat4("uLightSpaceMatrix", lightSpaceMatrix_);
    litShader_.setVec2("uShadowTexelSize",
                       glm::vec2(1.0f / static_cast<float>(shadowMap_.size())));
    shadowMap_.bindTexture(1);
    litShader_.setInt("uUseTexture", 1);

    for (const DrawItem& item : scene.items()) {
        if (item.mesh == nullptr) {
            continue;
        }
        litShader_.setMat4("uModel", item.model);
        litShader_.setMat3("uNormalMatrix",
                           glm::transpose(glm::inverse(glm::mat3(item.model))));
        litShader_.setVec3("uBaseColor", item.material.baseColor);
        litShader_.setVec3("uSpecularColor", item.material.specularColor);
        litShader_.setFloat("uShininess", item.material.shininess);
        if (item.texture != nullptr) {
            item.texture->bind(0);
            litShader_.setInt("uUseTexture", 1);
        } else {
            litShader_.setInt("uUseTexture", 0);
        }
        item.mesh->draw();
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);

    postProcess_.end();
    postProcess_.apply(settings.postMode);

    checkGLError("Renderer::render");
}

} // namespace gmin
