cmake_minimum_required(VERSION 4.0)
project(chad_vis LANGUAGES CXX)

# settings
option(PREFER_MOLD "Prefer to use mold linker (Linux only)" ON)
option(PREFER_CCACHE "Prefer to use ccache (Linux only)" ON)
option(USE_FAST_MATH "Enable aggressive math optimizations, disables cross-platform determinism" ON)
option(USE_STRICT_COMPILATION "Force all warnings to emit errors" OFF)
include("cmake/options_global.cmake")
include("cmake/options_compiler.cmake")

# create executable
add_executable(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_SCAN_FOR_MODULES ON
    RUNTIME_OUTPUT_DIRECTORY         "${CMAKE_CURRENT_BINARY_DIR}/bin/"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_CURRENT_BINARY_DIR}/bin/"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/bin/")

# add source files
file(GLOB_RECURSE CPP_SOURCE_FILES  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE CPPM_SOURCE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm")
target_sources(${PROJECT_NAME} PRIVATE ${CPP_SOURCE_FILES})
target_sources(${PROJECT_NAME} PRIVATE
    FILE_SET cxx_module_files
    TYPE CXX_MODULES
    FILES "${CPPM_SOURCE_FILES}")

# add dependencies
include("cmake/vulkan.cmake")
include("cmake/vma.cmake")
include("cmake/shaders.cmake")
include("cmake/glm.cmake")
include("cmake/sdl.cmake")
