#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <stdexcept>
#include <string>

namespace gmin {

inline void checkGLError(const char* where)
{
    GLenum error = glGetError();
    while (error != GL_NO_ERROR) {
        const char* name = "UNKNOWN";
        switch (error) {
        case GL_INVALID_ENUM: name = "GL_INVALID_ENUM"; break;
        case GL_INVALID_VALUE: name = "GL_INVALID_VALUE"; break;
        case GL_INVALID_OPERATION: name = "GL_INVALID_OPERATION"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: name = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
        case GL_OUT_OF_MEMORY: name = "GL_OUT_OF_MEMORY"; break;
        default: break;
        }
        std::fprintf(stderr, "[GL] %s at %s\n", name, where);
        error = glGetError();
    }
}

[[noreturn]] inline void fail(const std::string& message, const char* where)
{
    throw std::runtime_error(message + " (" + where + ")");
}

} // namespace gmin
