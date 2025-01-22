FetchContent_Declare(vulkanmemoryallocator
    GIT_REPOSITORY "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git"
    GIT_TAG "v3.2.0"
    GIT_SHALLOW ON
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_Declare(vulkanmemoryallocator-hpp
    GIT_REPOSITORY "https://github.com/YaaZ/VulkanMemoryAllocator-Hpp.git"
    GIT_TAG "4fcfec043b12629f934f4d542aedeedaa12a101e"
    # GIT_TAG "v3.1.0"
    # GIT_SHALLOW ON
    GIT_SUBMODULES ""
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(vulkanmemoryallocator vulkanmemoryallocator-hpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
    GPUOpen::VulkanMemoryAllocator
    VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "VMA_DYNAMIC_VULKAN_FUNCTIONS=1"
	"VMA_STATIC_VULKAN_FUNCTIONS=0")