#include "App.h"

#include "GlCommon.h"
#include "Paths.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stb_image_write.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace gmin {
namespace {

const char* postModeName(int mode)
{
    switch (mode) {
    case 1: return "Grayscale";
    case 2: return "Invert";
    default: return "Normal";
    }
}

} // namespace

App::App(const Options& options)
    : options_(options)
{
    settings_.shadowsEnabled = !options_.noShadow;
    settings_.wireframe = options_.wireframe;
    settings_.postMode = options_.postMode >= 0 ? options_.postMode : 0;
}

App::~App()
{
    cleanup();
}

void App::glfwErrorCallback(int code, const char* description)
{
    std::fprintf(stderr, "[GLFW] error %d: %s\n", code, description);
}

void App::init()
{
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    glfwInitialized_ = true;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    window_ = glfwCreateWindow(options_.width, options_.height,
                               "gl_mini_renderer", nullptr, nullptr);
    if (window_ == nullptr) {
        throw std::runtime_error("Failed to create a 1280x720 OpenGL 3.3 core window");
    }

    glfwMakeContextCurrent(window_);
    const int loaded = gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress));
    if (loaded == 0) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    std::printf("OpenGL %s | %s | %s\n",
                glGetString(GL_VERSION),
                glGetString(GL_RENDERER),
                glGetString(GL_VENDOR));

    glfwSwapInterval((options_.vsyncOff || isScreenshotMode()) ? 0 : 1);
    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, keyCallback);
    glfwSetCursorPosCallback(window_, cursorCallback);
    glfwSetScrollCallback(window_, scrollCallback);
    glfwSetFramebufferSizeCallback(window_, framebufferCallback);
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwGetFramebufferSize(window_, &framebufferWidth_, &framebufferHeight_);
    camera_.setAspect(static_cast<float>(framebufferWidth_)
                      / static_cast<float>(std::max(framebufferHeight_, 1)));
    resetCamera();

    scene_.init();
    renderer_.init(scene_);
    renderer_.resize(framebufferWidth_, framebufferHeight_);

    std::printf("Assets: %s\n", assetPath("").string().c_str());
    std::printf("Controls: WASD + mouse | scroll zoom | F1 post | F2 wireframe | "
                "F3 shadows | R reset | ESC quit\n");

    lastTime_ = glfwGetTime();
    updateTitle();
    checkGLError("App::init");
}

void App::cleanup()
{
    if (cleanedUp_) {
        return;
    }
    cleanedUp_ = true;

    if (window_ != nullptr) {
        glfwMakeContextCurrent(window_);
        renderer_.shutdown();
        scene_.shutdown();
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    if (glfwInitialized_) {
        glfwTerminate();
        glfwInitialized_ = false;
    }
}

void App::resetCamera()
{
    camera_.reset(glm::vec3(9.0f, 5.0f, 12.0f), glm::vec3(0.0f, 2.8f, 0.0f));
    camera_.setFov(45.0f);
    firstMouse_ = true;
}

void App::run()
{
    init();

    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();

        const double now = glfwGetTime();
        float dt = static_cast<float>(now - lastTime_);
        lastTime_ = now;
        dt = std::clamp(dt, 0.0f, 0.1f);
        elapsedTime_ += dt;

        updateCameraInput(dt);
        if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }

        glfwGetFramebufferSize(window_, &framebufferWidth_, &framebufferHeight_);
        if (framebufferWidth_ > 0 && framebufferHeight_ > 0) {
            camera_.setAspect(static_cast<float>(framebufferWidth_)
                              / static_cast<float>(framebufferHeight_));
            scene_.update(static_cast<float>(elapsedTime_));
            renderer_.render(scene_, camera_, framebufferWidth_, framebufferHeight_, settings_);

            ++frameCount_;
            saveScreenshotIfNeeded();

            glfwSwapBuffers(window_);
        } else {
            glfwWaitEventsTimeout(0.05);
        }

        ++fpsFrameCount_;
        fpsTimer_ += static_cast<double>(dt);
        if (fpsTimer_ >= 0.5) {
            displayedFps_ = static_cast<float>(fpsFrameCount_ / fpsTimer_);
            fpsFrameCount_ = 0;
            fpsTimer_ = 0.0;
            updateTitle();
        }

        if (options_.frames >= 0 && frameCount_ >= options_.frames) {
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }
    }

    cleanup();
}

