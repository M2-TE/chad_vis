#pragma once
#include <vulkan/vulkan.hpp>
#include "chad_vis/core/device.hpp"
#include "chad_vis/core/window.hpp"
#include "chad_vis/device/image.hpp"

struct Swapchain {
    void init(Device& device, Window& window);
    void destroy(Device& device);
    void resize(Device& device, Window& window);
    void set_target_framerate(std::size_t fps);
    void present(Device& device, Image& src_image, vk::Semaphore src_ready_to_read, vk::Semaphore src_ready_to_write);

    vk::SwapchainKHR _swapchain;
    std::vector<Image> _images;
    vk::Extent2D _extent;
    vk::Format _format;
    bool _resize_requested;
    bool _srgb_required;

private:
    struct SyncFrame {
        void init(Device& device);
        void destroy(Device& device);
        // command recording
        vk::CommandPool _command_pool;
        vk::CommandBuffer _command_buffer;
        // synchronization
        vk::Fence _ready_to_record;
        vk::Semaphore _ready_to_write;
        vk::Semaphore _ready_to_read;
    };
    std::vector<SyncFrame> _sync_frames;
    uint32_t _sync_frame_i = 0;
    std::chrono::duration<int64_t, std::nano> _target_frame_time;
    std::chrono::time_point<std::chrono::high_resolution_clock> _timestamp;
};