#pragma once
#include <vulkan/vulkan.hpp>
#include "chad_vis/core/window.hpp"
#include "chad_vis/device/image.hpp"
#include "chad_vis/device/queues.hpp"

class Swapchain {
    struct SyncFrame {
        void init(vk::Device device, dv::Queues& queues) {
            _command_pool = device.createCommandPool({ .queueFamilyIndex = queues._graphics_i });
            vk::CommandBufferAllocateInfo bufferInfo {
                .commandPool = _command_pool,
                .level = vk::CommandBufferLevel::ePrimary,
                .commandBufferCount = 1,
            };
            _command_buffer = device.allocateCommandBuffers(bufferInfo).front();

            vk::SemaphoreCreateInfo semaInfo {};
            _ready_to_record = device.createFence({ .flags = vk::FenceCreateFlagBits::eSignaled });
            _ready_to_write = device.createSemaphore(semaInfo);
            _ready_to_read = device.createSemaphore(semaInfo);
        }
        void destroy(vk::Device device) {
            device.destroyCommandPool(_command_pool);
            device.destroyFence(_ready_to_record);
            device.destroySemaphore(_ready_to_write);
            device.destroySemaphore(_ready_to_read);
        }
        // command recording
        vk::CommandPool _command_pool;
        vk::CommandBuffer _command_buffer;
        // synchronization
        vk::Fence _ready_to_record;
        vk::Semaphore _ready_to_write;
        vk::Semaphore _ready_to_read;
    };
public:
    void init(vk::PhysicalDevice phys_device, vk::Device device, Window& window, dv::Queues& queues) {
        // query swapchain properties
        vk::SurfaceCapabilitiesKHR capabilities = phys_device.getSurfaceCapabilitiesKHR(window._surface);
        _presentation_queue = queues._universal;
        _extent = window.size();
        // manually clamp extent to capabilities
        _extent.width = std::clamp(_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        _extent.height = std::clamp(_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        // pick color space and format
        std::vector<vk::SurfaceFormatKHR> available_formats = phys_device.getSurfaceFormatsKHR(window._surface);
        std::vector<vk::SurfaceFormatKHR> preferred_formats = {
            { vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear },
            { vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear },
            { vk::Format::eR8G8B8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear },
            { vk::Format::eB8G8R8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear },
        };
        vk::ColorSpaceKHR color_space = vk::ColorSpaceKHR::eSrgbNonlinear;
        for (auto pref: preferred_formats) {
            if (std::find(available_formats.cbegin(), available_formats.cend(), pref) != available_formats.cend()) {
                _format = pref.format;
                color_space = pref.colorSpace;
                break;
            }
        }

        // pick present mode
        std::vector<vk::PresentModeKHR> available_present_modes = phys_device.getSurfacePresentModesKHR(window._surface);
        std::vector<vk::PresentModeKHR> preferred_present_modes = {
            vk::PresentModeKHR::eFifo,
            vk::PresentModeKHR::eMailbox,
            vk::PresentModeKHR::eImmediate,
        };
        vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo;
        for (auto mode: preferred_present_modes) {
            if (std::find(available_present_modes.cbegin(), available_present_modes.cend(), mode) != available_present_modes.cend()) {
                present_mode = mode;
                break;
            }
        }

        // create swapchain
        vk::SwapchainCreateInfoKHR info_swapchain {
            .surface = window._surface,
            .minImageCount = std::clamp<uint32_t>(
                3,
                capabilities.minImageCount, 
                capabilities.maxImageCount == 0
                    ? std::numeric_limits<uint32_t>::max()
                    : capabilities.maxImageCount),
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
        vk::SwapchainKHR old_swapchain = _swapchain;
        _swapchain = device.createSwapchainKHR(info_swapchain);
        if (old_swapchain != vk::SwapchainKHR(nullptr) && _images.size() > 0) device.destroySwapchainKHR(old_swapchain);
        _images.clear(); // old images were owned by previous swapchain

        // retrieve and wrap swapchain images
        std::vector<vk::Image> images = device.getSwapchainImagesKHR(_swapchain);
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
    }
    void destroy(vk::Device device) {
        if (_images.size() > 0) device.destroySwapchainKHR(_swapchain);
        for (auto& frame: _sync_frames) frame.destroy(device);
    }
    void resize(vk::PhysicalDevice physDevice, vk::Device device, Window& window, dv::Queues& queues) {
        for (auto& frame: _sync_frames) frame.destroy(device);
        init(physDevice, device, window, queues);
        std::println("Swapchain resized to: {}x{}", _extent.width, _extent.height);
    }
    void set_target_framerate(std::size_t fps) {
        double ns = 0;
        if (fps > 0) {
            double fps_d = (double)fps;
            double ms = 1.0 / fps_d * 1000.0;
            ns = ms * 1000000.0;
        }
        _target_frame_time = std::chrono::nanoseconds(static_cast<int64_t>(ns));
    }
    void present(vk::Device device, dv::Image& src_image, vk::Semaphore src_ready_to_read, vk::Semaphore src_ready_to_write) {
        // wait for this frame's fence to be signaled and reset it
        SyncFrame& frame = _sync_frames[_sync_frame_i++ % _sync_frames.size()];
        while (vk::Result::eTimeout == device.waitForFences(frame._ready_to_record, vk::True, UINT64_MAX));
        device.resetFences(frame._ready_to_record);

        // acquire image from swapchain
        uint32_t swap_index = 0;
        for (auto result = vk::Result::eTimeout; result == vk::Result::eTimeout;) {
            try {
                std::tie(result, swap_index) = device.acquireNextImageKHR(_swapchain, UINT64_MAX, frame._ready_to_write);
            }
            catch (const vk::OutOfDateKHRError& e) {
                std::println("Swapchain out of date ({})", e.what());
                _resize_requested = true;
                return;
            }
            if (result == vk::Result::eSuboptimalKHR) {
                // mark swapchain for resize, but still continue to present
                std::println("Swapchain image suboptimal");
                _resize_requested = true;
            }
        }
        
        // restart command buffer
        device.resetCommandPool(frame._command_pool);
        vk::CommandBuffer cmd = frame._command_buffer;
        cmd.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
        // draw incoming image onto the current swapchain image
        draw_swapchain(cmd, src_image, _images[swap_index]);
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
        vk::SubmitInfo info_submit {
            .waitSemaphoreCount = (uint32_t)wait_semaphores.size(),
            .pWaitSemaphores = wait_semaphores.data(),
            .pWaitDstStageMask = wait_stages.data(),
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd,
            .signalSemaphoreCount = (uint32_t)sign_semaphores.size(),
            .pSignalSemaphores = sign_semaphores.data(),
        };
        _presentation_queue.submit(info_submit, frame._ready_to_record);

        // present swapchain image
        vk::PresentInfoKHR presentInfo {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &frame._ready_to_read,
            .swapchainCount = 1,
            .pSwapchains = &_swapchain,
            .pImageIndices = &swap_index,
            .pResults = nullptr
        };
        wait_target_framerate();
        try {
            vk::Result result = _presentation_queue.presentKHR(presentInfo);
            if (result == vk::Result::eSuboptimalKHR) {
                std::println("Swapchain suboptimal");
                _resize_requested = true;
            }
        }
        catch (const vk::OutOfDateKHRError& e) {
            std::println("Swapchain out of date ({})", e.what());
            _resize_requested = true;
            return;
        }
    }

private:
    void draw_swapchain(vk::CommandBuffer cmd, dv::Image& src_image, dv::Image& dst_image) {
        // perform blit from source to swapchain image
        src_image.transition_layout({
            .cmd = cmd,
            .new_layout = vk::ImageLayout::eTransferSrcOptimal,
            .dst_stage = vk::PipelineStageFlagBits2::eBlit,
            .dst_access = vk::AccessFlagBits2::eTransferRead,
        });
        dst_image.transition_layout({
            .cmd = cmd,
            .new_layout = vk::ImageLayout::eTransferDstOptimal,
            .dst_stage = vk::PipelineStageFlagBits2::eBlit,
            .dst_access = vk::AccessFlagBits2::eTransferWrite,
        });
        dst_image.blit(cmd, src_image);
    }
    void wait_target_framerate() {
        // keep track of how much time passed since last frame was presented
        auto new_timestamp = std::chrono::high_resolution_clock::now();
        std::chrono::nanoseconds diff = new_timestamp - _timestamp;

        // figure out if and how long needs to be waited
        if (_target_frame_time > diff) {
            std::this_thread::sleep_for(_target_frame_time - diff);
        }
        _timestamp = std::chrono::high_resolution_clock::now();
    }
public:
    vk::SwapchainKHR _swapchain;
    std::vector<dv::Image> _images;
    vk::Extent2D _extent;
    vk::Format _format;
    vk::Queue _presentation_queue;
    bool _resize_requested;
private:
    std::vector<SyncFrame> _sync_frames;
    uint32_t _sync_frame_i = 0;
    std::chrono::duration<int64_t, std::nano> _target_frame_time;
    std::chrono::time_point<std::chrono::high_resolution_clock> _timestamp;
};