#pragma once

#include "Camera.h"
#include "Options.h"
#include "Renderer.h"
#include "Scene.h"

struct GLFWwindow;

namespace gmin {

class App {
public:
    explicit App(const Options& options);
    ~App();

    void run();

private:
    Options options_;
    GLFWwindow* window_ = nullptr;
    Camera camera_;
    Scene scene_;
    Renderer renderer_;
    RenderSettings settings_;

    int framebufferWidth_ = 1280;
    int framebufferHeight_ = 720;
    int frameCount_ = 0;
    int fpsFrameCount_ = 0;
    double lastTime_ = 0.0;
    double fpsTimer_ = 0.0;
    double elapsedTime_ = 0.0;
    float displayedFps_ = 0.0f;

    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;
    bool firstMouse_ = true;
    bool glfwInitialized_ = false;
    bool cleanedUp_ = false;

    void init();
    void cleanup();
    void resetCamera();
    void updateCameraInput(float dt);
    void updateTitle();
    void onKey(int key, int action);
    void onMouseMove(double x, double y);
    void onScroll(double yOffset);
    void onFramebufferResize(int width, int height);
    bool isScreenshotMode() const;
    void saveScreenshotIfNeeded();

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void cursorCallback(GLFWwindow* window, double x, double y);
    static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);
    static void framebufferCallback(GLFWwindow* window, int width, int height);
    static void glfwErrorCallback(int code, const char* description);
};

} // namespace gmin
