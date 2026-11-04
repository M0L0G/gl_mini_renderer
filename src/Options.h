#pragma once

#include <string>

namespace gmin {

struct Options {
    bool showHelp = false;
    int width = 1280;
    int height = 720;
    int frames = -1;
    std::string screenshotPath;
    int postMode = -1;
    bool noShadow = false;
    bool wireframe = false;
    bool vsyncOff = false;
};

Options parseOptions(int argc, char** argv);
void printHelp();

} // namespace gmin
