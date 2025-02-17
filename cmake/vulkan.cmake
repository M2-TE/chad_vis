# set(VULKAN_HPP_ENABLE_CPP20_MODULES ON)
# set(VULKAN_HPP_ENABLE_STD_MODULE OFF)
# set(VULKAN_HPP_TYPESAFE_CONVERSION OFF)
# set(VULKAN_HPP_DISPATCH_LOADER_DYNAMIC ON)
# set(VULKAN_HPP_NO_SETTERS ON)
# set(VULKAN_HPP_NO_TO_STRING OFF)
# set(VULKAN_HPP_NO_PROTOTYPES ON)
# set(VULKAN_HPP_NO_CONSTRUCTORS ON)
# set(VULKAN_HPP_NO_SMART_HANDLE ON)
# set(VULKAN_HPP_NO_SPACESHIP_OPERATOR ON)
# FetchContent_Declare(vulkan-hpp
#     GIT_REPOSITORY "https://github.com/KhronosGroup/Vulkan-Hpp.git"
#     GIT_TAG "7d7c25f9bebf89606892360efdb0d1c873dd1de8"
#     # GIT_SHALLOW ON
#     GIT_SUBMODULES "Vulkan-Headers"
#     OVERRIDE_FIND_PACKAGE
#     EXCLUDE_FROM_ALL
#     SYSTEM)
# FetchContent_MakeAvailable(vulkan-hpp)
# target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::HppModule)

set(VK_NO_PROTOTYPES ON)
set(VULKAN_HPP_ENABLE_MODULE ON)
set(VULKAN_HPP_ENABLE_MODULE_STD OFF)
set(VULKAN_HPP_TYPESAFE_CONVERSION ON)
set(VULKAN_HPP_DISPATCH_LOADER_DYNAMIC ON)
set(VULKAN_HPP_NO_SETTERS ON)
set(VULKAN_HPP_NO_TO_STRING ON)
set(VULKAN_HPP_NO_PROTOTYPES ON)
set(VULKAN_HPP_NO_CONSTRUCTORS ON)
set(VULKAN_HPP_NO_SMART_HANDLE ON)
set(VULKAN_HPP_NO_SPACESHIP_OPERATOR ON)
FetchContent_Declare(vulkan-headers
    GIT_REPOSITORY "https://github.com/M2-TE/Vulkan-Headers.git"
    GIT_TAG "cmake-vulkan-hpp"
    GIT_SHALLOW ON
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(vulkan-headers)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::HppModule)

# create vk_layer_settings.txt for validation layers
get_target_property(VALIDATION_LAYER_OVERRIDE_FOLDER ${PROJECT_NAME} RUNTIME_OUTPUT_DIRECTORY)
file(MAKE_DIRECTORY "${VALIDATION_LAYER_OVERRIDE_FOLDER}")
file(WRITE "${VALIDATION_LAYER_OVERRIDE_FOLDER}/vk_layer_settings.txt"
"# Enable core validation
khronos_validation.validate_core = true

# Enable thread safety validation
khronos_validation.thread_safety = true

# Enable best practices validation for all vendors
khronos_validation.validate_best_practices = true
khronos_validation.validate_best_practices_arm = true
khronos_validation.validate_best_practices_amd = true
khronos_validation.validate_best_practices_img = true
khronos_validation.validate_best_practices_nvidia = true

# Enable synchronization validation
khronos_validation.validate_sync = true
khronos_validation.syncval_shader_accesses_heuristic = true

# Enable GPU-assisted validation
khronos_validation.gpuav_select_instrumented_shaders = true
khronos_validation.gpuav_debug_validate_instrumented_shaders = true

# Filter out certain messages
# 601872502   -> info about validation layers being enabled
# 1762589289  -> suboptimal swapchain image (already handled)
# -2001217138 -> outdated swapchain (its properly handled already)
# -920984000  -> bound vertex buffer wasnt used (imgui again)
# -488404154  -> lod clamping (imgui sets -1000 to 1000 clamping)
# -855582553  -> use of primitive restart is not recommended (dont care)
# -1443561624 -> warning about high number of fences
# -539066078  -> warning about high number of semaphores
khronos_validation.message_id_filter = 601872502,1762589289,-2001217138,-920984000,-488404154,-855582553,-1443561624,-539066078

# Set report flags to include info, warnings, errors, and performance issues
khronos_validation.report_flags = info,warn,error,perf")