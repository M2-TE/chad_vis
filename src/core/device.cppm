export module core.device;
import vulkan_hpp;
import vulkan.allocator;

export enum class QueueType { eUniversal, eGraphics, eCompute, eTransfer };
export struct Device {
    struct CreateInfo;
    void init(const CreateInfo& info);
    void destroy();
    // TODO: return oneshot structure with automatic cleanup (waiting on fence(s))
    // start new command buffer on specified queue
    auto oneshot_begin(QueueType queue) -> vk::CommandBuffer;
    // end the command buffer and synchronize via semaphores
    void oneshot_end(QueueType queue, vk::CommandBuffer cmd,
            const vk::ArrayProxy<vk::Semaphore>& wait_semaphores = {},
            const vk::ArrayProxy<vk::Semaphore>& sign_semaphores = {});

    vk::Device _logical;
    vk::PhysicalDevice _physical;
    vma::Allocator _vmalloc;
    
    // TODO: std::map with QueueType
    uint32_t _universal_i, _graphics_i, _compute_i, _transfer_i;
    vk::Queue _universal_queue, _graphics_queue, _compute_queue, _transfer_queue;
    vk::CommandPool _universal_pool, _graphics_pool, _compute_pool, _transfer_pool;
    vk::Fence _oneshot_fence;
};

struct Device::CreateInfo {
    vk::Instance _instance;
    vk::SurfaceKHR _surface = nullptr;
    uint32_t _required_major = 1;
    uint32_t _required_minor = 0;
    vk::PhysicalDeviceType _preferred_device_type = vk::PhysicalDeviceType::eDiscreteGpu;
    vk::PhysicalDeviceFeatures _required_features = {};
    vk::PhysicalDeviceVulkan11Features _required_vk11_features = {};
    vk::PhysicalDeviceVulkan12Features _required_vk12_features = {};
    vk::PhysicalDeviceVulkan13Features _required_vk13_features = {};
    std::vector<const char*> _required_extensions = {};
    std::vector<const char*> _optional_extensions = {};
    std::vector<std::pair<void*, const char*>> _optional_features = {};
};