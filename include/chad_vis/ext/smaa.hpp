#pragma once
#include <vulkan/vulkan.hpp>
#include "chad_vis/core/device.hpp"
#include "chad_vis/core/pipeline.hpp"
#include "chad_vis/device/image.hpp"

class SMAA {
public:
    void init(Device& device, vma::Allocator vmalloc, vk::Extent2D extent, dv::Image& color, dv::DepthStencil& depth_stencil) {
        init_images(device, vmalloc, extent, color._format);
        init_pipelines(device, extent, color, depth_stencil);
    }
    void destroy(Device& device, vma::Allocator vmalloc) {
        // destroy images
        _img_output.destroy(device, vmalloc);
        _img_weights.destroy(device, vmalloc);
        _img_edges.destroy(device, vmalloc);
        _img_area.destroy(device, vmalloc);
        _img_search.destroy(device, vmalloc);
        // destroy pipelines
        _pipe_blending.destroy(device);
        _pipe_weights.destroy(device);
        _pipe_edges.destroy(device);
    }
    void execute(vk::CommandBuffer cmd, dv::Image& color, dv::DepthStencil& depth_stencil) {
        dv::Image::TransitionInfo info_transition_read {
            .cmd = cmd,
            .new_layout = vk::ImageLayout::eShaderReadOnlyOptimal,
            .dst_stage = vk::PipelineStageFlagBits2::eFragmentShader,
            .dst_access = vk::AccessFlagBits2::eShaderSampledRead
        };
        dv::Image::TransitionInfo info_transition_write {
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
    auto get_output() -> dv::Image& {
        return _img_output;
    }

private:
    void init_images(Device& device, vma::Allocator vmalloc, vk::Extent2D extent, vk::Format color_format);
    void init_pipelines(Device& device, vk::Extent2D extent, dv::Image& color, dv::DepthStencil& depth_stencil);
    
    // static images
    dv::Image _img_area;
    dv::Image _img_search;
    // render targets
    dv::Image _img_edges;
    dv::Image _img_weights;
    dv::Image _img_output;
    // pipelines
    Graphics _pipe_edges;
    Graphics _pipe_weights;
    Graphics _pipe_blending;
};