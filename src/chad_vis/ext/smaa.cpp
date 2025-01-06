#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "AreaTex.h"
#include "SearchTex.h"
#include "chad_vis/ext/smaa.hpp"

void SMAA::init_images(Device& device, vma::Allocator vmalloc, vk::Extent2D extent, vk::Format color_format) {
    // create SMAA render targets
    _img_edges.init({
        .device = device, .vmalloc = vmalloc,
        .format = vk::Format::eR8G8Unorm,
        .extent { extent.width, extent.height, 1 },
        .usage = 
            vk::ImageUsageFlagBits::eColorAttachment | 
            vk::ImageUsageFlagBits::eSampled,
    });
    _img_weights.init({
        .device = device, .vmalloc = vmalloc,
        .format = vk::Format::eR8G8B8A8Unorm,
        .extent { extent.width, extent.height, 1 },
        .usage = 
            vk::ImageUsageFlagBits::eColorAttachment | 
            vk::ImageUsageFlagBits::eSampled,
    });
    _img_output.init({
        .device = device, .vmalloc = vmalloc,
        .format = color_format,
        .extent { extent.width, extent.height, 1 },
        .usage = 
            vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eSampled
    });

    // load smaa lookup textures
    _img_search.init({
        .device = device, .vmalloc = vmalloc,
        .format = vk::Format::eR8Unorm,
        .extent = { SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, 1 },
        .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
    });
    auto search_tex = std::span(reinterpret_cast<const std::byte*>(searchTexBytes), sizeof(searchTexBytes));
    _img_search.load_texture(device, vmalloc, search_tex);
    _img_area.init({
        .device = device, .vmalloc = vmalloc,
        .format = vk::Format::eR8G8Unorm,
        .extent = { AREATEX_WIDTH, AREATEX_HEIGHT, 1 },
        .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
    });
    auto area_tex = std::span(reinterpret_cast<const std::byte*>(areaTexBytes), sizeof(areaTexBytes));
    _img_area.load_texture(device, vmalloc, area_tex);
    // transition smaa textures to their permanent layouts
    vk::CommandBuffer cmd = device.oneshot_begin(QueueType::eUniversal);
    dv::Image::TransitionInfo info_transition {
        .cmd = cmd,
        .new_layout = vk::ImageLayout::eShaderReadOnlyOptimal,
        .dst_stage = vk::PipelineStageFlagBits2::eFragmentShader,
        .dst_access = vk::AccessFlagBits2::eColorAttachmentRead
    };
    _img_search.transition_layout(info_transition);
    _img_area.transition_layout(info_transition);
    device.oneshot_end(QueueType::eUniversal, cmd);
}
void SMAA::init_pipelines(Device& device, vk::Extent2D extent, dv::Image& color, dv::DepthStencil& depth_stencil) {
    // create SMAA pipelines
    glm::vec4 SMAA_RT_METRICS = {
        1.0 / (double)extent.width,
        1.0 / (double)extent.height,
        (double)extent.width,
        (double)extent.height
    };
    std::array<vk::SpecializationMapEntry, 4> smaa_spec_entries {
        vk::SpecializationMapEntry { .constantID = 0, .offset =  0, .size = 4 },
        vk::SpecializationMapEntry { .constantID = 1, .offset =  4, .size = 4 },
        vk::SpecializationMapEntry { .constantID = 2, .offset =  8, .size = 4 },
        vk::SpecializationMapEntry { .constantID = 3, .offset = 12, .size = 4 }
    };
    vk::SpecializationInfo smaa_spec_info = {
        .mapEntryCount = smaa_spec_entries.size(),
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