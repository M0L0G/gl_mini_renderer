#include "Skybox.h"

#include "Camera.h"
#include "GlCommon.h"
#include "Paths.h"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace gmin {
namespace {

constexpr int kCubeFaceSize = 256;

glm::vec3 faceDirection(int face, float u, float v)
{
    const float x = u * 2.0f - 1.0f;
    const float y = v * 2.0f - 1.0f;
    glm::vec3 direction(0.0f);
    switch (face) {
    case 0: direction = glm::vec3(1.0f, -y, -x); break;  // +X
    case 1: direction = glm::vec3(-1.0f, -y, x); break;  // -X
    case 2: direction = glm::vec3(x, 1.0f, y); break;    // +Y
    case 3: direction = glm::vec3(x, -1.0f, -y); break;  // -Y
    case 4: direction = glm::vec3(x, -y, 1.0f); break;   // +Z
    case 5: direction = glm::vec3(-x, -y, -1.0f); break; // -Z
    default: break;
    }
    return glm::normalize(direction);
}

glm::vec3 skyColor(const glm::vec3& direction, const glm::vec3& sunDirection)
{
    const glm::vec3 zenith(0.018f, 0.055f, 0.18f);
    const glm::vec3 horizon(0.48f, 0.58f, 0.72f);
    const glm::vec3 groundHaze(0.035f, 0.045f, 0.06f);
    const glm::vec3 warmHaze(0.92f, 0.68f, 0.42f);
    const glm::vec3 sunColor(1.0f, 0.72f, 0.36f);

    const float height = direction.y;
    glm::vec3 color;
    if (height >= 0.0f) {
        const float t = std::pow(std::min(height, 1.0f), 0.55f);
        color = glm::mix(horizon, zenith, t);
    } else {
        const float t = std::clamp(-height / 0.55f, 0.0f, 1.0f);
        color = glm::mix(horizon, groundHaze, t);
    }

    // Warm scattering band around the horizon.
    const float horizonBand = std::exp(-std::abs(height) * 9.0f);
    color = glm::mix(color, warmHaze, horizonBand * 0.22f);

    // Baked sun glow/disc; values above 1.0 survive because the cubemap is HDR.
    const float sunDot = std::max(glm::dot(direction, sunDirection), 0.0f);
    const float glow = std::pow(sunDot, 14.0f) * 0.55f + std::pow(sunDot, 120.0f) * 2.2f;
    const float disc = std::pow(std::max(sunDot, 0.0f), 900.0f);
    color += sunColor * glow;
    color += sunColor * disc * 3.5f;

    return glm::max(color, glm::vec3(0.0f));
}

void buildCubeVertices(std::vector<float>& vertices)
{
    // 6 faces, 2 triangles each, positions only.
    const float p[8][3] = {
        {-1.0f, -1.0f, -1.0f}, {1.0f, -1.0f, -1.0f},
        {1.0f, 1.0f, -1.0f},   {-1.0f, 1.0f, -1.0f},
        {-1.0f, -1.0f, 1.0f},  {1.0f, -1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},    {-1.0f, 1.0f, 1.0f},
    };
    const int faces[6][4] = {
        {4, 5, 6, 7}, // +Z
        {1, 0, 3, 2}, // -Z
        {5, 1, 2, 6}, // +X
        {0, 4, 7, 3}, // -X
        {7, 6, 2, 3}, // +Y
        {0, 1, 5, 4}, // -Y
    };
    vertices.clear();
    for (const auto& face : faces) {
        const int indices[6] = {face[0], face[1], face[2], face[0], face[2], face[3]};
        for (int index : indices) {
            vertices.push_back(p[index][0]);
            vertices.push_back(p[index][1]);
            vertices.push_back(p[index][2]);
        }
    }
}

} // namespace

Skybox::~Skybox()
{
    shutdown();
}

void Skybox::init(const glm::vec3& sunDirection)
{
    shutdown();
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    shader_.load(assetPath("shaders/skybox.vert").string(),
                 assetPath("shaders/skybox.frag").string());

    std::vector<float> vertices;
    buildCubeVertices(vertices);
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    createCubemap(glm::normalize(sunDirection));
    checkGLError("Skybox::init");
}

void Skybox::shutdown()
{
    if (cubemap_ != 0) {
        glDeleteTextures(1, &cubemap_);
        cubemap_ = 0;
    }
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    shader_ = Shader();
}

void Skybox::createCubemap(const glm::vec3& sunDirection)
{
    glGenTextures(1, &cubemap_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    std::vector<float> pixels(static_cast<size_t>(kCubeFaceSize) * kCubeFaceSize * 4);
    for (int face = 0; face < 6; ++face) {
        for (int y = 0; y < kCubeFaceSize; ++y) {
            for (int x = 0; x < kCubeFaceSize; ++x) {
                const float u = (static_cast<float>(x) + 0.5f) / kCubeFaceSize;
                const float v = (static_cast<float>(y) + 0.5f) / kCubeFaceSize;
                const glm::vec3 direction = faceDirection(face, u, v);
                const glm::vec3 color = skyColor(direction, sunDirection);
                const size_t index = (static_cast<size_t>(y) * kCubeFaceSize + x) * 4;
                pixels[index + 0] = color.r;
                pixels[index + 1] = color.g;
                pixels[index + 2] = color.b;
                pixels[index + 3] = 1.0f;
            }
        }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGBA16F,
                     kCubeFaceSize, kCubeFaceSize, 0, GL_RGBA, GL_FLOAT, pixels.data());
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

void Skybox::render(const Camera& camera)
{
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    shader_.use();
    const glm::mat4 view = camera.viewMatrix();
    const glm::mat4 rotationOnly = glm::mat4(glm::mat3(view));
    shader_.setMat4("uViewRotation", rotationOnly);
    shader_.setMat4("uProj", camera.projectionMatrix());
    shader_.setInt("uSky", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

} // namespace gmin
