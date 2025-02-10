module;
#include <span>
#include <tuple>
#include <vector>
#include <cstring>
export module image;
import vulkan_hpp;
import vk_mem_alloc_hpp;
import device;

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

module: private;
void Image::init(const CreateInfo& info) {
    _owning = true;
    _format = info.format;
    _extent = info.extent;
    _aspects = info.aspects;
    _last_layout = vk::ImageLayout::eUndefined;
    _last_access = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
    _last_stage = vk::PipelineStageFlagBits2::eTopOfPipe;
    // create image
    vk::ImageCreateInfo info_image {
        .imageType = vk::ImageType::e2D,
        .format = _format,
        .extent = _extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = info.usage
    };
    vma::AllocationCreateInfo info_alloc {
        .usage = vma::MemoryUsage::eAutoPreferDevice,
        .requiredFlags = vk::MemoryPropertyFlagBits::eDeviceLocal,
        .priority = info.priority,
    };
    std::tie(_image, _allocation) = info.device._vmalloc.createImage(info_image, info_alloc);
    
    // create image view
    vk::ImageViewCreateInfo info_view {
        .image = _image,
        .viewType = vk::ImageViewType::e2D,
        .format = _format,
        .components {
            .r = vk::ComponentSwizzle::eIdentity,
            .g = vk::ComponentSwizzle::eIdentity,
            .b = vk::ComponentSwizzle::eIdentity,
            .a = vk::ComponentSwizzle::eIdentity,
        },
        .subresourceRange {
            .aspectMask = _aspects,
            .baseMipLevel = 0,
            .levelCount = vk::RemainingMipLevels,
            .baseArrayLayer = 0,
            .layerCount = vk::RemainingArrayLayers,
        }
    };
    _view = info.device._logical.createImageView(info_view);
}
void Image::wrap(const WrapInfo& info) {
    _owning = false;
    _image = info.image;
    _view = info.image_view;
    _extent = info.extent;
    _aspects = info.aspects;
    _last_layout = vk::ImageLayout::eUndefined;
    _last_access = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
    _last_stage = vk::PipelineStageFlagBits2::eTopOfPipe;
}
void Image::destroy(Device& device) {
    if (_owning) {
        device._vmalloc.destroyImage(_image, _allocation);
        device._logical.destroyImageView(_view);
    }
}
void Image::load_texture(Device& device, std::span<const std::byte> tex_data) {
    // create image data buffer
    vk::BufferCreateInfo info_buffer {
        .size = tex_data.size(),
        .usage = vk::BufferUsageFlagBits::eTransferSrc,
        .sharingMode = vk::SharingMode::eExclusive,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &device._universal_i,
    };
    vma::AllocationCreateInfo info_allocation {
        .flags = 
            vma::AllocationCreateFlagBits::eHostAccessSequentialWrite |
            vma::AllocationCreateFlagBits::eMapped,
        .usage = 
            vma::MemoryUsage::eAuto,
        .requiredFlags =
            vk::MemoryPropertyFlagBits::eDeviceLocal,
        .preferredFlags =
            vk::MemoryPropertyFlagBits::eHostCoherent |
            vk::MemoryPropertyFlagBits::eHostVisible
    };
    auto [staging_buffer, staging_alloc] = device._vmalloc.createBuffer(info_buffer, info_allocation);

    // upload data
    void* mapped_data_p = device._vmalloc.mapMemory(staging_alloc);
    std::memcpy(mapped_data_p, tex_data.data(), tex_data.size());
    device._vmalloc.unmapMemory(staging_alloc);

    vk::CommandBuffer cmd = device.oneshot_begin(QueueType::eUniversal);
    // transition image for transfer
    TransitionInfo info_transition {
        .cmd = cmd,
        .new_layout = vk::ImageLayout::eTransferDstOptimal,
        .dst_stage = vk::PipelineStageFlagBits2::eTransfer,
        .dst_access = vk::AccessFlagBits2::eTransferWrite
    };
    transition_layout(info_transition);
    // copy staging buffer data to image
    vk::BufferImageCopy2 region {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource {
            .aspectMask = _aspects,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = vk::Offset3D(0, 0, 0),
        .imageExtent = _extent,
    };
    vk::CopyBufferToImageInfo2 info_copy {
        .srcBuffer = staging_buffer,
        .dstImage = _image,
        .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
        .regionCount = 1,
        .pRegions = &region,
    };
    cmd.copyBufferToImage2(info_copy);
    device.oneshot_end(QueueType::eUniversal, cmd);
    
    // clean up staging buffer
    device._vmalloc.destroyBuffer(staging_buffer, staging_alloc);
}
void Image::transition_layout(const TransitionInfo& info) {
    vk::ImageMemoryBarrier2 image_barrier {
        .srcStageMask = _last_stage,
        .srcAccessMask = _last_access,
        .dstStageMask = info.dst_stage,
        .dstAccessMask = info.dst_access,
        .oldLayout = _last_layout,
        .newLayout = info.new_layout,
        .image = _image,
        .subresourceRange {
            .aspectMask = _aspects,
            .baseMipLevel = 0,
            .levelCount = vk::RemainingMipLevels,
            .baseArrayLayer = 0,
            .layerCount = vk::RemainingArrayLayers,
        }
    };
    vk::DependencyInfo info_dep {
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &image_barrier,
    };
    info.cmd.pipelineBarrier2(info_dep);
    _last_layout = info.new_layout;
    _last_access = info.dst_access;
    _last_stage = info.dst_stage;
}
void Image::blit(vk::CommandBuffer cmd, Image& src_image) {
    vk::ImageBlit2 region {
        .srcSubresource { 
            .aspectMask = src_image._aspects,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .srcOffsets ={{ 
            vk::Offset3D(0, 0, 0), 
            vk::Offset3D(src_image._extent.width, src_image._extent.height, 1)
        }},
        .dstSubresource { 
            .aspectMask = _aspects,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .dstOffsets {{
            vk::Offset3D(0, 0, 0), 
            vk::Offset3D(_extent.width, _extent.height, 1)
        }},
    };
    cmd.blitImage2({
        .srcImage = src_image._image,
        .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
        .dstImage = _image,
        .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
        .regionCount = 1,
        .pRegions = &region,
        .filter = vk::Filter::eLinear,
    });
}
void DepthBuffer::init(Device& device, vk::Extent3D extent) {
    _owning = true;
    _extent = extent;
    _format = get_format();
    _aspects = vk::ImageAspectFlagBits::eDepth;
    _last_layout = vk::ImageLayout::eUndefined;
    _last_access = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
    _last_stage = vk::PipelineStageFlagBits2::eTopOfPipe;
    // create image
    vk::ImageCreateInfo info_image {
        .imageType = vk::ImageType::e2D,
        .format = _format,
        .extent = _extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled
    };
    vma::AllocationCreateInfo info_alloc {
        .usage = vma::MemoryUsage::eAutoPreferDevice,
        .requiredFlags = vk::MemoryPropertyFlagBits::eDeviceLocal,
        .priority = 1.0f,
    };
    std::tie(_image, _allocation) = device._vmalloc.createImage(info_image, info_alloc);
    
    // create image view
    vk::ImageViewCreateInfo info_depth_view {
        .image = _image,
        .viewType = vk::ImageViewType::e2D,
        .format = _format,
        .components {
            .r = vk::ComponentSwizzle::eIdentity,
            .g = vk::ComponentSwizzle::eIdentity,
            .b = vk::ComponentSwizzle::eIdentity,
            .a = vk::ComponentSwizzle::eIdentity,
        },
        .subresourceRange {
            .aspectMask = _aspects,
            .baseMipLevel = 0,
            .levelCount = vk::RemainingMipLevels,
            .baseArrayLayer = 0,
            .layerCount = vk::RemainingArrayLayers,
        }
    };
    _view = device._logical.createImageView(info_depth_view);
}
void DepthStencil::init(Device& device, vk::Extent3D extent) {
    _owning = true;
    _extent = extent;
    _format = get_format();
    _aspects = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    _last_layout = vk::ImageLayout::eUndefined;
    _last_access = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
    _last_stage = vk::PipelineStageFlagBits2::eTopOfPipe;
    // create image
    vk::ImageCreateInfo info_image {
        .imageType = vk::ImageType::e2D,
        .format = _format,
        .extent = _extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled
    };
    vma::AllocationCreateInfo info_alloc {
        .usage = vma::MemoryUsage::eAutoPreferDevice,
        .requiredFlags = vk::MemoryPropertyFlagBits::eDeviceLocal,
        .priority = 1.0f,
    };
    std::tie(_image, _allocation) = device._vmalloc.createImage(info_image, info_alloc);
    
    // create image view
    vk::ImageViewCreateInfo info_depth_view {
        .image = _image,
        .viewType = vk::ImageViewType::e2D,
        .format = _format,
        .components {
            .r = vk::ComponentSwizzle::eIdentity,
            .g = vk::ComponentSwizzle::eIdentity,
            .b = vk::ComponentSwizzle::eIdentity,
            .a = vk::ComponentSwizzle::eIdentity,
        },
        .subresourceRange {
            .aspectMask = _aspects,
            .baseMipLevel = 0,
            .levelCount = vk::RemainingMipLevels,
            .baseArrayLayer = 0,
            .layerCount = vk::RemainingArrayLayers,
        }
    };
    _view = device._logical.createImageView(info_depth_view);
}