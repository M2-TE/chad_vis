include(FetchContent)

# SPIR-V shader reflection
set(SPIRV_REFLECT_EXECUTABLE     OFF)
set(SPIRV_REFLECT_STATIC_LIB     ON)
set(SPIRV_REFLECT_BUILD_TESTS    OFF)
set(SPIRV_REFLECT_EXAMPLES       OFF)
FetchContent_Declare(spirv-reflect
    GIT_REPOSITORY "https://github.com/KhronosGroup/SPIRV-Reflect.git"
    GIT_TAG "vulkan-sdk-1.4.313.0"
    GIT_SHALLOW ON
    GIT_SUBMODULES ""
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(spirv-reflect)
target_link_libraries(${PROJECT_NAME} PRIVATE spirv-reflect-static)

# SMAA for anti-aliasing
FetchContent_Declare(smaa
    GIT_REPOSITORY "https://github.com/iryoku/smaa.git"
    GIT_TAG "71c806a838bdd7d517df19192a20f0c61b3ca29d"
    GIT_SHALLOW ON
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(smaa)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE "${smaa_SOURCE_DIR}/Textures")

# SPVRC for shader compilation and embedding
set(SPVRC_SHADER_ENV "vulkan1.3")
set(SPVRC_SHADER_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/")
set(SPVRC_SHADER_INCLUDE_DIRS "${smaa_SOURCE_DIR}")
FetchContent_Declare(spvrc
    GIT_REPOSITORY "https://github.com/M2-TE/spvrc.git"
    GIT_TAG "v1.0.5"
    GIT_SHALLOW ON
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(spvrc)
target_link_libraries(${PROJECT_NAME} PRIVATE spvrc::spvrc)