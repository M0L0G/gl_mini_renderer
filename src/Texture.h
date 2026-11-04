#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace gmin {

class Texture2D {
public:
    Texture2D() = default;
    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    // Try to load an image through stb_image. Returns false on failure.
    bool loadFromFile(const std::string& path, bool srgb = true, bool flipY = true);
    void createCheckerboard(int size, int cells, const glm::vec3& colorA,
                            const glm::vec3& colorB);
    void createWood(int size);
    void createStone(int size);
    void createJade(int size);

    void bind(GLuint unit = 0) const;
    GLuint id() const { return texture_; }
    int width() const { return width_; }
    int height() const { return height_; }

private:
    GLuint texture_ = 0;
    int width_ = 0;
    int height_ = 0;

    void upload(const unsigned char* pixels, int width, int height, bool srgb);
    void destroy();
};

} // namespace gmin
