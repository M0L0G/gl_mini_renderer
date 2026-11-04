#pragma once

#include <filesystem>
#include <string>

namespace gmin {

// Returns the directory that contains the executable, best-effort.
std::filesystem::path executableDirectory();

// Resolves a path such as "shaders/lit.vert" against:
//   1. <exe>/assets/<relative>
//   2. <cwd>/assets/<relative>
//   3. <source>/assets/<relative> (compile-time fallback)
// Returns a candidate path even when none exists; callers can report it.
std::filesystem::path assetPath(const std::string& relative);

bool ensureParentDirectory(const std::filesystem::path& filePath);

} // namespace gmin
