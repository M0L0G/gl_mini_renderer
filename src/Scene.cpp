#include "Scene.h"

#include "Paths.h"

#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cmath>
#include <cstdio>
#include <utility>

namespace gmin {

Scene::~Scene()
{
    shutdown();
}

void Scene::shutdown()
{
    items_.clear();
    textures_.clear();
    meshes_.clear();
    torusItem_ = -1;
}

void Scene::init()
{
    shutdown();

    auto groundTexture = std::make_unique<Texture2D>();
    const auto checkerPath = assetPath("textures/checker.png");
    if (groundTexture->loadFromFile(checkerPath.string(), true, true)) {
        std::printf("[Texture] loaded %s (%dx%d)\n", checkerPath.string().c_str(),
                    groundTexture->width(), groundTexture->height());
    } else {
        std::printf("[Texture] using procedural checker fallback\n");
        groundTexture->createCheckerboard(512, 8, glm::vec3(0.72f, 0.73f, 0.70f),
                                          glm::vec3(0.28f, 0.32f, 0.35f));
    }
    auto woodTexture = std::make_unique<Texture2D>();
    woodTexture->createWood(512);
    auto stoneTexture = std::make_unique<Texture2D>();
    stoneTexture->createStone(512);
    auto jadeTexture = std::make_unique<Texture2D>();
    jadeTexture->createJade(512);

    const Texture2D* groundTex = groundTexture.get();
    const Texture2D* woodTex = woodTexture.get();
    const Texture2D* stoneTex = stoneTexture.get();
    const Texture2D* jadeTex = jadeTexture.get();
    textures_.push_back(std::move(groundTexture));
    textures_.push_back(std::move(woodTexture));
    textures_.push_back(std::move(stoneTexture));
    textures_.push_back(std::move(jadeTexture));

    auto groundMesh = std::make_unique<Mesh>(geometry::plane(60.0f, 5.0f, 32));
    auto boxMesh = std::make_unique<Mesh>(geometry::box(1.0f));
    auto cylinderMesh = std::make_unique<Mesh>(geometry::cylinder(0.5f, 1.0f, 40));
    auto torusMesh = std::make_unique<Mesh>(geometry::torus(1.7f, 0.5f, 64, 32));

    const Mesh* ground = groundMesh.get();
    const Mesh* box = boxMesh.get();
    const Mesh* cylinder = cylinderMesh.get();
    const Mesh* torus = torusMesh.get();
    meshes_.push_back(std::move(groundMesh));
    meshes_.push_back(std::move(boxMesh));
    meshes_.push_back(std::move(cylinderMesh));
    meshes_.push_back(std::move(torusMesh));

    DrawItem groundItem;
    groundItem.name = "ground";
    groundItem.mesh = ground;
    groundItem.texture = groundTex;
    groundItem.model = glm::mat4(1.0f);
    groundItem.material.specularColor = glm::vec3(0.12f);
    groundItem.material.shininess = 24.0f;
    groundItem.castsShadow = false;
    items_.push_back(groundItem);

    DrawItem pedestal;
    pedestal.name = "pedestal";
    pedestal.mesh = cylinder;
    pedestal.texture = stoneTex;
    pedestal.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f))
                   * glm::scale(glm::mat4(1.0f), glm::vec3(3.2f, 0.6f, 3.2f));
    pedestal.material.specularColor = glm::vec3(0.22f);
    pedestal.material.shininess = 36.0f;
    pedestal.castsShadow = true;
    items_.push_back(pedestal);

    DrawItem torusItem;
    torusItem.name = "torus";
    torusItem.mesh = torus;
    torusItem.texture = jadeTex;
    torusItem.material.specularColor = glm::vec3(0.85f, 0.72f, 0.35f);
    torusItem.material.shininess = 96.0f;
    torusItem.castsShadow = true;
    torusItem_ = static_cast<int>(items_.size());
    items_.push_back(torusItem);

    const std::array<float, 6> pillarAngles = {18.0f, 78.0f, 138.0f, 198.0f, 258.0f, 318.0f};
    for (size_t i = 0; i < pillarAngles.size(); ++i) {
        const float angle = glm::radians(pillarAngles[i]);
        const float x = std::cos(angle) * 6.4f;
        const float z = std::sin(angle) * 6.4f;
        DrawItem pillar;
        pillar.name = "pillar_" + std::to_string(i);
        pillar.mesh = cylinder;
        pillar.texture = stoneTex;
        pillar.model = glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.0f, z))
                     * glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f))
                     * glm::scale(glm::mat4(1.0f), glm::vec3(0.9f, 2.8f, 0.9f));
        pillar.material.specularColor = glm::vec3(0.18f);
        pillar.material.shininess = 40.0f;
        pillar.castsShadow = true;
        items_.push_back(pillar);
    }

    struct CrateDef {
        glm::vec3 position;
        float size;
        float rotation;
    };
    const std::array<CrateDef, 4> crates = {{
        {{-3.8f, 0.0f, 0.6f}, 1.25f, 26.0f},
        {{-1.0f, 0.0f, -3.8f}, 1.5f, -18.0f},
        {{3.0f, 0.0f, 2.3f}, 2.05f, 14.0f},
        {{4.1f, 0.0f, -1.2f}, 1.1f, 40.0f},
    }};
    for (size_t i = 0; i < crates.size(); ++i) {
        const auto& crate = crates[i];
        DrawItem item;
        item.name = "crate_" + std::to_string(i);
        item.mesh = box;
        item.texture = woodTex;
        item.model = glm::translate(glm::mat4(1.0f),
                                    crate.position + glm::vec3(0.0f, crate.size * 0.5f, 0.0f))
                   * glm::rotate(glm::mat4(1.0f), glm::radians(crate.rotation),
                                 glm::vec3(0.0f, 1.0f, 0.0f))
                   * glm::scale(glm::mat4(1.0f), glm::vec3(crate.size));
        item.material.specularColor = glm::vec3(0.18f, 0.12f, 0.06f);
        item.material.shininess = 28.0f;
        item.castsShadow = true;
        items_.push_back(item);
    }

    light_.direction = glm::normalize(glm::vec3(-0.55f, -0.82f, -0.38f));

    std::string objError;
    const auto objPath = assetPath("models/torus.obj");
    MeshData torusData = geometry::loadObj(objPath.string(), objError);
    if (!torusData.vertices.empty()) {
        setTorusFromObj(torusData);
        std::printf("[Model] loaded %s (%zu vertices, %zu triangles)\n",
                    objPath.string().c_str(), torusData.vertices.size(),
                    torusData.indices.size() / 3);
    } else {
        std::fprintf(stderr, "[Model] OBJ load failed, using procedural torus fallback: %s\n",
                     objError.c_str());
    }

    update(0.0f);
}

void Scene::createTorusItem(const MeshData& data)
{
    if (torusItem_ < 0 || torusItem_ >= static_cast<int>(items_.size())) {
        return;
    }
    auto mesh = std::make_unique<Mesh>(data);
    items_[static_cast<size_t>(torusItem_)].mesh = mesh.get();
    meshes_.push_back(std::move(mesh));
    update(spinAngle_ / 0.22f);
}

void Scene::setTorusFromObj(const MeshData& data)
{
    createTorusItem(data);
}

void Scene::update(float elapsedSeconds)
{
    spinAngle_ = elapsedSeconds * 0.22f;
    if (torusItem_ < 0 || torusItem_ >= static_cast<int>(items_.size())) {
        return;
    }
    DrawItem& torusItem = items_[static_cast<size_t>(torusItem_)];
    torusItem.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 3.55f, 0.0f))
                    * glm::rotate(glm::mat4(1.0f), spinAngle_, glm::vec3(0.0f, 1.0f, 0.0f))
                    * glm::rotate(glm::mat4(1.0f), glm::radians(22.0f), glm::vec3(1.0f, 0.0f, 0.0f))
                    * glm::scale(glm::mat4(1.0f), glm::vec3(1.12f));
}

} // namespace gmin
