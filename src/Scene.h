#pragma once

#include "Mesh.h"
#include "Texture.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

namespace gmin {

struct Material {
    glm::vec3 baseColor{1.0f};
    glm::vec3 specularColor{0.25f};
    float shininess = 32.0f;
};

struct DrawItem {
    std::string name;
    const Mesh* mesh = nullptr;
    const Texture2D* texture = nullptr;
    glm::mat4 model{1.0f};
    Material material;
    bool castsShadow = true;
};

struct DirectionalLight {
    // Direction in which the light travels (from sun towards the scene).
    glm::vec3 direction{-0.55f, -0.8f, -0.35f};
    glm::vec3 color{1.0f, 0.93f, 0.82f};
    glm::vec3 ambientColor{0.24f, 0.30f, 0.42f};
    float intensity = 1.7f;
};

class Scene {
public:
    Scene() = default;
    ~Scene();

    void init();
    void shutdown();
    void update(float elapsedSeconds);

    const std::vector<DrawItem>& items() const { return items_; }
    const DirectionalLight& light() const { return light_; }
    glm::vec3 sceneCenter() const { return sceneCenter_; }
    float shadowRadius() const { return shadowRadius_; }

    void setTorusFromObj(const MeshData& data);

private:
    std::vector<std::unique_ptr<Mesh>> meshes_;
    std::vector<std::unique_ptr<Texture2D>> textures_;
    std::vector<DrawItem> items_;
    DirectionalLight light_;
    glm::vec3 sceneCenter_{0.0f, 1.8f, 0.0f};
    float shadowRadius_ = 10.0f;
    int torusItem_ = -1;
    float spinAngle_ = 0.0f;

    void createTorusItem(const MeshData& data);
};

} // namespace gmin
