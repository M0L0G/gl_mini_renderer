#pragma once

#include <glm/glm.hpp>

namespace gmin {

class Camera {
public:
    Camera() = default;

    void reset(const glm::vec3& position, const glm::vec3& target);
    void setAspect(float aspect) { aspect_ = aspect; }
    float aspect() const { return aspect_; }

    void move(float forwardAxis, float rightAxis, float upAxis, float dt, bool fast);
    void look(float xOffset, float yOffset, float sensitivity = 0.1f);
    void zoom(float scrollOffset);
    void setFov(float degrees);

    glm::vec3 position() const { return position_; }
    glm::vec3 front() const;
    glm::vec3 right() const;
    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix() const;
    float fov() const { return fov_; }
    float nearPlane() const { return nearPlane_; }
    float farPlane() const { return farPlane_; }

private:
    glm::vec3 position_{0.0f, 3.0f, 8.0f};
    float yaw_ = -90.0f;
    float pitch_ = 0.0f;
    float fov_ = 45.0f;
    float aspect_ = 1280.0f / 720.0f;
    float nearPlane_ = 0.1f;
    float farPlane_ = 200.0f;
    float moveSpeed_ = 6.0f;
};

} // namespace gmin
