#include "Texture.h"

#include <glad/gl.h>
#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>

namespace gmin {
namespace {

struct Rng {
    uint32_t state;
    explicit Rng(uint32_t seed) : state(seed == 0 ? 0x9e3779b9u : seed) {}
    uint32_t next()
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
    float nextFloat()
    {
        return static_cast<float>(next() & 0x00ffffffu) / 16777216.0f;
    }
};

float hash2(int x, int y, uint32_t seed)
{
    uint32_t h = seed + static_cast<uint32_t>(x) * 374761393u
               + static_cast<uint32_t>(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return static_cast<float>(h & 0x00ffffffu) / 16777216.0f;
}

float smoothStep(float t)
{
    return t * t * (3.0f - 2.0f * t);
}

float valueNoise(float x, float y, uint32_t seed)
{
    const int ix = static_cast<int>(std::floor(x));
    const int iy = static_cast<int>(std::floor(y));
    const float fx = x - static_cast<float>(ix);
    const float fy = y - static_cast<float>(iy);
    const float ux = smoothStep(fx);
    const float uy = smoothStep(fy);

    const float a = hash2(ix, iy, seed);
    const float b = hash2(ix + 1, iy, seed);
    const float c = hash2(ix, iy + 1, seed);
    const float d = hash2(ix + 1, iy + 1, seed);
    return glm::mix(glm::mix(a, b, ux), glm::mix(c, d, ux), uy);
}

float fbm(float x, float y, uint32_t seed, int octaves = 4)
{
    float value = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;
    for (int i = 0; i < octaves; ++i) {
        value += valueNoise(x * frequency, y * frequency, seed + static_cast<uint32_t>(i) * 1013u)
               * amplitude;
        frequency *= 2.0f;
        amplitude *= 0.5f;
    }
    return value;
}

unsigned char toByte(float value)
{
    return static_cast<unsigned char>(std::clamp(std::lround(value * 255.0f), 0L, 255L));
}

} // namespace

Texture2D::~Texture2D()
{
    destroy();
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : texture_(other.texture_)
    , width_(other.width_)
    , height_(other.height_)
{
    other.texture_ = 0;
    other.width_ = 0;
    other.height_ = 0;
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept
{
    if (this != &other) {
        destroy();
        texture_ = other.texture_;
        width_ = other.width_;
        height_ = other.height_;
        other.texture_ = 0;
        other.width_ = 0;
        other.height_ = 0;
    }
    return *this;
}

void Texture2D::destroy()
{
    if (texture_ != 0) {
        glDeleteTextures(1, &texture_);
        texture_ = 0;
    }
    width_ = 0;
    height_ = 0;
}

bool Texture2D::loadFromFile(const std::string& path, bool srgb, bool flipY)
{
    stbi_set_flip_vertically_on_load(flipY ? 1 : 0);
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
    stbi_set_flip_vertically_on_load(0);

    if (pixels == nullptr || width <= 0 || height <= 0) {
        std::fprintf(stderr, "[Texture] failed to load %s: %s\n", path.c_str(),
                     stbi_failure_reason());
        return false;
    }

    upload(pixels, width, height, srgb);
    stbi_image_free(pixels);
    return true;
}

void Texture2D::upload(const unsigned char* pixels, int width, int height, bool srgb)
{
    destroy();
    width_ = width;
    height_ = height;

    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    GLfloat anisotropy = 0.0f;
    glGetFloatv(0x84FF, &anisotropy);
    if (glGetError() == GL_NO_ERROR && anisotropy > 1.0f) {
        glTexParameterf(GL_TEXTURE_2D, 0x84FE, std::min(anisotropy, 8.0f));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

void Texture2D::createCheckerboard(int size, int cells,
                                   const glm::vec3& colorA, const glm::vec3& colorB)
{
    size = std::max(size, 8);
    cells = std::max(cells, 2);
    std::vector<unsigned char> pixels(static_cast<size_t>(size) * size * 4);

    const float cellSize = static_cast<float>(size) / static_cast<float>(cells);
    Rng rng(0x5eed1234u);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const int cx = std::min(static_cast<int>(x / cellSize), cells - 1);
            const int cy = std::min(static_cast<int>(y / cellSize), cells - 1);
            const bool light = ((cx + cy) & 1) == 0;
            glm::vec3 color = light ? colorA : colorB;

            const float localX = x - std::floor(x / cellSize) * cellSize;
            const float localY = y - std::floor(y / cellSize) * cellSize;
            const bool grout = localX < 2.0f || localY < 2.0f
                            || localX >= cellSize - 2.0f || localY >= cellSize - 2.0f;
            if (grout) {
                color *= 0.72f;
            }

            const float noise = (rng.nextFloat() - 0.5f) * 0.055f;
            color += glm::vec3(noise);
            color = glm::clamp(color, 0.0f, 1.0f);

            const size_t index = (static_cast<size_t>(y) * size + x) * 4;
            pixels[index + 0] = toByte(color.r);
            pixels[index + 1] = toByte(color.g);
            pixels[index + 2] = toByte(color.b);
            pixels[index + 3] = 255;
        }
    }
    upload(pixels.data(), size, size, true);
}

void Texture2D::createWood(int size)
{
    size = std::max(size, 16);
    std::vector<unsigned char> pixels(static_cast<size_t>(size) * size * 4);
    const glm::vec3 dark(0.24f, 0.13f, 0.055f);
    const glm::vec3 warm(0.55f, 0.33f, 0.15f);
    const glm::vec3 light(0.72f, 0.48f, 0.24f);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float u = static_cast<float>(x) / size;
            const float v = static_cast<float>(y) / size;
            const float warp = fbm(u * 5.0f, v * 2.0f, 0x71u) * 0.12f;
            const float grain = 0.5f + 0.5f * std::sin((u * 26.0f + warp + v * 2.5f) * 6.28318f);
            const float strands = fbm(u * 30.0f + warp * 10.0f, v * 3.0f, 0xabcd123u);
            glm::vec3 color = glm::mix(dark, warm, glm::clamp(grain * 0.65f + strands * 0.5f, 0.0f, 1.0f));
            color = glm::mix(color, light, glm::clamp(strands - 0.55f, 0.0f, 0.35f));
            color *= 0.9f + 0.1f * fbm(u * 4.0f, v * 4.0f, 0x55u);

            const float edge = std::min(std::min(u, 1.0f - u), std::min(v, 1.0f - v));
            if (edge < 0.012f) {
                color *= 0.55f;
            }

            const size_t index = (static_cast<size_t>(y) * size + x) * 4;
            pixels[index + 0] = toByte(color.r);
            pixels[index + 1] = toByte(color.g);
            pixels[index + 2] = toByte(color.b);
            pixels[index + 3] = 255;
        }
    }
    upload(pixels.data(), size, size, true);
}

void Texture2D::createStone(int size)
{
    size = std::max(size, 16);
    std::vector<unsigned char> pixels(static_cast<size_t>(size) * size * 4);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float u = static_cast<float>(x) / size;
            const float v = static_cast<float>(y) / size;
            const float base = fbm(u * 8.0f, v * 8.0f, 0x2222u);
            const float detail = fbm(u * 38.0f, v * 38.0f, 0x9876u);
            const float cracks = std::abs(std::sin((u + v) * 22.0f + base * 8.0f));
            float shade = 0.30f + base * 0.28f + detail * 0.12f;
            if (cracks < 0.045f) {
                shade *= 0.55f;
            }
            glm::vec3 color(shade * 0.93f, shade * 0.96f, shade * 1.02f);
            color = glm::clamp(color, 0.0f, 1.0f);

            const size_t index = (static_cast<size_t>(y) * size + x) * 4;
            pixels[index + 0] = toByte(color.r);
            pixels[index + 1] = toByte(color.g);
            pixels[index + 2] = toByte(color.b);
            pixels[index + 3] = 255;
        }
    }
    upload(pixels.data(), size, size, true);
}

