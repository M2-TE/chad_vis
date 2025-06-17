module;
#include <cmath>
#include <vector>
#include <cstdint>
module renderer.renderer;

void Renderer::init(Device& device, Scene& scene, vk::Extent2D extent, bool srgb_output) {
    // allocate single command pool and buffer pair
    _command_pool = device._logical.createCommandPool({ .queueFamilyIndex = device._universal_i });
    vk::CommandBufferAllocateInfo bufferInfo {
        .commandPool = _command_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    };
    _command_buffer = device._logical.allocateCommandBuffers(bufferInfo).front();

    // create timeline semaphore
    _synchronization.init(device);
    
    // create images and pipelines
    init_images(device, extent);
    init_pipelines(device, extent, scene, srgb_output);
    _smaa.init(device, extent, _color, _depth_stencil);
}
void Renderer::destroy(Device& device) {
    _smaa.destroy(device);
    // destroy images
    _color.destroy(device);
    _storage.destroy(device);
    _depth_stencil.destroy(device);
    // destroy pipelines
    _pipe_default.destroy(device);
    _pipe_tone.destroy(device);
    // destroy command pools
    device._logical.destroyCommandPool(_command_pool);
    // destroy synchronization objects
    _synchronization.destroy(device);
}
void Renderer::resize(Device& device, Scene& scene, vk::Extent2D extent, bool srgb_output) {
    destroy(device);
    init(device, scene, extent, srgb_output);
}
void Renderer::render(Device& device, Swapchain& swapchain, Scene& scene) {
    // reset and record command buffer
    device._logical.resetCommandPool(_command_pool, {});
    vk::CommandBuffer cmd = _command_buffer;
    cmd.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    execute_pipes(cmd, scene);
    cmd.end();
    
    // submit command buffer
    _synchronization.prepare_for_write();
    vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eTopOfPipe;
    vk::TimelineSemaphoreSubmitInfo info_timeline {
        .waitSemaphoreValueCount = 1, .pWaitSemaphoreValues = &_synchronization._val_ready_to_write,
        .signalSemaphoreValueCount = 1, .pSignalSemaphoreValues = &_synchronization._val_ready_to_read,
    };
    device._universal_queue.submit(vk::SubmitInfo {
        .pNext = &info_timeline,
        .waitSemaphoreCount = 1, .pWaitSemaphores = &_synchronization._semaphore,
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1, .pCommandBuffers = &cmd,
        .signalSemaphoreCount = 1, .pSignalSemaphores = &_synchronization._semaphore,
    });
    
    // present drawn image
    swapchain.present(device, _storage, _synchronization);
}
void Renderer::wait(Device& device) {
    // wait until write is unblocked to make sure no work is left
    _synchronization.wait_ready_to_write(device);
}
void Renderer::init_images(Device& device, vk::Extent2D extent) {
    // create image with 16 bits color depth
    _color.init({
        .device = device,
        .format = vk::Format::eR16G16B16A16Sfloat,
        .extent { extent.width, extent.height, 1 },
        .usage = 
            vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eSampled,
        .priority = 1.0f,
    });
    // create storage image with same format as _color
    _storage.init({
        .device = device,
        .format = vk::Format::eR16G16B16A16Sfloat,
        .extent { extent.width, extent.height, 1 },
        .usage = 
            vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eStorage,
        .priority = 1.0f,
    });
    // create depth stencil with depth/stencil format picked by driver
    _depth_stencil.init(device, { extent.width, extent.height, 1 });
}
void Renderer::init_pipelines(Device& device, vk::Extent2D extent, Scene& scene, bool srgb_output) {
    // create graphics pipelines
    _pipe_default.init({
        .device = device,
        .extent = extent,
        .vs_path = "defaults/default.vert",
        .fs_path = "defaults/default.frag",
        .color = { .formats = _color._format },
        .depth = {
            .format = _depth_stencil._format,
            .write = vk::True,
            .test = vk::True,
        },
        .dynamic_states = {
            vk::DynamicState::eCullMode,
        },
    });
    _pipe_default.write_descriptor(device, 0, 0, scene._camera._buffer, vk::DescriptorType::eUniformBuffer);
    
    // create sRGB conversion pipeline
    uint32_t srgb = (uint32_t)srgb_output;
    std::vector<vk::SpecializationMapEntry> image_spec_entries {
        vk::SpecializationMapEntry { .constantID = 0, .offset = 0, .size = 4 },
    };
    _pipe_tone.init({
        .device = device,
        .cs_path = "defaults/tone_mapping.comp",
        .spec_info {
            .mapEntryCount = (uint32_t)image_spec_entries.size(),
            .pMapEntries = image_spec_entries.data(),
            .dataSize = sizeof(srgb),
            .pData = &srgb,
        }
    });
    _pipe_tone.write_descriptor(device, 0, 0, _storage, vk::DescriptorType::eStorageImage);
}
void Renderer::execute_pipes(vk::CommandBuffer cmd, Scene& scene) {
    // draw scene data
    _color.transition_layout({
        .cmd = cmd,
        .new_layout = vk::ImageLayout::eAttachmentOptimal,
        .dst_stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .dst_access = vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite });
    _depth_stencil.transition_layout({
        .cmd = cmd,
        .new_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        .dst_stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests,
        .dst_access = vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite});
    cmd.setCullMode(vk::CullModeFlagBits::eNone); // want to see both front and back faces
    _pipe_default.execute(cmd, _color, vk::AttachmentLoadOp::eClear, _depth_stencil, vk::AttachmentLoadOp::eClear, scene._mesh._mesh);
    // _pipe_default.execute(cmd, _color, vk::AttachmentLoadOp::eClear, _depth_stencil, vk::AttachmentLoadOp::eClear, scene._grid._query_points);

    // optionally run SMAA
    if (_smaa_enabled) _smaa.execute(cmd, _color, _depth_stencil);

    // convert from linear to srgb
    Image& final_image = _smaa_enabled ? _smaa.get_output() : _color;
    final_image.transition_layout({
        .cmd = cmd,
        .new_layout = vk::ImageLayout::eTransferSrcOptimal,
        .dst_stage = vk::PipelineStageFlagBits2::eBlit,
        .dst_access = vk::AccessFlagBits2::eTransferRead
    });
    _storage.transition_layout({
        .cmd = cmd,
        .new_layout = vk::ImageLayout::eTransferDstOptimal,
        .dst_stage = vk::PipelineStageFlagBits2::eBlit,
        .dst_access = vk::AccessFlagBits2::eTransferWrite
    });
    _storage.blit(cmd, final_image);
    _storage.transition_layout({
        .cmd = cmd,
        .new_layout = vk::ImageLayout::eGeneral,
        .dst_stage = vk::PipelineStageFlagBits2::eComputeShader,
        .dst_access = vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite
    });
    uint32_t nx = (uint32_t)std::ceil(_storage._extent.width / 8.0);
    uint32_t ny = (uint32_t)std::ceil(_storage._extent.height / 8.0);
    _pipe_tone.execute(cmd, nx, ny, 1);
}