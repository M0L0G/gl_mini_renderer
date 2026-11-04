#pragma once

#include <glad/gl.h>

namespace gmin {

class ShadowMap {
public:
    ShadowMap() = default;
    ~ShadowMap();

    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    void init(int size);
    void shutdown();
    void begin();
    void end(int framebufferWidth, int framebufferHeight);
    void bindTexture(GLuint unit) const;

    GLuint texture() const { return texture_; }
    int size() const { return size_; }

private:
    GLuint fbo_ = 0;
    GLuint texture_ = 0;
    int size_ = 0;
};

} // namespace gmin
