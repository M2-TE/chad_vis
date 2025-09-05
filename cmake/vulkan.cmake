set(VULKAN_HEADERS_ENABLE_MODULE      OFF)
set(VULKAN_HEADERS_ENABLE_MODULE_STD  OFF)

find_package(Vulkan 1.4.322 QUIET)
if (NOT Vulkan_FOUND)
    include(FetchContent)
    FetchContent_Declare(vulkan-headers
        GIT_REPOSITORY "https://github.com/KhronosGroup/Vulkan-Headers.git"
        GIT_TAG "v1.4.322"
        GIT_SHALLOW ON
        OVERRIDE_FIND_PACKAGE
        EXCLUDE_FROM_ALL
        SYSTEM)
    FetchContent_MakeAvailable(vulkan-headers)
    set(Vulkan_INCLUDE_DIR "${vulkan-headers_SOURCE_DIR}/include")
endif()
if(NOT TARGET Vulkan::HppModule)
    add_library(Vulkan-HppModule)
    add_library(Vulkan::HppModule ALIAS Vulkan-HppModule)
    target_sources(Vulkan-HppModule
        PUBLIC
        FILE_SET cxx_module_vulkan
        TYPE CXX_MODULES
        BASE_DIRS ${Vulkan_INCLUDE_DIR}
        FILES "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::HppModule)
target_compile_definitions(Vulkan-HppModule PUBLIC
    "VK_NO_PROTOTYPES"
    "VULKAN_HPP_ENABLE_STD_MODULE"
    "VULKAN_HPP_TYPESAFE_CONVERSION"
    "VULKAN_HPP_NO_SETTERS"
    "VULKAN_HPP_NO_TO_STRING"
    "VULKAN_HPP_NO_PROTOTYPES"
    "VULKAN_HPP_NO_CONSTRUCTORS"
    "VULKAN_HPP_NO_SMART_HANDLE"
    "VULKAN_HPP_NO_SPACESHIP_OPERATOR")

# copy vk_layer_settings.txt next to executable
get_target_property(VALIDATION_LAYER_OVERRIDE_DIR ${PROJECT_NAME} RUNTIME_OUTPUT_DIRECTORY)
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets/misc/vk_layer_settings.txt" DESTINATION "${VALIDATION_LAYER_OVERRIDE_DIR}")