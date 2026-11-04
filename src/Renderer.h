#pragma once

#include "PostProcess.h"
#include "Shader.h"
#include "ShadowMap.h"
#include "Skybox.h"

#include <glm/glm.hpp>

namespace gmin {

class Camera;
class Scene;

struct RenderSettings {
    bool shadowsEnabled = true;
    bool wireframe = false;
    int postMode = 0; // 0 = normal, 1 = grayscale, 2 = invert
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    void init(const Scene& scene);
    void shutdown();
    void resize(int width, int height);
    void render(const Scene& scene, const Camera& camera, int width, int height,
                const RenderSettings& settings);

private:
    Shader litShader_;
    Shader shadowShader_;
    ShadowMap shadowMap_;
    PostProcess postProcess_;
    Skybox skybox_;
    glm::mat4 lightSpaceMatrix_{1.0f};
};

} // namespace gmin
