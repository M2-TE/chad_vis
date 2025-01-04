#pragma once
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.hpp>
#include "chad_vis/device/queues.hpp"

namespace dv {
struct Image {
    struct CreateInfo {
        vk::Device device;
        vma::Allocator vmalloc;
        vk::Format format;
        vk::Extent3D extent;
        vk::ImageUsageFlags usage;
        vk::ImageAspectFlags aspects = vk::ImageAspectFlagBits::eColor;
        float priority = 0.5f;
    };
    struct WrapInfo {
        vk::Image image;
        vk::ImageView image_view;
        vk::Extent3D extent;
        vk::ImageAspectFlags aspects;
    };
    struct TransitionInfo {
        vk::CommandBuffer cmd;
        vk::ImageLayout new_layout;
        vk::PipelineStageFlags2 dst_stage = vk::PipelineStageFlagBits2::eTopOfPipe;
        vk::AccessFlags2 dst_access = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
    };
    // create new image with direct ownership
    void init(const CreateInfo& info);
    // wrap existing image without direct ownership
    void wrap(const WrapInfo& info);
    // destroy device-side resources if owning
    void destroy(vk::Device device, vma::Allocator vmalloc);
    
    void load_texture(vk::Device device, vma::Allocator vmalloc, dv::Queues& queues, std::span<const std::byte> tex_data);
    void transition_layout(const TransitionInfo& info);
    void blit(vk::CommandBuffer cmd, dv::Image& src_image);
    
    vma::Allocation _allocation;
    vk::Image _image;
    vk::ImageView _view;
    vk::Extent3D _extent;
    vk::Format _format;
    vk::ImageAspectFlags _aspects;
    vk::ImageLayout _last_layout;
    vk::AccessFlags2 _last_access;
    vk::PipelineStageFlags2 _last_stage;
    bool _owning;
};

struct DepthBuffer: public Image {
    static vk::Format& get_format() {
        static vk::Format format = vk::Format::eUndefined;
        return format;
    }
    static void set_format(vk::PhysicalDevice phys_device) {
        // depth formats in order of preference
        std::vector<vk::Format> formats = {
            vk::Format::eD32Sfloat,
            vk::Format::eD24UnormS8Uint,
            vk::Format::eD16UnormS8Uint,
            vk::Format::eD32SfloatS8Uint,
        };
        // set depth format to first supported format
        for (auto& format: formats) {
            vk::FormatProperties props = phys_device.getFormatProperties(format);
            if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
                get_format() = format;
                break;
            }
        }
    }
    void init(vk::Device device, vma::Allocator vmalloc, vk::Extent3D extent);
};
struct DepthStencil: public Image {
    static vk::Format& get_format() {
        static vk::Format format = vk::Format::eUndefined;
        return format;
    }
    static void set_format(vk::PhysicalDevice phys_device) {
        // depth stencil formats in order of preference
        std::vector<vk::Format> formats = {
            vk::Format::eD24UnormS8Uint,
            vk::Format::eD16UnormS8Uint,
            vk::Format::eD32SfloatS8Uint,
        };
        // set depth stencil format to first supported format
        for (auto& format: formats) {
            vk::FormatProperties props = phys_device.getFormatProperties(format);
            if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
                get_format() = format;
                break;
            }
        }
    }
    void init(vk::Device device, vma::Allocator vmalloc, vk::Extent3D extent);
};
} // namespace dv