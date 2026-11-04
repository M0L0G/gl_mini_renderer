#include "Mesh.h"

#include <glad/gl.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <tiny_obj_loader.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <map>
#include <string>
#include <tuple>
#include <utility>

namespace gmin {

Mesh::Mesh(const MeshData& data)
{
    create(data);
}

Mesh::~Mesh()
{
    destroy();
}

Mesh::Mesh(Mesh&& other) noexcept
    : vao_(other.vao_)
    , vbo_(other.vbo_)
    , ebo_(other.ebo_)
    , indexCount_(other.indexCount_)
{
    other.vao_ = 0;
    other.vbo_ = 0;
    other.ebo_ = 0;
    other.indexCount_ = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other) {
        destroy();
        vao_ = other.vao_;
        vbo_ = other.vbo_;
        ebo_ = other.ebo_;
        indexCount_ = other.indexCount_;
        other.vao_ = 0;
        other.vbo_ = 0;
        other.ebo_ = 0;
        other.indexCount_ = 0;
    }
    return *this;
}

void Mesh::destroy()
{
    if (ebo_ != 0) {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0;
    }
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    indexCount_ = 0;
}

void Mesh::create(const MeshData& data)
{
    destroy();
    if (data.vertices.empty() || data.indices.empty()) {
        return;
    }

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(data.vertices.size() * sizeof(Vertex)),
                 data.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(data.indices.size() * sizeof(uint32_t)),
                 data.indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, uv)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    indexCount_ = static_cast<GLsizei>(data.indices.size());
}

void Mesh::bind() const
{
    glBindVertexArray(vao_);
}

void Mesh::draw() const
{
    if (vao_ == 0 || indexCount_ == 0) {
        return;
    }
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
}

