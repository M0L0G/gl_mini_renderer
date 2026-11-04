#include "Shader.h"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace gmin {
namespace {

std::string readTextFile(const std::string& path, const char* stage)
{
    std::ifstream file(path, std::ios::in);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open " + std::string(stage) + " shader: " + path);
    }
    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

GLuint compileShader(GLenum type, const std::string& source, const std::string& path)
{
    GLuint shader = glCreateShader(type);
    const char* sourcePtr = source.c_str();
    glShaderSource(shader, 1, &sourcePtr, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string log(static_cast<size_t>(length > 0 ? length : 1), '\0');
        glGetShaderInfoLog(shader, length, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error("Shader compile failed: " + path + "\n" + log);
    }
    return shader;
}

} // namespace

Shader::~Shader()
{
    destroy();
}

Shader::Shader(Shader&& other) noexcept
    : program_(other.program_)
    , uniformCache_(std::move(other.uniformCache_))
{
    other.program_ = 0;
    other.uniformCache_.clear();
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other) {
        destroy();
        program_ = other.program_;
        uniformCache_ = std::move(other.uniformCache_);
        other.program_ = 0;
        other.uniformCache_.clear();
    }
    return *this;
}

void Shader::destroy()
{
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    uniformCache_.clear();
}

void Shader::load(const std::string& vertexPath, const std::string& fragmentPath)
{
    destroy();
    const std::string vertexSource = readTextFile(vertexPath, "vertex");
    const std::string fragmentSource = readTextFile(fragmentPath, "fragment");

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource, vertexPath);
    GLuint fragmentShader = 0;
    try {
        fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource, fragmentPath);
    } catch (...) {
        glDeleteShader(vertexShader);
        throw;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::string log(static_cast<size_t>(length > 0 ? length : 1), '\0');
        glGetProgramInfoLog(program, length, nullptr, log.data());
        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        throw std::runtime_error("Shader link failed: " + vertexPath + " + " + fragmentPath + "\n" + log);
    }

    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    program_ = program;
}

void Shader::use() const
{
    glUseProgram(program_);
}

GLint Shader::location(const std::string& name) const
{
    const auto found = uniformCache_.find(name);
    if (found != uniformCache_.end()) {
        return found->second;
    }
    GLint loc = glGetUniformLocation(program_, name.c_str());
    uniformCache_.emplace(name, loc);
    return loc;
}

void Shader::setInt(const std::string& name, int value) const
{
    glUniform1i(location(name), value);
}

void Shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(location(name), value);
}

void Shader::setBool(const std::string& name, bool value) const
{
    glUniform1i(location(name), value ? 1 : 0);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const
{
    glUniform2fv(location(name), 1, glm::value_ptr(value));
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const
{
    glUniform3fv(location(name), 1, glm::value_ptr(value));
}

void Shader::setMat3(const std::string& name, const glm::mat3& value) const
{
    glUniformMatrix3fv(location(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const
{
    glUniformMatrix4fv(location(name), 1, GL_FALSE, glm::value_ptr(value));
}

} // namespace gmin
