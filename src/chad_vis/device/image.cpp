#include "chad_vis/device/image.hpp"

namespace dv {
    // create new image with direct ownership
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
        std::tie(_image, _allocation) = info.vmalloc.createImage(info_image, info_alloc);
        
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
        _view = info.device.createImageView(info_view);
    }
    // wrap existing image without direct ownership
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
    // destroy device-side resources if owning
    void Image::destroy(Device& device, vma::Allocator vmalloc) {
        if (_owning) {
            vmalloc.destroyImage(_image, _allocation);
            device._logical.destroyImageView(_view);
        }
    }

    void Image::load_texture(Device& device, vma::Allocator vmalloc, std::span<const std::byte> tex_data) {
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
        auto [staging_buffer, staging_alloc] = vmalloc.createBuffer(info_buffer, info_allocation);

        // upload data
        void* mapped_data_p = vmalloc.mapMemory(staging_alloc);
        std::memcpy(mapped_data_p, tex_data.data(), tex_data.size());
        vmalloc.unmapMemory(staging_alloc);

        vk::CommandBuffer cmd = device.oneshot_begin();
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
        device.oneshot_end(cmd);
        
        // clean up staging buffer
        vmalloc.destroyBuffer(staging_buffer, staging_alloc);
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
    void Image::blit(vk::CommandBuffer cmd, dv::Image& src_image) {
        vk::ImageBlit2 region {
            .srcSubresource { 
                .aspectMask = src_image._aspects,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .srcOffsets = std::array<vk::Offset3D, 2>{ 
                vk::Offset3D(), 
                vk::Offset3D(src_image._extent.width, src_image._extent.height, 1) },
            .dstSubresource { 
                .aspectMask = _aspects,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .dstOffsets = std::array<vk::Offset3D, 2>{ 
                vk::Offset3D(), 
                vk::Offset3D(_extent.width, _extent.height, 1) },
        };
        vk::BlitImageInfo2 info_blit {
            .srcImage = src_image._image,
            .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
            .dstImage = _image,
            .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
            .regionCount = 1,
            .pRegions = &region,
            .filter = vk::Filter::eLinear,
        };
        cmd.blitImage2(info_blit);
    }

    void DepthBuffer::init(vk::Device device, vma::Allocator vmalloc, vk::Extent3D extent) {
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
        std::tie(_image, _allocation) = vmalloc.createImage(info_image, info_alloc);
        
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
        _view = device.createImageView(info_depth_view);
    }
    void DepthStencil::init(vk::Device device, vma::Allocator vmalloc, vk::Extent3D extent) {
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
        std::tie(_image, _allocation) = vmalloc.createImage(info_image, info_alloc);
        
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
        _view = device.createImageView(info_depth_view);
    }
} // namespace dv