namespace geometry {
namespace {

void addQuad(MeshData& data, const glm::vec3& p0, const glm::vec3& p1,
             const glm::vec3& p2, const glm::vec3& p3, const glm::vec3& normal,
             float uMax = 1.0f, float vMax = 1.0f)
{
    const uint32_t base = static_cast<uint32_t>(data.vertices.size());
    data.vertices.push_back({p0, normal, {0.0f, 0.0f}});
    data.vertices.push_back({p1, normal, {uMax, 0.0f}});
    data.vertices.push_back({p2, normal, {uMax, vMax}});
    data.vertices.push_back({p3, normal, {0.0f, vMax}});
    data.indices.push_back(base + 0);
    data.indices.push_back(base + 1);
    data.indices.push_back(base + 2);
    data.indices.push_back(base + 0);
    data.indices.push_back(base + 2);
    data.indices.push_back(base + 3);
}

} // namespace

MeshData plane(float size, float uvTiles, int subdivisions)
{
    MeshData data;
    subdivisions = std::max(subdivisions, 1);
    const float half = size * 0.5f;
    const glm::vec3 up(0.0f, 1.0f, 0.0f);

    for (int iz = 0; iz <= subdivisions; ++iz) {
        for (int ix = 0; ix <= subdivisions; ++ix) {
            const float fx = static_cast<float>(ix) / static_cast<float>(subdivisions);
            const float fz = static_cast<float>(iz) / static_cast<float>(subdivisions);
            data.vertices.push_back({
                glm::vec3(-half + size * fx, 0.0f, -half + size * fz),
                up,
                glm::vec2(uvTiles * fx, uvTiles * fz)
            });
        }
    }

    const uint32_t row = static_cast<uint32_t>(subdivisions + 1);
    for (int iz = 0; iz < subdivisions; ++iz) {
        for (int ix = 0; ix < subdivisions; ++ix) {
            const uint32_t a = static_cast<uint32_t>(iz) * row + static_cast<uint32_t>(ix);
            const uint32_t b = a + 1;
            const uint32_t c = a + row + 1;
            const uint32_t d = a + row;
            data.indices.push_back(a);
            data.indices.push_back(c);
            data.indices.push_back(b);
            data.indices.push_back(a);
            data.indices.push_back(d);
            data.indices.push_back(c);
        }
    }
    return data;
}

MeshData box(float size)
{
    MeshData data;
    const float h = size * 0.5f;
    addQuad(data, {h, -h, h}, {h, -h, -h}, {h, h, -h}, {h, h, h}, {1, 0, 0});
    addQuad(data, {-h, -h, -h}, {-h, -h, h}, {-h, h, h}, {-h, h, -h}, {-1, 0, 0});
    addQuad(data, {-h, h, h}, {h, h, h}, {h, h, -h}, {-h, h, -h}, {0, 1, 0});
    addQuad(data, {-h, -h, -h}, {h, -h, -h}, {h, -h, h}, {-h, -h, h}, {0, -1, 0});
    addQuad(data, {-h, -h, h}, {h, -h, h}, {h, h, h}, {-h, h, h}, {0, 0, 1});
    addQuad(data, {h, -h, -h}, {-h, -h, -h}, {-h, h, -h}, {h, h, -h}, {0, 0, -1});
    return data;
}

MeshData cylinder(float radius, float height, int segments)
{
    MeshData data;
    segments = std::max(segments, 3);
    const float twoPi = glm::two_pi<float>();

    for (int i = 0; i < segments; ++i) {
        const float a0 = twoPi * static_cast<float>(i) / static_cast<float>(segments);
        const float a1 = twoPi * static_cast<float>(i + 1) / static_cast<float>(segments);
        const glm::vec3 n0(std::cos(a0), 0.0f, std::sin(a0));
        const glm::vec3 n1(std::cos(a1), 0.0f, std::sin(a1));
        const float u0 = static_cast<float>(i) / static_cast<float>(segments);
        const float u1 = static_cast<float>(i + 1) / static_cast<float>(segments);

        const uint32_t b0 = static_cast<uint32_t>(data.vertices.size());
        data.vertices.push_back({{n0.x * radius, 0.0f, n0.z * radius}, n0, {u0, 0.0f}});
        data.vertices.push_back({{n0.x * radius, height, n0.z * radius}, n0, {u0, 1.0f}});
        data.vertices.push_back({{n1.x * radius, 0.0f, n1.z * radius}, n1, {u1, 0.0f}});
        data.vertices.push_back({{n1.x * radius, height, n1.z * radius}, n1, {u1, 1.0f}});

        data.indices.push_back(b0 + 0);
        data.indices.push_back(b0 + 1);
        data.indices.push_back(b0 + 2);
        data.indices.push_back(b0 + 2);
        data.indices.push_back(b0 + 1);
        data.indices.push_back(b0 + 3);
    }

    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    const glm::vec3 down(0.0f, -1.0f, 0.0f);
    const uint32_t topCenter = static_cast<uint32_t>(data.vertices.size());
    data.vertices.push_back({{0.0f, height, 0.0f}, up, {0.5f, 0.5f}});
    const uint32_t topRing = static_cast<uint32_t>(data.vertices.size());
    for (int i = 0; i < segments; ++i) {
        const float a = twoPi * static_cast<float>(i) / static_cast<float>(segments);
        data.vertices.push_back({{std::cos(a) * radius, height, std::sin(a) * radius}, up,
                                 {std::cos(a) * 0.5f + 0.5f, std::sin(a) * 0.5f + 0.5f}});
    }
    for (int i = 0; i < segments; ++i) {
        data.indices.push_back(topCenter);
        data.indices.push_back(topRing + static_cast<uint32_t>((i + 1) % segments));
        data.indices.push_back(topRing + static_cast<uint32_t>(i));
    }

    const uint32_t bottomCenter = static_cast<uint32_t>(data.vertices.size());
    data.vertices.push_back({{0.0f, 0.0f, 0.0f}, down, {0.5f, 0.5f}});
    const uint32_t bottomRing = static_cast<uint32_t>(data.vertices.size());
    for (int i = 0; i < segments; ++i) {
        const float a = twoPi * static_cast<float>(i) / static_cast<float>(segments);
        data.vertices.push_back({{std::cos(a) * radius, 0.0f, std::sin(a) * radius}, down,
                                 {std::cos(a) * 0.5f + 0.5f, std::sin(a) * 0.5f + 0.5f}});
    }
    for (int i = 0; i < segments; ++i) {
        data.indices.push_back(bottomCenter);
        data.indices.push_back(bottomRing + static_cast<uint32_t>(i));
        data.indices.push_back(bottomRing + static_cast<uint32_t>((i + 1) % segments));
    }
    return data;
}

MeshData torus(float majorRadius, float tubeRadius, int majorSegments, int minorSegments)
{
    MeshData data;
    majorSegments = std::max(majorSegments, 3);
    minorSegments = std::max(minorSegments, 3);
    const float twoPi = glm::two_pi<float>();

    for (int i = 0; i <= majorSegments; ++i) {
        const float u = static_cast<float>(i) / static_cast<float>(majorSegments);
        const float a = u * twoPi;
        const glm::vec3 center(std::cos(a) * majorRadius, 0.0f, std::sin(a) * majorRadius);
        const glm::vec3 outward(std::cos(a), 0.0f, std::sin(a));

        for (int j = 0; j <= minorSegments; ++j) {
            const float v = static_cast<float>(j) / static_cast<float>(minorSegments);
            const float b = v * twoPi;
            const glm::vec3 normal = outward * std::cos(b) + glm::vec3(0.0f, std::sin(b), 0.0f);
            const glm::vec3 position = center + normal * tubeRadius;
            data.vertices.push_back({position, glm::normalize(normal), {u, v}});
        }
    }

    for (int i = 0; i < majorSegments; ++i) {
        for (int j = 0; j < minorSegments; ++j) {
            const uint32_t a = static_cast<uint32_t>(i * (minorSegments + 1) + j);
            const uint32_t b = static_cast<uint32_t>((i + 1) * (minorSegments + 1) + j);
            const uint32_t c = static_cast<uint32_t>((i + 1) * (minorSegments + 1) + j + 1);
            const uint32_t d = static_cast<uint32_t>(i * (minorSegments + 1) + j + 1);
            data.indices.push_back(a);
            data.indices.push_back(c);
            data.indices.push_back(b);
            data.indices.push_back(a);
            data.indices.push_back(d);
            data.indices.push_back(c);
        }
    }
    return data;
}

MeshData loadObj(const std::string& path, std::string& error)
{
    MeshData data;
    error.clear();

    tinyobj::ObjReaderConfig config;
    config.triangulate = true;
    config.vertex_color = false;

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path, config)) {
        error = reader.Error();
        if (error.empty()) {
            error = "tinyobjloader could not parse " + path;
        }
        return data;
    }
    if (!reader.Warning().empty()) {
        std::fprintf(stderr, "[OBJ] warnings for %s:\n%s\n", path.c_str(),
                     reader.Warning().c_str());
    }

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();

    std::map<std::tuple<int, int, int>, uint32_t> cache;
    bool missingNormals = false;
    bool missingTexcoords = false;
    bool invalidIndex = false;
    bool outOfRange = false;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            if (index.vertex_index < 0) {
                invalidIndex = true;
                break;
            }

            const bool hasNormal = index.normal_index >= 0;
            const bool hasTexcoord = index.texcoord_index >= 0;
            missingNormals = missingNormals || !hasNormal;
            missingTexcoords = missingTexcoords || !hasTexcoord;

            const std::tuple<int, int, int> key(index.vertex_index, index.normal_index,
                                               index.texcoord_index);
            const auto found = cache.find(key);
            if (found != cache.end()) {
                data.indices.push_back(found->second);
                continue;
            }

            const auto vIndex = static_cast<size_t>(index.vertex_index);
            if (vIndex * 3 + 2 >= attrib.vertices.size()) {
                outOfRange = true;
                break;
            }

            Vertex vertex;
            vertex.position = glm::vec3(attrib.vertices[vIndex * 3 + 0],
                                        attrib.vertices[vIndex * 3 + 1],
                                        attrib.vertices[vIndex * 3 + 2]);
            if (hasNormal) {
                const auto nIndex = static_cast<size_t>(index.normal_index);
                if (nIndex * 3 + 2 >= attrib.normals.size()) {
                    outOfRange = true;
                    break;
                }
                vertex.normal = glm::vec3(attrib.normals[nIndex * 3 + 0],
                                          attrib.normals[nIndex * 3 + 1],
                                          attrib.normals[nIndex * 3 + 2]);
            } else {
                vertex.normal = glm::vec3(0.0f);
            }
            if (hasTexcoord) {
                const auto tIndex = static_cast<size_t>(index.texcoord_index);
                if (tIndex * 2 + 1 >= attrib.texcoords.size()) {
                    outOfRange = true;
                    break;
                }
                vertex.uv = glm::vec2(attrib.texcoords[tIndex * 2 + 0],
                                      attrib.texcoords[tIndex * 2 + 1]);
            } else {
                vertex.uv = glm::vec2(0.0f);
            }

            const uint32_t newIndex = static_cast<uint32_t>(data.vertices.size());
            data.vertices.push_back(vertex);
            cache.emplace(key, newIndex);
            data.indices.push_back(newIndex);
        }
        if (invalidIndex || outOfRange) {
            break;
        }
    }

    if (invalidIndex) {
        error = "OBJ contains negative vertex indices: " + path;
        return MeshData{};
    }
    if (outOfRange) {
        error = "OBJ contains indices outside the attribute arrays: " + path;
        return MeshData{};
    }
    if (data.vertices.empty() || data.indices.empty()) {
        error = "OBJ contains no triangle geometry: " + path;
        return MeshData{};
    }
    if (missingTexcoords) {
        std::fprintf(stderr, "[OBJ] %s has missing UVs; defaulting to (0,0)\n", path.c_str());
    }

    if (missingNormals) {
        // Recompute smooth normals when the OBJ omits them.
        for (auto& vertex : data.vertices) {
            vertex.normal = glm::vec3(0.0f);
        }
        for (size_t i = 0; i + 2 < data.indices.size(); i += 3) {
            const auto ia = data.indices[i + 0];
            const auto ib = data.indices[i + 1];
            const auto ic = data.indices[i + 2];
            const glm::vec3 a = data.vertices[ia].position;
            const glm::vec3 b = data.vertices[ib].position;
            const glm::vec3 c = data.vertices[ic].position;
            const glm::vec3 faceNormal = glm::cross(b - a, c - a);
            data.vertices[ia].normal += faceNormal;
            data.vertices[ib].normal += faceNormal;
            data.vertices[ic].normal += faceNormal;
        }
    }
    for (auto& vertex : data.vertices) {
        const float lengthSquared = glm::dot(vertex.normal, vertex.normal);
        vertex.normal = lengthSquared > 1e-12f
                      ? vertex.normal / std::sqrt(lengthSquared)
                      : glm::vec3(0.0f, 1.0f, 0.0f);
    }

    return data;
}

} // namespace geometry

} // namespace gmin
