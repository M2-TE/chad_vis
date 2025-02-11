cmake_minimum_required(VERSION 3.30)
project(chad_vis LANGUAGES CXX)

# cmake settings
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) # option() should honor normal variables

# language settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# create visualizer executable
add_executable(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/"
    CXX_SCAN_FOR_MODULES ON)

# add source files
file(GLOB_RECURSE CPP_SOURCE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
target_sources(${PROJECT_NAME} PRIVATE "${CPP_SOURCE_FILES}")
file(GLOB_RECURSE CPPM_SOURCE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm")
target_sources(${PROJECT_NAME} PUBLIC FILE_SET CXX_MODULES FILES "${CPPM_SOURCE_FILES}")

# add dependencies
include(FetchContent)
set(FETCHCONTENT_QUIET OFF) # enable git output for FetchContent steps
set(FETCHCONTENT_UPDATES_DISCONNECTED ON) # speed up consecutive config runs
include("cmake/vulkan.cmake")
include("cmake/vma.cmake")
include("cmake/shaders.cmake")
include("cmake/glfw.cmake")
include("cmake/glm.cmake")