void Texture2D::createJade(int size)
{
    size = std::max(size, 16);
    std::vector<unsigned char> pixels(static_cast<size_t>(size) * size * 4);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float u = static_cast<float>(x) / size;
            const float v = static_cast<float>(y) / size;
            const float warp = fbm(u * 3.0f, v * 3.0f, 0x314159u);
            const float marble = 0.5f + 0.5f * std::sin((u + v + warp * 1.7f) * 12.0f);
            const float veins = std::pow(1.0f - marble, 6.0f);
            glm::vec3 base(0.055f, 0.30f, 0.24f);
            glm::vec3 deep(0.015f, 0.11f, 0.10f);
            glm::vec3 gold(0.72f, 0.62f, 0.30f);
            glm::vec3 color = glm::mix(deep, base, 0.55f + 0.45f * fbm(u * 9.0f, v * 9.0f, 0x77u));
            color = glm::mix(color, gold, veins * 0.45f);
            color = glm::clamp(color, 0.0f, 1.0f);

            const size_t index = (static_cast<size_t>(y) * size + x) * 4;
            pixels[index + 0] = toByte(color.r);
            pixels[index + 1] = toByte(color.g);
            pixels[index + 2] = toByte(color.b);
            pixels[index + 3] = 255;
        }
    }
    upload(pixels.data(), size, size, true);
}

void Texture2D::bind(GLuint unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture_);
}

} // namespace gmin
