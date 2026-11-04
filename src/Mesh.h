#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace gmin {

struct Vertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec2 uv{0.0f};
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

class Mesh {
public:
    Mesh() = default;
    explicit Mesh(const MeshData& data);
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void create(const MeshData& data);
    void draw() const;
    void bind() const;
    GLsizei indexCount() const { return indexCount_; }

private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ebo_ = 0;
    GLsizei indexCount_ = 0;
    void destroy();
};

namespace geometry {

MeshData plane(float size, float uvTiles, int subdivisions = 1);
MeshData box(float size = 1.0f);
MeshData cylinder(float radius, float height, int segments = 32);
MeshData torus(float majorRadius, float tubeRadius, int majorSegments = 48,
               int minorSegments = 24);

// Loads and triangulates an OBJ through tinyobjloader. On failure, returns an
// empty MeshData and writes a human-readable message into `error`.
MeshData loadObj(const std::string& path, std::string& error);

} // namespace geometry

} // namespace gmin