void App::updateCameraInput(float dt)
{
    if (window_ == nullptr) {
        return;
    }
    float forward = 0.0f;
    float right = 0.0f;
    float up = 0.0f;
    if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS) {
        forward += 1.0f;
    }
    if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS) {
        forward -= 1.0f;
    }
    if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS) {
        right += 1.0f;
    }
    if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS) {
        right -= 1.0f;
    }
    if (glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS) {
        up += 1.0f;
    }
    if (glfwGetKey(window_, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
        || glfwGetKey(window_, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
        up -= 1.0f;
    }
    const bool fast = glfwGetKey(window_, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
                   || glfwGetKey(window_, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    camera_.move(forward, right, up, dt, fast);
}

void App::onKey(int key, int action)
{
    if (action != GLFW_PRESS) {
        return;
    }
    switch (key) {
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
        break;
    case GLFW_KEY_F1:
        settings_.postMode = (settings_.postMode + 1) % 3;
        std::printf("[Post] %s\n", postModeName(settings_.postMode));
        updateTitle();
        break;
    case GLFW_KEY_F2:
        settings_.wireframe = !settings_.wireframe;
        std::printf("[Wireframe] %s\n", settings_.wireframe ? "ON" : "OFF");
        updateTitle();
        break;
    case GLFW_KEY_F3:
        settings_.shadowsEnabled = !settings_.shadowsEnabled;
        std::printf("[Shadows] %s\n", settings_.shadowsEnabled ? "ON" : "OFF");
        updateTitle();
        break;
    case GLFW_KEY_R:
        resetCamera();
        std::printf("[Camera] reset\n");
        break;
    default:
        break;
    }
}

void App::onMouseMove(double x, double y)
{
    if (firstMouse_) {
        lastMouseX_ = x;
        lastMouseY_ = y;
        firstMouse_ = false;
        return;
    }
    const float xOffset = static_cast<float>(x - lastMouseX_);
    const float yOffset = static_cast<float>(lastMouseY_ - y);
    lastMouseX_ = x;
    lastMouseY_ = y;
    camera_.look(xOffset, yOffset);
}

void App::onScroll(double yOffset)
{
    camera_.zoom(static_cast<float>(yOffset));
}

void App::onFramebufferResize(int width, int height)
{
    framebufferWidth_ = width;
    framebufferHeight_ = height;
    if (width > 0 && height > 0) {
        camera_.setAspect(static_cast<float>(width) / static_cast<float>(height));
        renderer_.resize(width, height);
    }
}

void App::updateTitle()
{
    if (window_ == nullptr) {
        return;
    }
    char title[256];
    std::snprintf(title, sizeof(title),
                  "gl_mini_renderer | %.1f FPS | shadows %s | wireframe %s | post %s | FOV %.0f",
                  displayedFps_, settings_.shadowsEnabled ? "ON" : "OFF",
                  settings_.wireframe ? "ON" : "OFF", postModeName(settings_.postMode),
                  camera_.fov());
    glfwSetWindowTitle(window_, title);
}

bool App::isScreenshotMode() const
{
    return !options_.screenshotPath.empty();
}

void App::saveScreenshotIfNeeded()
{
    if (!isScreenshotMode()) {
        return;
    }

    // A positive --frames value doubles as the capture frame. With no value we
    // wait a few frames so that the scene animation and first-frame driver
    // warm-up have settled; --frames 0 still captures the very first frame.
    const int targetFrames = options_.frames > 0
                                 ? options_.frames
                                 : (options_.frames == 0 ? 1 : 4);
    if (frameCount_ < targetFrames) {
        return;
    }

    const int width = framebufferWidth_;
    const int height = framebufferHeight_;
    if (width <= 0 || height <= 0) {
        throw std::runtime_error("Cannot save screenshot from an empty framebuffer");
    }

    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glFinish();

    // OpenGL returns the image bottom-up; PNG rows are top-down.
    std::vector<unsigned char> flipped(pixels.size());
    const size_t rowBytes = static_cast<size_t>(width) * 4;
    for (int y = 0; y < height; ++y) {
        std::memcpy(flipped.data() + static_cast<size_t>(y) * rowBytes,
                    pixels.data() + static_cast<size_t>(height - 1 - y) * rowBytes,
                    rowBytes);
    }

    const std::filesystem::path path = options_.screenshotPath;
    if (!ensureParentDirectory(path)) {
        throw std::runtime_error("Cannot create screenshot directory for: " + path.string());
    }
    if (stbi_write_png(path.string().c_str(), width, height, 4, flipped.data(),
                       static_cast<int>(rowBytes)) == 0) {
        throw std::runtime_error("Failed to write screenshot: " + path.string());
    }

    std::printf("[Screenshot] %dx%d -> %s\n", width, height, path.string().c_str());
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

void App::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (app != nullptr) {
        app->onKey(key, action);
    }
}

void App::cursorCallback(GLFWwindow* window, double x, double y)
{
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (app != nullptr) {
        app->onMouseMove(x, y);
    }
}

void App::scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    (void)xOffset;
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (app != nullptr) {
        app->onScroll(yOffset);
    }
}

void App::framebufferCallback(GLFWwindow* window, int width, int height)
{
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (app != nullptr) {
        app->onFramebufferResize(width, height);
    }
}

} // namespace gmin
