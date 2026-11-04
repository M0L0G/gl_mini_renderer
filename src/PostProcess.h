#pragma once

#include "Shader.h"

namespace gmin {

class PostProcess {
public:
    PostProcess() = default;
    ~PostProcess();

    void init();
    void shutdown();
    void resize(int width, int height);

    void begin();
    void end();
    void apply(int mode);

    int width() const { return width_; }
    int height() const { return height_; }

private:
    GLuint fbo_ = 0;
    GLuint colorTexture_ = 0;
    GLuint depthRenderbuffer_ = 0;
    GLuint emptyVao_ = 0;
    Shader shader_;
    int width_ = 0;
    int height_ = 0;

    void destroyFramebuffer();
};

} // namespace gmin
