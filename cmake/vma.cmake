FetchContent_Declare(vulkanmemoryallocator
    GIT_REPOSITORY "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git"
    GIT_TAG "v3.1.0"
    GIT_SHALLOW ON
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_Declare(vulkanmemoryallocator-hpp
    GIT_REPOSITORY "https://github.com/YaaZ/VulkanMemoryAllocator-Hpp.git"
    GIT_TAG "v3.1.0"
    GIT_SHALLOW ON
    GIT_SUBMODULES ""
    SOURCE_SUBDIR "disabled" # their cmake should be fixed by v3.2.0
    OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(vulkanmemoryallocator vulkanmemoryallocator-hpp)

target_link_libraries(${PROJECT_NAME} PRIVATE GPUOpen::VulkanMemoryAllocator)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE "${vulkanmemoryallocator-hpp_SOURCE_DIR}/include")
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "VMA_DYNAMIC_VULKAN_FUNCTIONS=1"
	"VMA_STATIC_VULKAN_FUNCTIONS=0")