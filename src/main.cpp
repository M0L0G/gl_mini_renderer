#include "App.h"
#include "Options.h"

#include <cstdio>
#include <exception>

int main(int argc, char** argv)
{
    const gmin::Options options = gmin::parseOptions(argc, argv);
    if (options.showHelp) {
        gmin::printHelp();
        return 0;
    }

    try {
        gmin::App app(options);
        app.run();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FATAL: %s\n", error.what());
        return 1;
    }
    return 0;
}
