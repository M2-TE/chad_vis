#include "chad_vis/device/queues.hpp"

namespace dv {
    void Queues::init(vk::Device device, std::span<uint32_t> queue_mappings) {
        _universal_i = queue_mappings[0];
        _graphics_i = queue_mappings[1];
        _compute_i = queue_mappings[2];
        _transfer_i = queue_mappings[3];
        _universal = device.getQueue(_universal_i, 0);
        _graphics = device.getQueue(_graphics_i, 0);
        _compute = device.getQueue(_compute_i, 0);
        _transfer = device.getQueue(_transfer_i, 0);

        vk::CommandPoolCreateInfo info = {
            .flags = vk::CommandPoolCreateFlagBits::eTransient,
            .queueFamilyIndex = _universal_i,
        };
        _universal_pool = device.createCommandPool(info);

    }
    void Queues::destroy(vk::Device device) {
        device.destroyCommandPool(_universal_pool);
    }

    auto Queues::oneshot_begin(vk::Device device) -> vk::CommandBuffer {
        vk::CommandBufferAllocateInfo info = {
            .commandPool = _universal_pool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        };
        vk::CommandBuffer cmd = device.allocateCommandBuffers(info)[0];
        cmd.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
        return cmd;
    }
    void Queues::oneshot_end(vk::Device device, vk::CommandBuffer cmd, const vk::ArrayProxy<vk::Semaphore>& sign_semaphores) {
        cmd.end();
        vk::SubmitInfo info = {
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd,
            .signalSemaphoreCount = (uint32_t)sign_semaphores.size(),
            .pSignalSemaphores = sign_semaphores.data(),
        };
        _universal.submit(info);
        _universal.waitIdle();
        device.freeCommandBuffers(_universal_pool, cmd);
    }
} // namespace dv