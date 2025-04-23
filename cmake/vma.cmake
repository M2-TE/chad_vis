FetchContent_Declare(vulkanmemoryallocator
    GIT_REPOSITORY "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git"
    GIT_TAG "v3.2.1"
    GIT_SHALLOW ON
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_Declare(vulkanmemoryallocator-hpp
    GIT_REPOSITORY "https://github.com/M2-TE/VulkanMemoryAllocator-Hpp.git"
    GIT_TAG "fix-no-to-string"
    GIT_SHALLOW ON
    GIT_SUBMODULES ""
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(vulkanmemoryallocator vulkanmemoryallocator-hpp)
target_link_libraries(${PROJECT_NAME} PRIVATE 
    GPUOpen::VulkanMemoryAllocator
    VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp)