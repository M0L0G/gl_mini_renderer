#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace gmin {

void Camera::reset(const glm::vec3& position, const glm::vec3& target)
{
    position_ = position;
    const glm::vec3 direction = glm::normalize(target - position);
    pitch_ = glm::degrees(std::asin(glm::clamp(direction.y, -1.0f, 1.0f)));
    yaw_ = glm::degrees(std::atan2(direction.z, direction.x));
}

glm::vec3 Camera::front() const
{
    const float yawRad = glm::radians(yaw_);
    const float pitchRad = glm::radians(pitch_);
    return glm::normalize(glm::vec3(
        std::cos(pitchRad) * std::cos(yawRad),
        std::sin(pitchRad),
        std::cos(pitchRad) * std::sin(yawRad)));
}

glm::vec3 Camera::right() const
{
    return glm::normalize(glm::cross(front(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::mat4 Camera::viewMatrix() const
{
    return glm::lookAt(position_, position_ + front(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::projectionMatrix() const
{
    return glm::perspective(glm::radians(fov_), std::max(aspect_, 0.01f),
                            nearPlane_, farPlane_);
}

void Camera::move(float forwardAxis, float rightAxis, float upAxis, float dt, bool fast)
{
    glm::vec3 direction(0.0f);
    direction += front() * forwardAxis;
    direction += right() * rightAxis;
    direction += glm::vec3(0.0f, 1.0f, 0.0f) * upAxis;
    if (glm::dot(direction, direction) > 0.0f) {
        direction = glm::normalize(direction);
    }
    const float speed = moveSpeed_ * (fast ? 2.5f : 1.0f);
    position_ += direction * speed * dt;
}

void Camera::look(float xOffset, float yOffset, float sensitivity)
{
    yaw_ += xOffset * sensitivity;
    pitch_ -= yOffset * sensitivity;
    pitch_ = glm::clamp(pitch_, -89.0f, 89.0f);
}

void Camera::zoom(float scrollOffset)
{
    fov_ -= scrollOffset;
    fov_ = glm::clamp(fov_, 25.0f, 85.0f);
}

void Camera::setFov(float degrees)
{
    fov_ = glm::clamp(degrees, 25.0f, 85.0f);
}

} // namespace gmin
