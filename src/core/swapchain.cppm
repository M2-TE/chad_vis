module;
#include <print>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdint>
export module swapchain;
import vulkan_hpp;
import device;
import window;
import image;

export struct Swapchain {
    void init(Device& device, Window& window);
    void destroy(Device& device);
    void resize(Device& device, Window& window);
    void set_target_framerate(uint32_t fps);
    void present(Device& device, Image& src_image, vk::Semaphore src_ready_to_read, vk::Semaphore src_ready_to_write);

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
    void init(Device& device) {
        // create command pool for this frame
        _command_pool = device._logical.createCommandPool({ .queueFamilyIndex = device._universal_i });
        
        // allocate command buffer from command pool
        _command_buffer = device._logical.allocateCommandBuffers({
            .commandPool = _command_pool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        }).front();

        // create synchronization objects for this frame
        _ready_to_record = device._logical.createFence({ .flags = vk::FenceCreateFlagBits::eSignaled });
        _ready_to_write = device._logical.createSemaphore({});
        _ready_to_read = device._logical.createSemaphore({});
    }
    void destroy(Device& device) {
        device._logical.destroyCommandPool(_command_pool);
        device._logical.destroyFence(_ready_to_record);
        device._logical.destroySemaphore(_ready_to_write);
        device._logical.destroySemaphore(_ready_to_read);
    }

    // command recording
    vk::CommandPool _command_pool;
    vk::CommandBuffer _command_buffer;
    // synchronization
    vk::Fence _ready_to_record;
    vk::Semaphore _ready_to_write;
    vk::Semaphore _ready_to_read;
};

