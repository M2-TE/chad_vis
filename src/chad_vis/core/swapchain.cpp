#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_to_string.hpp>
#include "chad_vis/core/swapchain.hpp"

void Swapchain::init(vk::PhysicalDevice phys_device, vk::Device device, Window& window, dv::Queues& queues) {
    // query swapchain properties
    auto capabilities_res = phys_device.getSurfaceCapabilitiesKHR(window._surface);
    if (capabilities_res.result != vk::Result::eSuccess) {
        std::println("Failed to query surface capabilities: {}", vk::to_string(capabilities_res.result));
        return;
    }
    vk::SurfaceCapabilitiesKHR capabilities = capabilities_res.value;
    _presentation_queue = queues._universal;
    _extent = window.size();
    // manually clamp extent to capabilities
    _extent.width = std::clamp(_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    _extent.height = std::clamp(_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    // pick color space and format
    auto available_formats_res = phys_device.getSurfaceFormatsKHR(window._surface);
    if (available_formats_res.result != vk::Result::eSuccess) {
        std::println("Failed to query surface formats: {}", vk::to_string(available_formats_res.result));
        return;
    }
    std::vector<vk::SurfaceFormatKHR> available_formats = available_formats_res.value;
    for (auto& format: available_formats) {
        std::println("Available format: {}, {}", vk::to_string(format.format), vk::to_string(format.colorSpace));
    }
    std::vector<vk::SurfaceFormatKHR> preferred_formats = {
        { vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear },
        { vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear },
        { vk::Format::eR8G8B8Srgb,   vk::ColorSpaceKHR::eSrgbNonlinear },
        { vk::Format::eB8G8R8Srgb,   vk::ColorSpaceKHR::eSrgbNonlinear },
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

    // pick present mode
    auto available_present_modes_res = phys_device.getSurfacePresentModesKHR(window._surface);
    if (available_present_modes_res.result != vk::Result::eSuccess) {
        std::println("Failed to query surface present modes: {}", vk::to_string(available_present_modes_res.result));
        return;
    }
    std::vector<vk::PresentModeKHR> available_present_modes = available_present_modes_res.value;
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
        .minImageCount = std::clamp<uint32_t>(3, capabilities.minImageCount, capabilities.maxImageCount),
        .imageFormat = _format,
        .imageColorSpace = color_space,
        .imageExtent = _extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eTransferDst,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &queues._universal_i,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = present_mode,
        .clipped = true,
        .oldSwapchain = _swapchain,
    };
    auto swapchain_res = device.createSwapchainKHR(info_swapchain);
    if (swapchain_res.result != vk::Result::eSuccess) {
        std::println("Failed to create swapchain: {}", vk::to_string(swapchain_res.result));
        return;
    }
    if (_images.size() > 0) device.destroySwapchainKHR(_swapchain);
    _images.clear();
    _swapchain = swapchain_res.value;

    // retrieve and wrap swapchain images
    auto images_res = device.getSwapchainImagesKHR(_swapchain);
    if (images_res.result != vk::Result::eSuccess) {
        std::println("Failed to get swapchain images: {}", vk::to_string(images_res.result));
        return;
    }
    std::vector<vk::Image> images = images_res.value;
    for (vk::Image image: images) {
        _images.emplace_back().wrap({
            .image = image,
            .image_view = nullptr,
            .extent = { _extent.width, _extent.height, 1 },
            .aspects = vk::ImageAspectFlagBits::eColor
        });
    }

    // create command pools and buffers
    _sync_frames.resize(images.size());
    for (std::size_t i = 0; i < images.size(); i++) {
        _sync_frames[i].init(device, queues);
    }
    _resize_requested = false;
    std::println("Swapchain created: {}x{}, {}, {}, {}",
        _extent.width, _extent.height,
        vk::to_string(_format),
        vk::to_string(color_space),
        vk::to_string(present_mode));
}
void Swapchain::destroy(vk::Device device) {
    if (_images.size() > 0) device.destroySwapchainKHR(_swapchain);
    for (auto& frame: _sync_frames) frame.destroy(device);
}
void Swapchain::resize(vk::PhysicalDevice physDevice, vk::Device device, Window& window, dv::Queues& queues) {
    for (auto& frame: _sync_frames) frame.destroy(device);
    init(physDevice, device, window, queues);
}
void Swapchain::set_target_framerate(std::size_t fps) {
    double ns = 0;
    if (fps > 0) {
        double fps_d = (double)fps;
        double ms = 1.0 / fps_d * 1000.0;
        ns = ms * 1000000.0;
    }
    _target_frame_time = std::chrono::nanoseconds(static_cast<int64_t>(ns));
}
void Swapchain::present(vk::Device device, dv::Image& src_image, vk::Semaphore src_ready_to_read, vk::Semaphore src_ready_to_write) {
    // wait for this frame's fence to be signaled and reset it
    SyncFrame& frame = _sync_frames[_sync_frame_i++ % _sync_frames.size()];
    while (vk::Result::eTimeout == device.waitForFences(frame._ready_to_record, vk::True, UINT64_MAX));
    device.resetFences(frame._ready_to_record);

    // acquire image from swapchain
    uint32_t swap_index = 0;
    for (auto result = vk::Result::eTimeout; result == vk::Result::eTimeout;) {
        std::tie(result, swap_index) = device.acquireNextImageKHR(_swapchain, UINT64_MAX, frame._ready_to_write);
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
    device.resetCommandPool(frame._command_pool);
    vk::CommandBuffer cmd = frame._command_buffer;
    vk::Result res = cmd.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    if (res != vk::Result::eSuccess) std::println("Failed to begin command buffer: {}", vk::to_string(res));

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
    res = cmd.end();
    if (res != vk::Result::eSuccess) std::println("Failed to end command buffer: {}", vk::to_string(res));
    
    // submit command buffer to graphics queue
    std::array<vk::Semaphore, 2> wait_semaphores = { src_ready_to_read, frame._ready_to_write };
    std::array<vk::Semaphore, 2> sign_semaphores = { src_ready_to_write, frame._ready_to_read };
    std::array<vk::PipelineStageFlags, 2> wait_stages = { 
        vk::PipelineStageFlagBits::eTopOfPipe, 
        vk::PipelineStageFlagBits::eTopOfPipe 
    };
    vk::SubmitInfo info_submit {
        .waitSemaphoreCount = (uint32_t)wait_semaphores.size(),
        .pWaitSemaphores = wait_semaphores.data(),
        .pWaitDstStageMask = wait_stages.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = (uint32_t)sign_semaphores.size(),
        .pSignalSemaphores = sign_semaphores.data(),
    };
    res = _presentation_queue.submit(info_submit, frame._ready_to_record);
    if (res != vk::Result::eSuccess) std::println("Failed to submit command buffer: {}", vk::to_string(res));

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
    res = _presentation_queue.presentKHR(presentInfo);
    switch (res) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfDateKHR: {
            std::println("Swapchain out of date");
            _resize_requested = true;
            break;
        }
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

void Swapchain::SyncFrame::init(vk::Device device, dv::Queues& queues) {
    // create command pool for this frame
    auto command_pool_res = device.createCommandPool({ .queueFamilyIndex = queues._graphics_i });
    if (command_pool_res.result != vk::Result::eSuccess) {
        std::println("Failed to create command pool: {}", vk::to_string(command_pool_res.result));
        return;
    }
    _command_pool = command_pool_res.value;
    
    // allocate command buffer from command pool
    auto command_buffer_res = device.allocateCommandBuffers({
        .commandPool = _command_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    });
    if (command_buffer_res.result != vk::Result::eSuccess) {
        std::println("Failed to allocate command buffer: {}", vk::to_string(command_buffer_res.result));
        return;
    }
    _command_buffer = command_buffer_res.value.front();

    // create synchronization objects for this frame
    auto fence_res = device.createFence({ .flags = vk::FenceCreateFlagBits::eSignaled });
    if (fence_res.result != vk::Result::eSuccess) std::println("Failed to create fence: {}", vk::to_string(fence_res.result));
    _ready_to_record = fence_res.value;
    auto semaphore_res = device.createSemaphore({});
    if (semaphore_res.result != vk::Result::eSuccess) std::println("Failed to create semaphore: {}", vk::to_string(semaphore_res.result));
    _ready_to_write = semaphore_res.value;
    semaphore_res = device.createSemaphore({});
    if (semaphore_res.result != vk::Result::eSuccess) std::println("Failed to create semaphore: {}", vk::to_string(semaphore_res.result));
    _ready_to_read = semaphore_res.value;
}
void Swapchain::SyncFrame::destroy(vk::Device device) {
    device.destroyCommandPool(_command_pool);
    device.destroyFence(_ready_to_record);
    device.destroySemaphore(_ready_to_write);
    device.destroySemaphore(_ready_to_read);
}
