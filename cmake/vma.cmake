# set(VMA_BUILD_EXAMPLE OFF)
# set(VMA_BUILD_CXX_MODULE ON)
# set(VMA_BUILD_WITH_STD_MODULE OFF)
# set(VMA_BUILD_WITH_VULKAN_MODULE OFF)
# set(VMA_BUILD_MODULE_VULKAN_DYNAMIC ON)
FetchContent_Declare(vulkanmemoryallocator
    GIT_REPOSITORY "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git"
    GIT_TAG "v3.2.0"
    GIT_SHALLOW ON
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_Declare(vulkanmemoryallocator-hpp
    GIT_REPOSITORY "https://github.com/M2-TE/VulkanMemoryAllocator-Hpp.git"
    GIT_TAG "fix-no-to-string"
    GIT_SHALLOW ON
    GIT_SUBMODULES ""
    SOURCE_SUBDIR "" # disabled for now
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(vulkanmemoryallocator vulkanmemoryallocator-hpp)

# temporary baindaid fix until vma module cmake is improved
target_sources(${PROJECT_NAME} PUBLIC FILE_SET CXX_MODULES FILES "${vulkanmemoryallocator-hpp_SOURCE_DIR}/src/vk_mem_alloc.cppm")
target_link_libraries(${PROJECT_NAME} PRIVATE GPUOpen::VulkanMemoryAllocator)
target_include_directories(${PROJECT_NAME} PRIVATE "${vulkanmemoryallocator-hpp_SOURCE_DIR}/include")
target_compile_definitions(${PROJECT_NAME} PRIVATE "VMA_ENABLE_VULKAN_HPP_MODULE")