set(VMA_HPP_ENABLE_MODULE ON)
set(VMA_HPP_ENABLE_MODULE_STD OFF)
set(VMA_HPP_ENABLE_VULKAN_HPP_MODULE ON)
set(VMA_HPP_ENABLE_VULKAN_HPP_DYNAMIC ON)
FetchContent_Declare(vulkanmemoryallocator-hpp
    GIT_REPOSITORY "https://github.com/M2-TE/VulkanMemoryAllocator-Hpp.git"
    GIT_TAG "update-cmake"
    GIT_SHALLOW ON
    GIT_SUBMODULES "VulkanMemoryAllocator"
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(vulkanmemoryallocator-hpp)

target_link_libraries(VulkanMemoryAllocator-HppModule PUBLIC Vulkan::HppModule)
target_link_libraries(${PROJECT_NAME} PRIVATE VulkanMemoryAllocator::HppModule)