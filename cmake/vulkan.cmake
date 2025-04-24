include(FetchContent)

set(VULKAN_HEADERS_ENABLE_MODULE      ON)
set(VULKAN_HEADERS_ENABLE_MODULE_STD OFF)
FetchContent_Declare(vulkan-headers
    GIT_REPOSITORY "https://github.com/KhronosGroup/Vulkan-Headers.git"
    GIT_TAG "v1.4.313"
    GIT_SHALLOW ON
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(vulkan-headers)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::HppModule)
target_compile_definitions(Vulkan-HppModule PUBLIC
    "VK_NO_PROTOTYPES"
    "VULKAN_HPP_TYPESAFE_CONVERSION"
    "VULKAN_HPP_NO_SETTERS"
    "VULKAN_HPP_NO_TO_STRING"
    "VULKAN_HPP_NO_PROTOTYPES"
    "VULKAN_HPP_NO_CONSTRUCTORS"
    "VULKAN_HPP_NO_SMART_HANDLE"
    "VULKAN_HPP_NO_SPACESHIP_OPERATOR")

# copy vk_layer_settings.txt next to executable
get_target_property(VALIDATION_LAYER_OVERRIDE_DIR ${PROJECT_NAME} RUNTIME_OUTPUT_DIRECTORY)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/misc/vk_layer_settings.txt DESTINATION ${VALIDATION_LAYER_OVERRIDE_DIR})