module: private;
void Swapchain::init(Device& device, Window& window) {
    // query swapchain properties
    auto capabilities = device._physical.getSurfaceCapabilitiesKHR(window._surface);
    // manually clamp extent to capabilities
    _extent = window.size();
    _extent.width = std::clamp(_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    _extent.height = std::clamp(_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    // pick color space and format
    auto available_formats = device._physical.getSurfaceFormatsKHR(window._surface);
    // for (auto& format: available_formats) {
    //     std::println("Available format: {}, {}", vk::to_string(format.format), vk::to_string(format.colorSpace));
    // }
    std::vector<vk::SurfaceFormatKHR> preferred_formats = {
        { vk::Format::eA2R10G10B10UnormPack32, vk::ColorSpaceKHR::eSrgbNonlinear },
        { vk::Format::eA2B10G10R10UnormPack32, vk::ColorSpaceKHR::eSrgbNonlinear },
        { vk::Format::eR16G16B16A16Sfloat,     vk::ColorSpaceKHR::eSrgbNonlinear },
        { vk::Format::eR16G16B16A16Unorm,      vk::ColorSpaceKHR::eSrgbNonlinear },
        { vk::Format::eR8G8B8A8Srgb,           vk::ColorSpaceKHR::eSrgbNonlinear },
        { vk::Format::eB8G8R8A8Srgb,           vk::ColorSpaceKHR::eSrgbNonlinear },
        { vk::Format::eR8G8B8A8Unorm,          vk::ColorSpaceKHR::eSrgbNonlinear },
        { vk::Format::eB8G8R8A8Unorm,          vk::ColorSpaceKHR::eSrgbNonlinear },
    };
    _format = available_formats.front().format;
    vk::ColorSpaceKHR color_space = available_formats.front().colorSpace;
    for (auto pref: preferred_formats) {
        if (std::find(available_formats.cbegin(), available_formats.cend(), pref) != available_formats.cend()) {
            _format = pref.format;
            color_space = pref.colorSpace;
            break;
        }
    }
    if (_format == vk::Format::eR8G8B8A8Srgb || _format == vk::Format::eB8G8R8A8Srgb) _manual_srgb_required = false;
    else _manual_srgb_required = true;

    // pick present mode
    auto available_present_modes = device._physical.getSurfacePresentModesKHR(window._surface);
    std::vector<vk::PresentModeKHR> preferred_present_modes = {
        vk::PresentModeKHR::eFifo,
        vk::PresentModeKHR::eMailbox,
        vk::PresentModeKHR::eImmediate,
    };
    vk::PresentModeKHR present_mode = available_present_modes.front();
    for (auto pref: preferred_present_modes) {
        if (std::find(available_present_modes.cbegin(), available_present_modes.cend(), pref) != available_present_modes.cend()) {
            present_mode = pref;
            break;
        }
    }

    // create swapchain
    if (capabilities.maxImageCount == 0) capabilities.maxImageCount = std::numeric_limits<uint32_t>::max();
    vk::SwapchainCreateInfoKHR info_swapchain {
        .surface = window._surface,
        .minImageCount = std::min<uint32_t>(capabilities.minImageCount + 1, capabilities.maxImageCount),
        .imageFormat = _format,
        .imageColorSpace = color_space,
        .imageExtent = _extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eTransferDst,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &device._universal_i,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = present_mode,
        .clipped = true,
        .oldSwapchain = _swapchain,
    };
    _swapchain = device._logical.createSwapchainKHR(info_swapchain);
    if (_images.size() > 0) device._logical.destroySwapchainKHR(_swapchain);
    _images.clear();

    // retrieve and wrap swapchain images
    auto images = device._logical.getSwapchainImagesKHR(_swapchain);
    for (vk::Image image: images) {
        _images.emplace_back().wrap({
            .image = image,
            .image_view = nullptr,
            .extent = { _extent.width, _extent.height, 1 },
            .aspects = vk::ImageAspectFlagBits::eColor
        });
    }

    // create command pools and buffers
    for (std::size_t i = 0; i < images.size(); i++) {
        _sync_frames.emplace_back().init(device);
    }
    _resize_requested = false;
    std::println("Swapchain created: {}, {}x{}, {}",
        vk::to_string(present_mode),
        _extent.width, _extent.height,
        vk::to_string(_format)
    );
}
void Swapchain::destroy(Device& device) {
    for (auto& frame: _sync_frames) frame.destroy(device);
    if (_images.size() > 0) device._logical.destroySwapchainKHR(_swapchain);
}
void Swapchain::resize(Device& device, Window& window) {
    for (auto& frame: _sync_frames) frame.destroy(device);
    init(device, window);
}
void Swapchain::set_target_framerate(uint32_t frames_per_second) {
    double ns = 0;
    if (frames_per_second > 0) {
        double fps = (double)frames_per_second;
        double ms = 1.0 / fps * 1000.0;
        ns = ms * 1000000.0;
    }
    _target_frame_time = std::chrono::nanoseconds(static_cast<int64_t>(ns));
}
void Swapchain::present(Device& device, Image& src_image, vk::Semaphore src_ready_to_read, vk::Semaphore src_ready_to_write) {
    // wait for this frame's fence to be signaled and reset it
    SyncFrame& frame = _sync_frames[_sync_frame_i++ % _sync_frames.size()];
    while (vk::Result::eTimeout == device._logical.waitForFences(frame._ready_to_record, vk::True, UINT64_MAX));
    device._logical.resetFences(frame._ready_to_record);

    // acquire image from swapchain
    uint32_t swap_index = 0;
    for (auto result = vk::Result::eTimeout; result == vk::Result::eTimeout;) {
        std::tie(result, swap_index) = device._logical.acquireNextImageKHR(_swapchain, UINT64_MAX, frame._ready_to_write);
        switch (result) {
            case vk::Result::eSuccess: break;
            case vk::Result::eSuboptimalKHR: {
                // mark swapchain for resize, but still continue to present
                std::println("Swapchain image suboptimal");
                _resize_requested = true;
                break;
            }
            case vk::Result::eErrorOutOfDateKHR: {
                // mark swapchain for resize and abort current immage presentation
                std::println("Swapchain out of date");
                _resize_requested = true;
                return;
            }
            default: {
                std::println("Unexpected result during swapchain image acquisition: {}", vk::to_string(result));
                return;
            }
        }
    }
    
    // restart command buffer
    device._logical.resetCommandPool(frame._command_pool);
    vk::CommandBuffer cmd = frame._command_buffer;
    cmd.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    // draw incoming image onto the current swapchain image via blit
    src_image.transition_layout({
        .cmd = cmd,
        .new_layout = vk::ImageLayout::eTransferSrcOptimal,
        .dst_stage = vk::PipelineStageFlagBits2::eBlit,
        .dst_access = vk::AccessFlagBits2::eTransferRead,
    });
    _images[swap_index].transition_layout({
        .cmd = cmd,
        .new_layout = vk::ImageLayout::eTransferDstOptimal,
        .dst_stage = vk::PipelineStageFlagBits2::eBlit,
        .dst_access = vk::AccessFlagBits2::eTransferWrite,
    });
    _images[swap_index].blit(cmd, src_image);

    // transition swapchain image into presentation layout
    _images[swap_index].transition_layout({
        .cmd = cmd,
        .new_layout = vk::ImageLayout::ePresentSrcKHR,
        .dst_stage = vk::PipelineStageFlagBits2::eBottomOfPipe,
        .dst_access = vk::AccessFlagBits2::eNone,
    });
    cmd.end();
    
    // submit command buffer to graphics queue
    std::array<vk::Semaphore, 2> wait_semaphores = { src_ready_to_read, frame._ready_to_write };
    std::array<vk::Semaphore, 2> sign_semaphores = { src_ready_to_write, frame._ready_to_read };
    std::array<vk::PipelineStageFlags, 2> wait_stages = { 
        vk::PipelineStageFlagBits::eTopOfPipe, 
        vk::PipelineStageFlagBits::eTopOfPipe 
    };
    device._universal_queue.submit(vk::SubmitInfo {
        .waitSemaphoreCount = (uint32_t)wait_semaphores.size(),
        .pWaitSemaphores = wait_semaphores.data(),
        .pWaitDstStageMask = wait_stages.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = (uint32_t)sign_semaphores.size(),
        .pSignalSemaphores = sign_semaphores.data(),
    }, frame._ready_to_record);

    // present swapchain image
    vk::PresentInfoKHR presentInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frame._ready_to_read,
        .swapchainCount = 1,
        .pSwapchains = &_swapchain,
        .pImageIndices = &swap_index,
        .pResults = nullptr
    };

    // wait for target framerate
    auto new_timestamp = std::chrono::high_resolution_clock::now();
    std::chrono::nanoseconds frame_time = new_timestamp - _timestamp;
    if (_target_frame_time > frame_time) {
        std::this_thread::sleep_for(_target_frame_time - frame_time);
    }
    _timestamp = std::chrono::high_resolution_clock::now();
    
    // present swapchain image
    try {
        auto res = device._universal_queue.presentKHR(presentInfo);
        switch (res) {
            case vk::Result::eSuccess: break;
            case vk::Result::eSuboptimalKHR: {
                std::println("Swapchain image suboptimal");
                _resize_requested = true;
                break;
            }
            default: {
                std::println("Unexpected result during swapchain presentation: {}", vk::to_string(res));
                _resize_requested = true;
                break;
            }
        }
    }
    catch (std::runtime_error& e) {
        std::println("Swapchain error: {}", e.what());
        _resize_requested = true;
    }
}
