module;
#include "AreaTex.h"
#include "SearchTex.h"
export module ext.smaa;
import vulkan_hpp;
import vulkan.allocator;
import core.device;
import buffers.image;
import renderer.pipeline;

export struct SMAA {
    void init(Device& device, vk::Extent2D extent, Image& color, DepthStencil& depth_stencil);
    void destroy(Device& device);
    void resize(Device& device, vk::Extent2D extent, Image& color, DepthStencil& depth_stencil);
    void execute(vk::CommandBuffer cmd, Image& color, DepthStencil& depth_stencil);
    auto get_output() -> Image&;

private:
    void init_render_targets(Device& device, vk::Extent2D extent, vk::Format color_format);
    void init_lookup_textures(Device& device);
    void init_pipelines(Device& device, vk::Extent2D extent, Image& color, DepthStencil& depth_stencil);
    
    // static images
    Image _img_area;
    Image _img_search;
    // render targets
    Image _img_edges;
    Image _img_weights;
    Image _img_output;
    // pipelines
    Graphics _pipe_edges;
    Graphics _pipe_weights;
    Graphics _pipe_blending;
};

module: private;
void SMAA::init(Device& device, vk::Extent2D extent, Image& color, DepthStencil& depth_stencil) {
    init_lookup_textures(device);
    init_render_targets(device, extent, color._format);
    init_pipelines(device, extent, color, depth_stencil);
}
void SMAA::destroy(Device& device) {
    // destroy images
    _img_output.destroy(device);
    _img_weights.destroy(device);
    _img_edges.destroy(device);
    _img_area.destroy(device);
    _img_search.destroy(device);
    // destroy pipelines
    _pipe_blending.destroy(device);
    _pipe_weights.destroy(device);
    _pipe_edges.destroy(device);
}
void SMAA::resize(Device& device, vk::Extent2D extent, Image& color, DepthStencil& depth_stencil) {
    // destroy images
    _img_output.destroy(device);
    _img_weights.destroy(device);
    _img_edges.destroy(device);
    // destroy pipelines
    _pipe_blending.destroy(device);
    _pipe_weights.destroy(device);
    _pipe_edges.destroy(device);

    // recreate them
    init_render_targets(device, extent, color._format);
    init_pipelines(device, extent, color, depth_stencil);
}
void SMAA::execute(vk::CommandBuffer cmd, Image& color, DepthStencil& depth_stencil) {
    Image::TransitionInfo info_transition_read {
        .cmd = cmd,
        .new_layout = vk::ImageLayout::eShaderReadOnlyOptimal,
        .dst_stage = vk::PipelineStageFlagBits2::eFragmentShader,
        .dst_access = vk::AccessFlagBits2::eShaderSampledRead
    };
    Image::TransitionInfo info_transition_write {
        .cmd = cmd,
        .new_layout = vk::ImageLayout::eColorAttachmentOptimal,
        .dst_stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .dst_access = vk::AccessFlagBits2::eColorAttachmentWrite
    };
    // transition depth stencil for stencil tests
    depth_stencil.transition_layout({
        .cmd = cmd,
        .new_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        .dst_stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests,
        .dst_access = vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite
    });
    // SMAA edge detection
    color.transition_layout(info_transition_read);
    _img_edges.transition_layout(info_transition_write);
    _pipe_edges.execute(cmd, _img_edges, vk::AttachmentLoadOp::eClear, depth_stencil, vk::AttachmentLoadOp::eLoad);

    // SMAA blending weight calculation
    _img_edges.transition_layout(info_transition_read);
    _img_weights.transition_layout(info_transition_write);
    _pipe_weights.execute(cmd, _img_weights, vk::AttachmentLoadOp::eClear, depth_stencil, vk::AttachmentLoadOp::eLoad);

    // SMAA neighborhood blending
    _img_weights.transition_layout(info_transition_read);
    _img_output.transition_layout(info_transition_write);
    _pipe_blending.execute(cmd, _img_output, vk::AttachmentLoadOp::eClear);
}
auto SMAA::get_output() -> Image& {
    return _img_output;
}
void SMAA::init_render_targets(Device& device, vk::Extent2D extent, vk::Format color_format) {
    // create SMAA render targets
    _img_edges.init({
        .device = device,
        .format = vk::Format::eR8G8Unorm,
        .extent { extent.width, extent.height, 1 },
        .usage = 
            vk::ImageUsageFlagBits::eColorAttachment | 
            vk::ImageUsageFlagBits::eSampled,
    });
    _img_weights.init({
        .device = device, 
        .format = vk::Format::eR8G8B8A8Unorm,
        .extent { extent.width, extent.height, 1 },
        .usage = 
            vk::ImageUsageFlagBits::eColorAttachment | 
            vk::ImageUsageFlagBits::eSampled,
    });
    _img_output.init({
        .device = device,
        .format = color_format,
        .extent { extent.width, extent.height, 1 },
        .usage = 
            vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eSampled
    });
}
void SMAA::init_lookup_textures(Device& device) {
    _img_search.init({
        .device = device,
        .format = vk::Format::eR8Unorm,
        .extent = { SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, 1 },
        .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
    });
    auto search_tex = std::span(reinterpret_cast<const std::byte*>(searchTexBytes), sizeof(searchTexBytes));
    _img_search.load_texture(device, search_tex);
    _img_area.init({
        .device = device,
        .format = vk::Format::eR8G8Unorm,
        .extent = { AREATEX_WIDTH, AREATEX_HEIGHT, 1 },
        .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
    });
    auto area_tex = std::span(reinterpret_cast<const std::byte*>(areaTexBytes), sizeof(areaTexBytes));
    _img_area.load_texture(device, area_tex);
    // transition smaa textures to their permanent layouts
    vk::CommandBuffer cmd = device.oneshot_begin(QueueType::eUniversal);
    Image::TransitionInfo info_transition {
        .cmd = cmd,
        .new_layout = vk::ImageLayout::eShaderReadOnlyOptimal,
        .dst_stage = vk::PipelineStageFlagBits2::eFragmentShader,
        .dst_access = vk::AccessFlagBits2::eColorAttachmentRead
    };
    _img_search.transition_layout(info_transition);
    _img_area.transition_layout(info_transition);
    device.oneshot_end(QueueType::eUniversal, cmd);
}
void SMAA::init_pipelines(Device& device, vk::Extent2D extent, Image& color, DepthStencil& depth_stencil) {
    // create SMAA pipelines
    std::array<float, 4> SMAA_RT_METRICS = {
        1.0f / (float)extent.width,
        1.0f / (float)extent.height,
        (float)extent.width,
        (float)extent.height
    };
    std::array<vk::SpecializationMapEntry, 4> smaa_spec_entries {
        vk::SpecializationMapEntry { .constantID = 0, .offset =  0, .size = 4 },
        vk::SpecializationMapEntry { .constantID = 1, .offset =  4, .size = 4 },
        vk::SpecializationMapEntry { .constantID = 2, .offset =  8, .size = 4 },
        vk::SpecializationMapEntry { .constantID = 3, .offset = 12, .size = 4 }
    };
    vk::SpecializationInfo smaa_spec_info = {
        .mapEntryCount = (uint32_t)smaa_spec_entries.size(),
        .pMapEntries = smaa_spec_entries.data(),
        .dataSize = sizeof(SMAA_RT_METRICS),
        .pData = &SMAA_RT_METRICS
    };
    _pipe_edges.init({
        .device = device,
        .extent = extent,
        .vs_path = "smaa/edges.vert", .vs_spec = smaa_spec_info,
        .fs_path = "smaa/edges.frag", .fs_spec = smaa_spec_info,
        .color {
            .formats = _img_edges._format,
        },
        .stencil {
            .format = depth_stencil._format,
            .test = vk::True,
            .front = {
                .failOp = vk::StencilOp::eKeep,
                .passOp = vk::StencilOp::eReplace,
                .compareOp = vk::CompareOp::eAlways,
                .compareMask = 0xff,
                .writeMask = 0xff,
                .reference = 1,
            }
        },
    });
    _pipe_weights.init({
        .device = device,
        .extent = extent,
        .vs_path = "smaa/weights.vert", .vs_spec = smaa_spec_info,
        .fs_path = "smaa/weights.frag", .fs_spec = smaa_spec_info,
        .color {
            .formats = _img_weights._format,
        },
        .stencil {
            .format = depth_stencil._format,
            .test = vk::True,
            .front = {
                .failOp = vk::StencilOp::eKeep,
                .passOp = vk::StencilOp::eKeep,
                .compareOp = vk::CompareOp::eEqual,
                .compareMask = 0xff,
                .writeMask = 0xff,
                .reference = 1,
            },
        },
    });
    _pipe_blending.init({
        .device = device,
        .extent = extent,
        .vs_path = "smaa/blending.vert", .vs_spec = smaa_spec_info,
        .fs_path = "smaa/blending.frag", .fs_spec = smaa_spec_info,
        .color {
            .formats = color._format,
        },
    });
    // update SMAA input texture descriptors
    _pipe_edges.write_descriptor(device, 0, 0, color, vk::DescriptorType::eCombinedImageSampler);
    //
    _pipe_weights.write_descriptor(device, 0, 0, _img_area, vk::DescriptorType::eCombinedImageSampler);
    _pipe_weights.write_descriptor(device, 0, 1, _img_search, vk::DescriptorType::eCombinedImageSampler);
    _pipe_weights.write_descriptor(device, 0, 2, _img_edges, vk::DescriptorType::eCombinedImageSampler);
    //
    _pipe_blending.write_descriptor(device, 0, 0, _img_weights, vk::DescriptorType::eCombinedImageSampler);
    _pipe_blending.write_descriptor(device, 0, 1, color, vk::DescriptorType::eCombinedImageSampler);
}