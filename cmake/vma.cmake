find_package(VulkanMemoryAllocator 3.3.0 QUIET)
if (NOT VulkanMemoryAllocator_FOUND)
    include(FetchContent)
    FetchContent_Declare(vulkanmemoryallocator
        GIT_REPOSITORY "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git"
        GIT_TAG "v3.3.0"
        GIT_SHALLOW ON
        OVERRIDE_FIND_PACKAGE
        EXCLUDE_FROM_ALL
        SYSTEM)
    FetchContent_MakeAvailable(vulkanmemoryallocator)
endif()

find_package(VulkanMemoryAllocator-Hpp 3.2.1 QUIET)
if (NOT VulkanMemoryAllocator-Hpp_FOUND)
    include(FetchContent)
    FetchContent_Declare(vulkanmemoryallocator-hpp
        GIT_REPOSITORY "https://github.com/YaaZ/VulkanMemoryAllocator-Hpp.git"
        GIT_TAG "v3.2.1"
        GIT_SHALLOW ON
        GIT_SUBMODULES ""
        OVERRIDE_FIND_PACKAGE
        EXCLUDE_FROM_ALL
        SYSTEM)
    FetchContent_MakeAvailable(vulkanmemoryallocator-hpp)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE 
    GPUOpen::VulkanMemoryAllocator
    VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp)