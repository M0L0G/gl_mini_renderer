#include "Options.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace gmin {
namespace {

bool parseInt(const char* text, int& out)
{
    if (text == nullptr || *text == '\0') {
        return false;
    }
    char* end = nullptr;
    const long value = std::strtol(text, &end, 10);
    if (end == text || *end != '\0') {
        return false;
    }
    out = static_cast<int>(value);
    return true;
}

bool parsePostMode(const char* text, int& mode)
{
    if (std::strcmp(text, "normal") == 0) {
        mode = 0;
    } else if (std::strcmp(text, "grayscale") == 0 || std::strcmp(text, "gray") == 0) {
        mode = 1;
    } else if (std::strcmp(text, "invert") == 0) {
        mode = 2;
    } else {
        return false;
    }
    return true;
}

} // namespace

Options parseOptions(int argc, char** argv)
{
    Options options;
    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0) {
            options.showHelp = true;
        } else if (std::strcmp(arg, "--frames") == 0 && i + 1 < argc) {
            if (!parseInt(argv[++i], options.frames) || options.frames < 0) {
                std::fprintf(stderr, "Invalid value for --frames\n");
                options.showHelp = true;
            }
        } else if (std::strcmp(arg, "--width") == 0 && i + 1 < argc) {
            if (!parseInt(argv[++i], options.width) || options.width < 320) {
                std::fprintf(stderr, "Invalid value for --width (minimum 320)\n");
                options.showHelp = true;
            }
        } else if (std::strcmp(arg, "--height") == 0 && i + 1 < argc) {
            if (!parseInt(argv[++i], options.height) || options.height < 240) {
                std::fprintf(stderr, "Invalid value for --height (minimum 240)\n");
                options.showHelp = true;
            }
        } else if (std::strcmp(arg, "--screenshot") == 0 && i + 1 < argc) {
            options.screenshotPath = argv[++i];
        } else if (std::strcmp(arg, "--post") == 0 && i + 1 < argc) {
            if (!parsePostMode(argv[++i], options.postMode)) {
                std::fprintf(stderr, "Invalid --post mode (normal|grayscale|invert)\n");
                options.showHelp = true;
            }
        } else if (std::strcmp(arg, "--no-shadow") == 0) {
            options.noShadow = true;
        } else if (std::strcmp(arg, "--wireframe") == 0) {
            options.wireframe = true;
        } else if (std::strcmp(arg, "--vsync-off") == 0) {
            options.vsyncOff = true;
        } else {
            std::fprintf(stderr, "Unknown or incomplete argument: %s\n", arg);
            options.showHelp = true;
        }
    }
    return options;
}

void printHelp()
{
    std::printf(
        "gl_mini_renderer - OpenGL 3.3 Core mini real-time renderer\n"
        "\n"
        "Usage: gl_mini_renderer [options]\n"
        "\n"
        "Options:\n"
        "  --help                Show this help and exit\n"
        "  --frames <n>          Exit after n rendered frames (smoke testing)\n"
        "  --width <n>           Window width (default 1280)\n"
        "  --height <n>          Window height (default 720)\n"
        "  --screenshot <path>   Render a frame, save a PNG, and exit\n"
        "  --post <mode>         normal | grayscale | invert (screenshot/testing)\n"
        "  --no-shadow           Start with shadows disabled\n"
        "  --wireframe           Start in wireframe mode\n"
        "  --vsync-off           Disable vertical sync\n"
        "\n"
        "Controls:\n"
        "  W / A / S / D         Move camera forward / left / back / right\n"
        "  Mouse                 Look around\n"
        "  Scroll                Zoom (changes FOV)\n"
        "  Shift                 Move faster\n"
        "  F1                    Cycle post-processing: normal / grayscale / invert\n"
        "  F2                    Toggle wireframe\n"
        "  F3                    Toggle directional shadows\n"
        "  R                     Reset camera\n"
        "  ESC                   Quit\n");
}

} // namespace gmin
