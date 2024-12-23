#pragma once
#include <cstdint>
#include <cmath>
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.hpp>
#include <fmt/base.h>
#include <glm/glm.hpp>
#include "chad_vis/core/swapchain.hpp"
#include "chad_vis/device/image.hpp"
#include "chad_vis/device/queues.hpp"
#include "chad_vis/device/pipeline.hpp"
#include "chad_vis/ext/smaa.hpp"

class Renderer {
public:
    void init(vk::Device device, vma::Allocator vmalloc, dv::Queues& queues, vk::Extent2D extent) {
        // allocate single command pool and buffer pair
        _command_pool = device.createCommandPool({ .queueFamilyIndex = queues._universal_i });
        vk::CommandBufferAllocateInfo bufferInfo {
            .commandPool = _command_pool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        };
        _command_buffer = device.allocateCommandBuffers(bufferInfo).front();

        // allocate semaphores and fences
        _ready_to_record = device.createFence({ .flags = vk::FenceCreateFlagBits::eSignaled });
        _ready_to_write = device.createSemaphore({});
        _ready_to_read = device.createSemaphore({});
        // create dummy submission to initialize _ready_to_write
        auto cmd = queues.oneshot_begin(device);
        queues.oneshot_end(device, cmd, _ready_to_write);
        
        // create images and pipelines
        init_images(device, vmalloc, extent);
        init_pipelines(device, extent);
        _smaa.init(device, vmalloc, extent, queues, _color, _depth_stencil);
    }
    void destroy(vk::Device device, vma::Allocator vmalloc) {
        _smaa.destroy(device, vmalloc);
        // destroy images
        _color.destroy(device, vmalloc);
        _storage.destroy(device, vmalloc);
        _depth_stencil.destroy(device, vmalloc);
        // destroy pipelines
        _pipe_wip.destroy(device);
        // destroy command pools
        device.destroyCommandPool(_command_pool);
        // destroy synchronization objects
        device.destroyFence(_ready_to_record);
        device.destroySemaphore(_ready_to_write);
        device.destroySemaphore(_ready_to_read);
    }
    
    void resize(vk::Device device, vma::Allocator vmalloc, dv::Queues& queues, vk::Extent2D extent) {
        destroy(device, vmalloc);
        init(device, vmalloc, queues, extent);
    }
    // record command buffer and submit it to the universal queue. wait() needs to have been called before this
    void render(vk::Device device, Swapchain& swapchain, dv::Queues& queues) {
        // reset and record command buffer
        device.resetCommandPool(_command_pool, {});
        vk::CommandBuffer cmd = _command_buffer;
        cmd.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
        execute_pipes(cmd);

        // optionally run SMAA
        if (_smaa_enabled) _smaa.execute(cmd, _color, _depth_stencil);
        cmd.end();

        // submit command buffer
        vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eTopOfPipe;
        vk::SubmitInfo info_submit {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &_ready_to_write,
            .pWaitDstStageMask = &wait_stage,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &_ready_to_read,
        };
        queues._universal.submit(info_submit, _ready_to_record);

        // present drawn image
        dv::Image& _final_image = _smaa_enabled ? _smaa.get_output() : _color;
        swapchain.present(device, _final_image, _ready_to_read, _ready_to_write);
    }
    // wait until device buffers are no longer in use and the command buffers can be recorded again
    void wait(vk::Device device) {
        while (vk::Result::eTimeout == device.waitForFences(_ready_to_record, vk::True, UINT64_MAX));
        device.resetFences(_ready_to_record);
    }
    
private:
    void init_images(vk::Device device, vma::Allocator vmalloc, vk::Extent2D extent) {
        // create image with 16 bits color depth
        _color.init({
            .device = device, .vmalloc = vmalloc,
            .format = vk::Format::eR16G16B16A16Sfloat,
            .extent { extent.width, extent.height, 1 },
            .usage = 
                vk::ImageUsageFlagBits::eColorAttachment |
                vk::ImageUsageFlagBits::eTransferSrc |
                vk::ImageUsageFlagBits::eTransferDst |
                vk::ImageUsageFlagBits::eSampled,
        });
        // create storage image with same format as _color
        _storage.init({
            .device = device, .vmalloc = vmalloc,
            .format = vk::Format::eR16G16B16A16Sfloat,
            .extent { extent.width, extent.height, 1 },
            .usage = 
                vk::ImageUsageFlagBits::eTransferSrc |
                vk::ImageUsageFlagBits::eStorage
        });
        // create depth stencil with depth/stencil format picked by driver
        _depth_stencil.init(device, vmalloc, { extent.width, extent.height, 1 });
    }
    void init_pipelines(vk::Device device, vk::Extent2D extent) {
        // create main compute pipeline
        glm::uvec2 image_size = { extent.width, extent.height };
        std::array<vk::SpecializationMapEntry, 2> image_spec_entries {
            vk::SpecializationMapEntry { .constantID = 0, .offset = 0, .size = 4 },
            vk::SpecializationMapEntry { .constantID = 1, .offset = 4, .size = 4 },
        };
        vk::SpecializationInfo image_spec_info = {
            .mapEntryCount = image_spec_entries.size(),
            .pMapEntries = image_spec_entries.data(),
            .dataSize = sizeof(image_size),
            .pData = &image_size
        };
        _pipe_wip.init({
            .device = device,
            .cs_path = "defaults/gradient.comp",
            .spec_info = &image_spec_info
        });
        _pipe_wip.write_descriptor(device, 0, 0, _storage, vk::DescriptorType::eStorageImage);
    }
    void execute_pipes(vk::CommandBuffer cmd) {
        // prepare for storage via compute shader
        _storage.transition_layout({
            .cmd = cmd,
            .new_layout = vk::ImageLayout::eGeneral,
            .dst_stage = vk::PipelineStageFlagBits2::eComputeShader,
            .dst_access = vk::AccessFlagBits2::eShaderWrite
        });

        // execute main compute pipeline
        uint32_t nx = std::ceil(_storage._extent.width / 8.0);
        uint32_t ny = std::ceil(_storage._extent.height / 8.0);
        _pipe_wip.execute(cmd, nx, ny, 1);

        // blit storage image to color image
        _storage.transition_layout({
            .cmd = cmd,
            .new_layout = vk::ImageLayout::eTransferSrcOptimal,
            .dst_stage = vk::PipelineStageFlagBits2::eBlit,
            .dst_access = vk::AccessFlagBits2::eTransferRead
        });
        _color.transition_layout({
            .cmd = cmd,
            .new_layout = vk::ImageLayout::eTransferDstOptimal,
            .dst_stage = vk::PipelineStageFlagBits2::eBlit,
            .dst_access = vk::AccessFlagBits2::eTransferWrite
        });
        _color.blit(cmd, _storage);
    }

private:
    // synchronization
    vk::Fence _ready_to_record;
    vk::Semaphore _ready_to_write;
    vk::Semaphore _ready_to_read;
    // command recording
    vk::CommandPool _command_pool;
    vk::CommandBuffer _command_buffer;
    // images
    dv::DepthStencil _depth_stencil;
    dv::Image _color;
    dv::Image _storage;
    // pipelines
    dv::Compute _pipe_wip;

    SMAA _smaa;
    bool _smaa_enabled = true;
};