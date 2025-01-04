#pragma once
#include <vulkan/vulkan.hpp>

namespace dv {
struct Queues {
    void init(vk::Device device, std::span<uint32_t> queue_mappings);
    void destroy(vk::Device device);

    auto oneshot_begin(vk::Device device) -> vk::CommandBuffer;
    void oneshot_end(vk::Device device, vk::CommandBuffer cmd, const vk::ArrayProxy<vk::Semaphore>& sign_semaphores = {});
    // queue handle
    vk::Queue _universal, _graphics, _compute, _transfer;
    // queue family index
    uint32_t _universal_i, _graphics_i, _compute_i, _transfer_i;
private:
    // oneshot command pool
    vk::CommandPool _universal_pool;
};
} // namespace dv