module;
#include <vector>
#include <chrono>
#include <cstdint>
export module renderer.swapchain;
import core.device;
import core.window;
import buffers.image;
import renderer.semaphore;
import vulkan_hpp;

export struct Swapchain {
    void init(Device& device, Window& window);
    void destroy(Device& device);
    void resize(Device& device, Window& window);
    void set_target_framerate(uint32_t fps);
    void present(Device& device, Image& src_image, RendererSemaphore& render_semaphore);

    vk::SwapchainKHR _swapchain;
    std::vector<Image> _images;
    vk::Extent2D _extent;
    vk::Format _format;
    bool _resize_requested;
    bool _manual_srgb_required;

private:
    struct SyncFrame;
    uint32_t _sync_frame_i = 0;
    std::vector<SyncFrame> _sync_frames;
    std::chrono::duration<int64_t, std::nano> _target_frame_time;
    std::chrono::time_point<std::chrono::high_resolution_clock> _timestamp;
};

struct Swapchain::SyncFrame {
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
