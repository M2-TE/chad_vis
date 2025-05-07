module;
#include <span>
#include <vector>
export module buffers.image;
import core.device;
import vulkan_hpp;
import vulkan_ma_hpp;

export struct Image {
    struct CreateInfo;
    struct WrapInfo;
    struct TransitionInfo;
    // create new image with direct ownership
    void init(const CreateInfo& info);
    // wrap existing image without direct ownership
    void wrap(const WrapInfo& info);
    // destroy device-side resources if owning
    void destroy(Device& device);
    void load_texture(Device& device, std::span<const std::byte> tex_data);
    void transition_layout(const TransitionInfo& info);
    void blit(vk::CommandBuffer cmd, Image& src_image);
    
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
export struct DepthBuffer: public Image {
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
    void init(Device& device, vk::Extent3D extent);
};
export struct DepthStencil: public Image {
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
    void init(Device& device, vk::Extent3D extent);
};

struct Image::CreateInfo {
    const Device& device;
    vk::Format format;
    vk::Extent3D extent;
    vk::ImageUsageFlags usage;
    vk::ImageAspectFlags aspects = vk::ImageAspectFlagBits::eColor;
    float priority = 0.5f;
};
struct Image::WrapInfo {
    vk::Image image;
    vk::ImageView image_view;
    vk::Extent3D extent;
    vk::ImageAspectFlags aspects;
};
struct Image::TransitionInfo {
    vk::CommandBuffer cmd;
    vk::ImageLayout new_layout;
    vk::PipelineStageFlags2 dst_stage = vk::PipelineStageFlagBits2::eTopOfPipe;
    vk::AccessFlags2 dst_access = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
};