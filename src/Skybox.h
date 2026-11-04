#pragma once

#include "Shader.h"

#include <glm/glm.hpp>

namespace gmin {

class Camera;

class Skybox {
public:
    Skybox() = default;
    ~Skybox();

    void init(const glm::vec3& sunDirection);
    void shutdown();
    void render(const Camera& camera);

private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint cubemap_ = 0;
    Shader shader_;

    void createCubemap(const glm::vec3& sunDirection);
};

} // namespace gmin
