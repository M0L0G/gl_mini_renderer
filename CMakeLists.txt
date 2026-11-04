cmake_minimum_required(VERSION 3.16)
project(gl_mini_renderer VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep the executable at <build>/gl_mini_renderer[.exe] for multi-config
# generators as well, so the documented run command is stable.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
foreach(_cfg Debug Release RelWithDebInfo MinSizeRel)
  string(TOUPPER ${_cfg} _cfg_upper)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_cfg_upper} ${CMAKE_BINARY_DIR})
endforeach()

if(MSVC)
  add_compile_options(/utf-8)
endif()

find_package(OpenGL REQUIRED)

# --- vendored GLFW -----------------------------------------------------------
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND  OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/glfw EXCLUDE_FROM_ALL)

# --- generated GLAD loader ---------------------------------------------------
add_library(glad STATIC third_party/glad/src/gl.c)
target_include_directories(glad PUBLIC third_party/glad/include)
target_link_libraries(glad PUBLIC OpenGL::GL ${CMAKE_DL_LIBS})
if(MSVC)
  target_compile_options(glad PRIVATE /W0)
endif()

# --- header-only dependencies -----------------------------------------------
add_library(gmin_headers INTERFACE)
target_include_directories(gmin_headers INTERFACE
  third_party/glm
  third_party/stb
  third_party/tinyobjloader
)

add_executable(gl_mini_renderer
  src/main.cpp
  src/App.cpp
  src/Camera.cpp
  src/Mesh.cpp
  src/Options.cpp
  src/Paths.cpp
  src/PostProcess.cpp
  src/Renderer.cpp
  src/Scene.cpp
  src/Shader.cpp
  src/ShadowMap.cpp
  src/Skybox.cpp
  src/StbImpl.cpp
  src/Texture.cpp
  src/TinyObjImpl.cpp
)

if(MSVC)
  # Vendored single-header libraries contain their own warning suppressions
  # and MSVC-specific code; keep our project warnings enabled for our code only.
  set_source_files_properties(src/StbImpl.cpp src/TinyObjImpl.cpp
    PROPERTIES COMPILE_OPTIONS "/W0")
endif()

target_link_libraries(gl_mini_renderer PRIVATE glfw glad gmin_headers)
target_compile_definitions(gl_mini_renderer PRIVATE
  GMIN_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

if(MSVC)
  target_compile_options(gl_mini_renderer PRIVATE /W3)
else()
  target_compile_options(gl_mini_renderer PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Assets are copied next to the executable, making the build directory
# self-contained. Missing optional assets are handled with code-generated
# fallbacks at runtime.
add_custom_command(TARGET gl_mini_renderer POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/assets"
          "$<TARGET_FILE_DIR:gl_mini_renderer>/assets"
  COMMENT "Copying runtime assets"
)
