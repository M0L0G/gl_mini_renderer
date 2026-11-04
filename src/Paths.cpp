#include "Paths.h"

#include <cstdlib>
#include <string>
#include <vector>

#ifndef GMIN_ASSET_DIR
#define GMIN_ASSET_DIR "assets"
#endif

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace gmin {
namespace {

std::filesystem::path currentPath()
{
    std::error_code error;
    const auto path = std::filesystem::current_path(error);
    return error ? std::filesystem::path() : path;
}

} // namespace

std::filesystem::path executableDirectory()
{
#if defined(_WIN32)
    wchar_t buffer[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        return std::filesystem::path(buffer).parent_path();
    }
#elif defined(__linux__)
    char buffer[PATH_MAX];
    const ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (length > 0) {
        buffer[length] = '\0';
        return std::filesystem::path(buffer).parent_path();
    }
#elif defined(__APPLE__)
    uint32_t size = 0;
    if (_NSGetExecutablePath(nullptr, &size) == -1) {
        std::vector<char> buffer(size + 1);
        if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
            return std::filesystem::path(buffer.data()).parent_path();
        }
    }
#endif
    return currentPath();
}

std::filesystem::path assetPath(const std::string& relative)
{
    std::vector<std::filesystem::path> candidates;
    candidates.push_back(executableDirectory() / "assets" / relative);
    candidates.push_back(currentPath() / "assets" / relative);
    candidates.push_back(std::filesystem::path(GMIN_ASSET_DIR) / relative);

    std::error_code error;
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate, error) && !error) {
            return candidate;
        }
        error.clear();
    }
    return candidates.front();
}

bool ensureParentDirectory(const std::filesystem::path& filePath)
{
    const auto parent = filePath.parent_path();
    if (parent.empty()) {
        return true;
    }
    std::error_code error;
    std::filesystem::create_directories(parent, error);
    return !error;
}

} // namespace gmin
